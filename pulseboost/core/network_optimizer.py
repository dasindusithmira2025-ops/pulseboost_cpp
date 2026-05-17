from __future__ import annotations

import asyncio
import socket
import statistics
import time
from typing import Any

import psutil

from core.audit_log import AuditLog
from core.capabilities import CapabilitySnapshot
from core.database import DatabaseService


class NetworkOptimizer:
    PUBLIC_TARGET = {"label": "Public target", "host": "1.1.1.1", "port": 443}

    def __init__(self, *, database: DatabaseService, audit_log: AuditLog, capabilities: CapabilitySnapshot) -> None:
        self.database = database
        self.audit_log = audit_log
        self.capabilities = capabilities

    async def diagnostics(
        self,
        *,
        session_mode: str = "normal",
        active_session: dict[str, Any] | None = None,
        probe_attempts: int | None = None,
        probe_timeout_seconds: float | None = None,
    ) -> dict[str, Any]:
        nic_capabilities = self._nic_capabilities()
        protocol_profile = self._protocol_profile(session_mode, active_session)
        resolved_attempts = probe_attempts
        resolved_timeout = probe_timeout_seconds
        if resolved_attempts is None:
            resolved_attempts = 1 if session_mode == "benchmark" else 3
        if resolved_timeout is None:
            resolved_timeout = 0.6 if session_mode == "benchmark" else 1.5
        router_probe = self._unsupported_probe("Router target detection is unavailable without a discovered gateway path.")
        public_probe = await self._probe_target(
            self.PUBLIC_TARGET["host"],
            self.PUBLIC_TARGET["port"],
            self.PUBLIC_TARGET["label"],
            attempts=max(1, int(resolved_attempts)),
            timeout_seconds=max(0.1, float(resolved_timeout)),
        )
        game_probe = self._unsupported_probe("Game server ping is unavailable until a concrete active server endpoint is detected.")

        summary_source = public_probe if public_probe["supported"] else None
        summary = {
            "latency_ms": summary_source.get("latency_ms") if summary_source else None,
            "jitter_ms": summary_source.get("jitter_ms") if summary_source else None,
            "packet_loss_percent": summary_source.get("packet_loss_percent") if summary_source else None,
            "bufferbloat_grade": self._bufferbloat_grade(summary_source.get("latency_ms"), summary_source.get("jitter_ms")) if summary_source else "UNSUPPORTED",
        }
        payload = {
            "timestamp": time.time(),
            "session_id": (active_session or {}).get("id"),
            "protocol_profile": protocol_profile,
            "nic_capabilities": nic_capabilities,
            "targets": {
                "router": router_probe,
                "public": public_probe,
                "game_server": game_probe,
            },
            "summary": summary,
            "qos": {
                "supported": self.capabilities.is_windows and self.capabilities.is_admin and self.capabilities.has_powershell,
                "mode": "dry_run" if self.capabilities.is_windows else "unsupported",
                "note": "QoS policy application is dry-run only until an OS-backed safe writer is confirmed for this machine."
                if self.capabilities.is_windows
                else "QoS policy application is unavailable on this machine/runtime.",
            },
        }
        await self.database.insert_network_diagnostics(payload)
        return payload

    async def latest_diagnostics(self) -> dict[str, Any] | None:
        return await self.database.latest_network_diagnostics()

    async def apply_qos_profile(self, profile_name: str, *, protocol: str, triggered_by: str = "user") -> dict[str, Any]:
        supported = self.capabilities.is_windows and self.capabilities.is_admin and self.capabilities.has_powershell
        result = {
            "success": supported,
            "supported": supported,
            "applied": False,
            "dry_run": supported,
            "profile_name": profile_name,
            "protocol": protocol,
            "reason": "Dry-run only: QoS support surface is present but real policy writes are not yet enabled for this machine."
            if supported
            else "QoS policy application is unsupported on this machine/runtime.",
        }
        await self.audit_log.record_event(
            module="network_optimizer",
            action="apply_qos_profile",
            target=profile_name,
            before_value={"protocol": protocol},
            after_value=result,
            rationale="Evaluated a protocol-aware QoS profile request through the safe network capability gate.",
            validity_tag="VALIDATED",
            triggered_by=triggered_by,
            status="dry_run" if supported else "blocked",
        )
        return result

    def _protocol_profile(self, session_mode: str, active_session: dict[str, Any] | None) -> dict[str, Any]:
        if active_session and active_session.get("game_name"):
            return {
                "detected_transport": "udp",
                "confidence": 0.8,
                "reasoning": "Active game sessions are treated as latency-sensitive and primarily UDP-oriented by default.",
            }
        if session_mode in {"gaming", "heavy_compute"}:
            return {
                "detected_transport": "udp",
                "confidence": 0.65,
                "reasoning": "Current session pattern looks latency-sensitive, so UDP-oriented tuning is preferred.",
            }
        if session_mode == "development":
            return {
                "detected_transport": "mixed",
                "confidence": 0.55,
                "reasoning": "Development sessions usually mix TCP-heavy downloads with UDP tools like remote debugging and calls.",
            }
        return {
            "detected_transport": "tcp",
            "confidence": 0.6,
            "reasoning": "General desktop traffic is treated as TCP-biased until an active game/server flow is known.",
        }

    def _nic_capabilities(self) -> list[dict[str, Any]]:
        stats = psutil.net_if_stats()
        addresses = psutil.net_if_addrs()
        nic_rows = []
        for name, info in stats.items():
            addr_rows = addresses.get(name, [])
            nic_rows.append(
                {
                    "name": name,
                    "is_up": bool(info.isup),
                    "speed_mbps": getattr(info, "speed", 0),
                    "mtu": getattr(info, "mtu", 0),
                    "duplex": getattr(info, "duplex", None),
                    "addresses": [getattr(item, "address", "") for item in addr_rows],
                    "qos_supported": self.capabilities.is_windows and self.capabilities.is_admin and self.capabilities.has_powershell,
                    "advanced_tuning_supported": self.capabilities.is_windows,
                }
            )
        return nic_rows

    @staticmethod
    def _probe_once(host: str, port: int, timeout_seconds: float) -> float | None:
        started = time.perf_counter()
        try:
            with socket.create_connection((host, port), timeout=timeout_seconds):
                return (time.perf_counter() - started) * 1000
        except OSError:
            return None

    async def _probe_target(
        self,
        host: str,
        port: int,
        label: str,
        *,
        attempts: int = 3,
        timeout_seconds: float = 1.5,
    ) -> dict[str, Any]:
        latencies = []
        failures = 0
        for _ in range(attempts):
            try:
                latency = await asyncio.wait_for(
                    asyncio.to_thread(self._probe_once, host, port, timeout_seconds),
                    timeout=max(0.1, timeout_seconds + 0.25),
                )
            except asyncio.TimeoutError:
                failures += 1
                await asyncio.sleep(0)
                continue
            if latency is None:
                failures += 1
            else:
                latencies.append(latency)
            await asyncio.sleep(0)
        if not latencies:
            return self._unsupported_probe(f"{label} probe failed on this machine/runtime.")
        jitter = statistics.pstdev(latencies) if len(latencies) > 1 else 0.0
        return {
            "label": label,
            "supported": True,
            "latency_ms": round(sum(latencies) / len(latencies), 2),
            "jitter_ms": round(jitter, 2),
            "packet_loss_percent": round((failures / attempts) * 100, 2),
            "attempts": attempts,
            "transport": "tcp_connect",
        }

    def _unsupported_probe(self, reason: str) -> dict[str, Any]:
        return {
            "supported": False,
            "latency_ms": None,
            "jitter_ms": None,
            "packet_loss_percent": None,
            "reason": reason,
        }

    def _bufferbloat_grade(self, latency_ms: float | None, jitter_ms: float | None) -> str:
        if latency_ms is None or jitter_ms is None:
            return "UNSUPPORTED"
        score = latency_ms + (jitter_ms * 2)
        if score <= 25:
            return "A"
        if score <= 50:
            return "B"
        if score <= 90:
            return "C"
        return "D"
