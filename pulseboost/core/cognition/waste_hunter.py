"""
PulseBoost waste hunter.
"""
from __future__ import annotations

import hashlib
import json
import time
from collections import defaultdict

from core.cognition.leak_analyzer import LeakAnalyzer


PILLARS = ("cpu", "ram", "disk", "network", "thermal", "scheduler", "leaks")


class WasteHunter:
    BACKGROUND_ABUSERS = {
        "SearchIndexer.exe",
        "MsMpEng.exe",
        "SgrmBroker.exe",
        "RuntimeBroker.exe",
        "backgroundTaskHost.exe",
        "TiWorker.exe",
    }
    SYNC_PROCESSES = {"OneDrive.exe", "Dropbox.exe", "GoogleDriveFS.exe", "iCloudDrive.exe"}
    BROWSER_NAMES = {"chrome.exe", "msedge.exe", "firefox.exe", "Code.exe", "code.exe", "electron.exe"}

    def __init__(self, memory):
        self.memory = memory
        self.process_history: dict[int, list[dict]] = defaultdict(list)
        self.leak_analyzer = LeakAnalyzer()

    def hunt(self, snapshot, session_mode: str) -> dict[str, object]:
        self._update_process_history(snapshot)
        findings: list[dict] = []
        process_intelligence: list[dict] = []
        leak_candidates: list[dict] = []

        cpu_findings = self._hunt_cpu_waste(snapshot, session_mode)
        findings.extend(cpu_findings)

        ram_findings, leak_candidates = self._hunt_ram_waste(snapshot)
        findings.extend(ram_findings)

        findings.extend(self._hunt_disk_waste(snapshot))
        findings.extend(self._hunt_network_waste(snapshot, session_mode))
        findings.extend(self._hunt_thermal_waste(snapshot))

        for process in snapshot.top_processes:
            process_intelligence.append(
                self._build_process_intelligence(process, findings, leak_candidates)
            )

        findings.sort(key=lambda item: item["priority_score"], reverse=True)
        process_intelligence.sort(key=lambda item: item["waste_score"], reverse=True)
        pillar_scores = self._pillar_scores(findings)

        efficiency_pressure = sum(item["waste_score"] for item in process_intelligence[:6]) / max(1, min(len(process_intelligence), 6))
        efficiency_score = round(max(0.0, 100.0 - min(70.0, efficiency_pressure * 0.42)), 1)

        for finding in findings:
            finding.setdefault("id", self._optimization_id(finding))
            finding.setdefault("status", "pending")
            finding.setdefault("timestamp", snapshot.timestamp)

        return {
            "optimizations": findings,
            "process_intelligence": process_intelligence,
            "leak_candidates": leak_candidates,
            "pillar_scores": pillar_scores,
            "system_efficiency_score": efficiency_score,
        }

    def _update_process_history(self, snapshot) -> None:
        seen = set()
        for proc in snapshot.top_processes:
            pid = int(proc.get("pid", 0))
            if not pid:
                continue
            seen.add(pid)
            record = {
                "timestamp": snapshot.timestamp,
                "cpu": float(proc.get("cpu_percent", 0.0)),
                "ram": int(proc.get("ram_bytes", 0)),
                "name": proc.get("name", "unknown"),
            }
            self.process_history[pid].append(record)
            self.process_history[pid] = self.process_history[pid][-20:]

        stale = [pid for pid, history in self.process_history.items() if pid not in seen and history and (snapshot.timestamp - history[-1]["timestamp"]) > 1800]
        for pid in stale:
            self.process_history.pop(pid, None)

    def _hunt_cpu_waste(self, snapshot, session_mode: str) -> list[dict]:
        findings = []
        for proc in snapshot.top_processes:
            name = proc.get("name", "")
            cpu = float(proc.get("cpu_percent", 0.0))
            pid = int(proc.get("pid", 0))
            history = self.process_history.get(pid, [])

            if name in self.BACKGROUND_ABUSERS and cpu > 12:
                findings.append(
                    {
                        "pillar": "cpu",
                        "priority": "high" if cpu > 20 else "medium",
                        "priority_score": cpu,
                        "title": f"{name} hogging CPU",
                        "finding": f"{name} is using {cpu:.1f}% CPU during a {session_mode} session.",
                        "impact": f"Stealing roughly {cpu:.0f}% CPU from foreground work.",
                        "action": "reduce_process_priority",
                        "params": {"pid": pid, "name": name},
                        "risk": "SAFE",
                        "auto_execute": True,
                        "user_message": f"{name} is acting like a background CPU hog. PulseBoost can push it into a lower priority lane.",
                        "estimated_gain": f"Recover ~{max(3.0, cpu * 0.55):.0f}% CPU",
                    }
                )

            if len(history) >= 5:
                values = [item["cpu"] for item in history[-5:]]
                variance = max(values) - min(values)
                average = sum(values) / len(values)
                if variance > 35 and average > 15:
                    findings.append(
                        {
                            "pillar": "cpu",
                            "priority": "medium",
                            "priority_score": variance,
                            "title": f"{name} thrashing CPU",
                            "finding": f"{name} is swinging between {min(values):.0f}% and {max(values):.0f}% CPU repeatedly.",
                            "impact": "Spiky CPU demand causes stutter and scheduling waste.",
                            "action": "investigate_process",
                            "params": {"pid": pid, "name": name},
                            "risk": "SAFE",
                            "auto_execute": False,
                            "user_message": f"{name} is thrashing CPU instead of consuming it steadily. That usually means inefficient polling or contention.",
                            "estimated_gain": "Stabilize frame time and responsiveness",
                        }
                    )
        return findings

    def _hunt_ram_waste(self, snapshot) -> tuple[list[dict], list[dict]]:
        findings: list[dict] = []
        leak_candidates: list[dict] = []
        for pid, history in self.process_history.items():
            leak = self.leak_analyzer.analyze(pid, history[-1]["name"] if history else "unknown", history)
            if not leak:
                continue
            if leak["growth_rate_mb_per_min"] > 2:
                leak_candidates.append(leak)
            if leak["is_leak"]:
                findings.append(
                    {
                        "pillar": "ram",
                        "priority": "high" if leak["growth_rate_mb_per_min"] > 8 else "medium",
                        "priority_score": leak["growth_rate_mb_per_min"] * 10,
                        "title": f"{history[-1]['name']} memory leak",
                        "finding": f"{history[-1]['name']} is growing at {leak['growth_rate_mb_per_min']:.1f} MB/min.",
                        "impact": f"It could reach {leak['projected_30min_mb']:.0f} MB within 30 minutes.",
                        "action": "warn_leak",
                        "params": {"pid": pid, "name": history[-1]["name"], "growth_rate": leak["growth_rate_mb_per_min"]},
                        "risk": "MODERATE",
                        "auto_execute": False,
                        "user_message": f"{history[-1]['name']} looks like a memory leak candidate. PulseBoost recommends intervention before RAM pressure spikes.",
                        "estimated_gain": f"Prevent ~{max(0.0, leak['projected_30min_mb'] - leak['current_ram_mb']):.0f} MB waste",
                    }
                )
        return findings, leak_candidates

    def _hunt_disk_waste(self, snapshot) -> list[dict]:
        findings = []
        write_mb = float(snapshot.disk_write_bytes) / (1024 * 1024)
        if write_mb > 50:
            findings.append(
                {
                    "pillar": "disk",
                    "priority": "high",
                    "priority_score": write_mb,
                    "title": "Disk writes spiking",
                    "finding": f"The machine is writing {write_mb:.1f} MB/s to disk.",
                    "impact": "Heavy background writes hurt responsiveness and SSD lifespan.",
                    "action": "identify_disk_writer",
                    "params": {},
                    "risk": "SAFE",
                    "auto_execute": False,
                    "user_message": "PulseBoost found sustained heavy disk writes and is treating them as waste until proven useful.",
                    "estimated_gain": "Reduce SSD wear and reclaim responsiveness",
                }
            )
        if snapshot.disk_percent > 88:
            findings.append(
                {
                    "pillar": "disk",
                    "priority": "immediate" if snapshot.disk_percent > 94 else "high",
                    "priority_score": float(snapshot.disk_percent),
                    "title": "Disk space almost gone",
                    "finding": f"The primary disk is {snapshot.disk_percent:.0f}% full.",
                    "impact": "Low free space degrades writes, updates, and app startup.",
                    "action": "clear_temp_files",
                    "params": {},
                    "risk": "SAFE",
                    "auto_execute": True,
                    "user_message": "Free space is low enough to hurt the machine. PulseBoost can clear temporary waste immediately.",
                    "estimated_gain": "Recover temp space now",
                }
            )
        return findings

    def _hunt_network_waste(self, snapshot, session_mode: str) -> list[dict]:
        findings = []
        upload_mb = float(snapshot.net_sent_bytes) / (1024 * 1024)
        if upload_mb > 5:
            sync_process = next((proc for proc in snapshot.top_processes if proc.get("name") in self.SYNC_PROCESSES), None)
            if sync_process:
                findings.append(
                    {
                        "pillar": "network",
                        "priority": "high" if upload_mb > 10 else "medium",
                        "priority_score": upload_mb,
                        "title": f"{sync_process['name']} saturating upload",
                        "finding": f"{sync_process['name']} aligns with a {upload_mb:.1f} MB/s upload spike during {session_mode}.",
                        "impact": "Background sync is competing with active tasks for uplink.",
                        "action": "investigate_process",
                        "params": {"pid": sync_process["pid"], "name": sync_process["name"]},
                        "risk": "SAFE",
                        "auto_execute": False,
                        "user_message": f"{sync_process['name']} looks like the main bandwidth hog right now. PulseBoost recommends deferring it.",
                        "estimated_gain": "Reclaim upload bandwidth",
                    }
                )
        return findings

    def _hunt_thermal_waste(self, snapshot) -> list[dict]:
        findings = []
        temperature = getattr(snapshot, "temperature", None)
        if temperature is None:
            return findings
        if temperature > 85:
            hottest = snapshot.top_processes[0] if snapshot.top_processes else None
            findings.append(
                {
                    "pillar": "thermal",
                    "priority": "immediate" if temperature > 92 else "high",
                    "priority_score": float(temperature),
                    "title": "Thermal pressure rising",
                    "finding": f"CPU temperature is {temperature:.0f}°C.",
                    "impact": "Thermal throttling is active or very close, which wastes performance.",
                    "action": "reduce_process_priority" if hottest else "investigate_process",
                    "params": {"pid": hottest["pid"], "name": hottest["name"]} if hottest else {},
                    "risk": "SAFE",
                    "auto_execute": bool(hottest and temperature > 92),
                    "user_message": "PulseBoost sees heat that is too high for comfortable sustained performance. It can cool the hottest workload by lowering priority.",
                    "estimated_gain": "Lower temperature and recover boost headroom",
                }
            )
        return findings

    def _build_process_intelligence(self, process: dict, findings: list[dict], leak_candidates: list[dict]) -> dict:
        cpu = float(process.get("cpu_percent", 0.0))
        ram_mb = float(process.get("ram_mb", 0.0))
        waste_score = min(100.0, cpu * 1.8 + ram_mb * 0.08)
        anomaly_level = "Normal"
        trend = "stable"
        history = self.process_history.get(int(process.get("pid", 0)), [])
        if len(history) >= 4:
            cpu_trend = history[-1]["cpu"] - history[0]["cpu"]
            ram_trend = history[-1]["ram"] - history[0]["ram"]
            trend = "up" if ram_trend > 20 * 1024 * 1024 else "down" if ram_trend < -20 * 1024 * 1024 else "stable"
            if abs(cpu_trend) > 30:
                waste_score += 8
        leak = next((item for item in leak_candidates if item["pid"] == process["pid"] and item["is_leak"]), None)
        if leak:
            anomaly_level = "LEAK"
            waste_score += 25
        elif any(item["params"].get("pid") == process["pid"] for item in findings):
            anomaly_level = "Above Baseline"
            waste_score += 12
        waste_score = round(min(100.0, waste_score), 1)
        return {
            "pid": process["pid"],
            "name": process["name"],
            "cpu_percent": cpu,
            "ram_bytes": int(process.get("ram_bytes", 0)),
            "ram_mb": ram_mb,
            "waste_score": waste_score,
            "anomaly_level": anomaly_level,
            "trend": trend,
            "status": process.get("status", "unknown"),
            "ai_flagged": anomaly_level != "Normal",
        }

    def _pillar_scores(self, findings: list[dict]) -> dict[str, dict]:
        scores = {}
        for pillar in PILLARS:
            pillar_findings = [item for item in findings if item["pillar"] == pillar or (pillar == "leaks" and item["pillar"] == "ram" and item["action"] == "warn_leak")]
            active = len(pillar_findings)
            penalty = sum(min(30.0, float(item.get("priority_score", 0.0)) * 0.35) for item in pillar_findings)
            score = round(max(0.0, 100.0 - penalty), 1)
            scores[pillar] = {
                "score": score,
                "active": active,
                "last_action": pillar_findings[0]["title"] if pillar_findings else None,
            }
        return scores

    def _optimization_id(self, finding: dict) -> str:
        identity = {
            "pillar": finding.get("pillar"),
            "action": finding.get("action"),
            "params": finding.get("params", {}),
            "title": finding.get("title"),
        }
        digest = hashlib.sha1(json.dumps(identity, sort_keys=True).encode("utf-8")).hexdigest()[:12]
        return f"opt_{digest}"
