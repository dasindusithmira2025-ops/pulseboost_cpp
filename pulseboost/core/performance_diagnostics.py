"""
Frame-time ingestion and live bottleneck classification.
"""
from __future__ import annotations

import csv
import math
import statistics
import time
from pathlib import Path
from typing import Any


BOTTLENECK_LABELS = {
    "CPU_BOUND",
    "GPU_BOUND",
    "RAM_BOUND",
    "DISK_BOUND",
    "NETWORK_BOUND",
    "THERMAL_BOUND",
    "BACKGROUND_PROCESS_BOUND",
}


class FrameTimeCapture:
    """Read real frame pacing samples from an external PresentMon-style CSV stream.

    PulseBoost does not inject hooks into games. When a trusted capture source is
    configured, this class ingests recent samples and exposes FPS/frame-time
    values. Without that source it returns an explicit unsupported state.
    """

    FRAME_TIME_COLUMNS = (
        "msBetweenPresents",
        "MsBetweenPresents",
        "msBetweenDisplayChange",
        "FrameTime",
        "FrameTimeMs",
        "frame_time_ms",
    )
    PROCESS_COLUMNS = ("Application", "ProcessName", "Process", "ProcessName.exe", "app")
    PID_COLUMNS = ("ProcessID", "PID", "Pid", "process_id")

    def __init__(self, csv_path: str | Path | None = None, *, max_rows: int = 600) -> None:
        self.csv_path = Path(csv_path).expanduser() if csv_path else None
        self.max_rows = max_rows

    def snapshot_marker(self) -> int:
        if not self.csv_path or not self.csv_path.exists():
            return 0
        try:
            with self.csv_path.open("r", encoding="utf-8-sig", newline="") as handle:
                return max(sum(1 for _ in handle) - 1, 0)
        except (OSError, UnicodeDecodeError):
            return 0

    def capture(
        self,
        *,
        foreground_app: dict[str, Any] | None,
        session_mode: str,
        since_marker: int | None = None,
    ) -> dict[str, Any]:
        if not self.csv_path:
            return self._unsupported(
                "Frame-time capture requires a PresentMon-compatible CSV source configured with PRESENTMON_CSV_PATH."
            )
        if not self.csv_path.exists():
            return self._unsupported(f"Configured frame-time CSV does not exist: {self.csv_path}")
        rows = self._read_rows()
        if rows is None:
            return self._unsupported(f"PulseBoost could not read the configured frame-time CSV: {self.csv_path}")
        if not rows:
            return self._unsupported("The configured frame-time CSV is empty and did not contain any sample rows.")
        if since_marker and since_marker > 0 and len(rows) <= since_marker:
            return self._unsupported("The configured frame-time CSV did not receive fresh rows during the benchmark window.")
        samples = self._read_samples(rows=rows, foreground_app=foreground_app, since_marker=since_marker)
        if not samples:
            return self._unsupported("No recent frame-time rows matched the foreground app in the configured CSV.")
        average_ms = statistics.fmean(samples)
        p95_ms = self._percentile(samples, 95)
        fps_values = [1000.0 / value for value in samples if value > 0]
        fps_average = statistics.fmean(fps_values) if fps_values else None
        fps_1_low = self._percentile(sorted(fps_values), 1) if fps_values else None
        return {
            "supported": True,
            "status": "capturing",
            "source": "presentmon_csv",
            "path": str(self.csv_path),
            "session_mode": session_mode,
            "sample_count": len(samples),
            "current_frame_time_ms": round(samples[-1], 2),
            "average_frame_time_ms": round(average_ms, 2),
            "p95_frame_time_ms": round(p95_ms, 2) if p95_ms is not None else None,
            "frame_time_variance_ms": round(statistics.pvariance(samples), 2) if len(samples) > 1 else 0.0,
            "fps_average": round(fps_average, 1) if fps_average is not None else None,
            "fps_1_low": round(fps_1_low, 1) if fps_1_low is not None else None,
            "reason": None,
            "timestamp": time.time(),
        }

    def _read_rows(self) -> list[dict[str, str]] | None:
        try:
            with self.csv_path.open("r", encoding="utf-8-sig", newline="") as handle:
                return list(csv.DictReader(handle))
        except (OSError, csv.Error, UnicodeDecodeError):
            return None

    def _read_samples(
        self,
        *,
        rows: list[dict[str, str]],
        foreground_app: dict[str, Any] | None,
        since_marker: int | None = None,
    ) -> list[float]:
        if since_marker and since_marker > 0:
            rows = rows[since_marker:]
        if len(rows) > self.max_rows:
            rows = rows[-self.max_rows :]
        target_pid = int(foreground_app.get("pid", 0)) if foreground_app else 0
        target_name = str(foreground_app.get("name") or "").lower() if foreground_app else ""
        samples: list[float] = []
        for row in rows:
            if not self._row_matches(row, target_pid=target_pid, target_name=target_name):
                continue
            value = self._frame_time_value(row)
            if value is not None and 0.1 <= value <= 1000.0:
                samples.append(value)
        return samples

    def _row_matches(self, row: dict[str, str], *, target_pid: int, target_name: str) -> bool:
        if not target_pid and not target_name:
            return True
        for column in self.PID_COLUMNS:
            raw = row.get(column)
            if raw and target_pid:
                try:
                    if int(float(raw)) == target_pid:
                        return True
                except ValueError:
                    pass
        for column in self.PROCESS_COLUMNS:
            raw = (row.get(column) or "").lower()
            if raw and target_name and (raw == target_name or raw.endswith(target_name)):
                return True
        return False

    def _frame_time_value(self, row: dict[str, str]) -> float | None:
        for column in self.FRAME_TIME_COLUMNS:
            raw = row.get(column)
            if raw is None:
                continue
            try:
                value = float(str(raw).strip())
            except ValueError:
                continue
            if math.isfinite(value):
                return value
        return None

    def _unsupported(self, reason: str) -> dict[str, Any]:
        return {
            "supported": False,
            "status": "unavailable",
            "source": None,
            "sample_count": 0,
            "current_frame_time_ms": None,
            "average_frame_time_ms": None,
            "p95_frame_time_ms": None,
            "frame_time_variance_ms": None,
            "fps_average": None,
            "fps_1_low": None,
            "reason": reason,
            "timestamp": time.time(),
        }

    @staticmethod
    def _percentile(values: list[float], percentile: float) -> float | None:
        if not values:
            return None
        ordered = sorted(values)
        index = (len(ordered) - 1) * (percentile / 100.0)
        lower = math.floor(index)
        upper = math.ceil(index)
        if lower == upper:
            return ordered[int(index)]
        weight = index - lower
        return ordered[lower] * (1 - weight) + ordered[upper] * weight


class BottleneckAnalyzer:
    def analyze(
        self,
        *,
        snapshot: Any,
        session_mode: str,
        frame_time: dict[str, Any] | None = None,
        gpu_stats: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        cpu = self._float(getattr(snapshot, "cpu_total", 0.0))
        ram = self._float(getattr(snapshot, "ram_percent", 0.0))
        disk_io = self._float(getattr(snapshot, "disk_read_bytes", 0.0)) + self._float(getattr(snapshot, "disk_write_bytes", 0.0))
        disk_full = self._float(getattr(snapshot, "disk_percent", 0.0))
        network_io = self._float(getattr(snapshot, "net_recv_bytes", 0.0)) + self._float(getattr(snapshot, "net_sent_bytes", 0.0))
        gpu = self._float((gpu_stats or {}).get("utilization_percent"))
        gpu_temp = self._float((gpu_stats or {}).get("temperature_c"))
        cpu_temp = self._float(getattr(snapshot, "temperature", None))
        foreground = getattr(snapshot, "foreground_app", None) or {}
        background = self._background_pressure(getattr(snapshot, "top_processes", []), foreground)
        frame_p95 = self._float((frame_time or {}).get("p95_frame_time_ms"))
        frame_supported = bool((frame_time or {}).get("supported"))

        scores = {
            "CPU_BOUND": min(1.0, cpu / 95.0),
            "GPU_BOUND": min(1.0, gpu / 98.0) if gpu > 0 else 0.0,
            "RAM_BOUND": min(1.0, ram / 96.0),
            "DISK_BOUND": max(min(1.0, disk_io / (80 * 1024 * 1024)), min(1.0, disk_full / 98.0) if disk_full >= 90 else 0.0),
            "NETWORK_BOUND": min(1.0, network_io / (16 * 1024 * 1024)) if session_mode == "gaming" else 0.0,
            "THERMAL_BOUND": max(min(1.0, gpu_temp / 90.0) if gpu_temp else 0.0, min(1.0, cpu_temp / 95.0) if cpu_temp else 0.0),
            "BACKGROUND_PROCESS_BOUND": min(1.0, background["cpu_percent"] / 35.0),
        }

        if gpu_temp >= 85 or cpu_temp >= 92:
            label = "THERMAL_BOUND"
        elif background["cpu_percent"] >= 18 and cpu >= 55:
            label = "BACKGROUND_PROCESS_BOUND"
        elif ram >= 90:
            label = "RAM_BOUND"
        elif disk_io >= 50 * 1024 * 1024 or disk_full >= 96:
            label = "DISK_BOUND"
        elif gpu >= 90 or (gpu >= 82 and cpu < 82):
            label = "GPU_BOUND"
        elif cpu >= 85 or (cpu >= 74 and (gpu == 0 or gpu < 78)):
            label = "CPU_BOUND"
        elif session_mode == "gaming" and network_io >= 10 * 1024 * 1024:
            label = "NETWORK_BOUND"
        else:
            label = max(scores, key=scores.get)

        confidence = max(scores[label], 0.25)
        if frame_supported and frame_p95 >= 33.0:
            confidence = min(1.0, confidence + 0.12)
        elif not frame_supported:
            confidence = min(confidence, 0.72)

        return {
            "current_bottleneck": label,
            "confidence": round(confidence, 2),
            "inputs": {
                "cpu_percent": round(cpu, 2),
                "per_process_cpu": getattr(snapshot, "top_processes", [])[:8],
                "ram_percent": round(ram, 2),
                "disk_io_bytes_per_sec": round(disk_io, 2),
                "gpu_percent": round(gpu, 2) if gpu else None,
                "gpu_temp_c": round(gpu_temp, 2) if gpu_temp else None,
                "foreground_app": foreground or None,
                "session_mode": session_mode,
            },
            "scores": {key: round(value, 3) for key, value in scores.items()},
            "frame_time": frame_time,
            "background_process": background["process"],
            "reason": self._reason(label, cpu, ram, disk_io, gpu, gpu_temp, cpu_temp, background, frame_supported),
            "timestamp": time.time(),
        }

    def _background_pressure(self, processes: list[dict[str, Any]], foreground: dict[str, Any]) -> dict[str, Any]:
        foreground_pid = int(foreground.get("pid", 0) or 0)
        best_process: dict[str, Any] | None = None
        best_cpu = 0.0
        for process in processes:
            pid = int(process.get("pid", 0) or 0)
            if foreground_pid and pid == foreground_pid:
                continue
            cpu = self._float(process.get("cpu_percent"))
            if cpu > best_cpu:
                best_cpu = cpu
                best_process = process
        return {"cpu_percent": best_cpu, "process": best_process}

    def _reason(
        self,
        label: str,
        cpu: float,
        ram: float,
        disk_io: float,
        gpu: float,
        gpu_temp: float,
        cpu_temp: float,
        background: dict[str, Any],
        frame_supported: bool,
    ) -> str:
        if label == "THERMAL_BOUND":
            return f"Thermal pressure is highest: CPU {cpu_temp:.1f}C, GPU {gpu_temp:.1f}C."
        if label == "BACKGROUND_PROCESS_BOUND" and background["process"]:
            proc = background["process"]
            return f"Background process {proc.get('name')} is competing for {background['cpu_percent']:.1f}% CPU."
        if label == "RAM_BOUND":
            return f"RAM usage is {ram:.1f}% and may force paging or cache eviction."
        if label == "DISK_BOUND":
            return f"Disk throughput is {disk_io / (1024 * 1024):.1f} MB/s or capacity pressure is high."
        if label == "GPU_BOUND":
            return f"GPU utilization is {gpu:.1f}% and is the dominant pressure signal."
        if label == "NETWORK_BOUND":
            return "Network throughput is high during a gaming session; latency diagnostics should confirm jitter or loss."
        suffix = " Frame-time telemetry is unavailable, so confidence is capped." if not frame_supported else ""
        return f"CPU load is {cpu:.1f}% and is the strongest available pressure signal.{suffix}"

    @staticmethod
    def _float(value: Any) -> float:
        try:
            if value is None:
                return 0.0
            number = float(value)
        except (TypeError, ValueError):
            return 0.0
        return number if math.isfinite(number) else 0.0
