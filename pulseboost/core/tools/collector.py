"""
PulseBoost metric collector.
"""
from __future__ import annotations

import shutil
import sys
import time
from dataclasses import asdict, dataclass
from typing import Any

import psutil


@dataclass
class MetricSnapshot:
    timestamp: float
    cpu_total: float
    cpu_per_core: list[float]
    ram_used: int
    ram_total: int
    ram_percent: float
    swap_used: int
    disk_read_bytes: float
    disk_write_bytes: float
    disk_percent: float
    net_sent_bytes: float
    net_recv_bytes: float
    temperature: float | None
    top_processes: list[dict[str, Any]]
    process_count: int = 0
    health_score: float = 100.0
    session_mode: str = "normal"
    foreground_app: dict[str, Any] | None = None
    gpu_percent: float | None = None
    gpu_temp: float | None = None
    frametime: dict[str, Any] | None = None
    current_bottleneck: str | None = None
    bottleneck_details: dict[str, Any] | None = None

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


class MetricCollector:
    def __init__(self, disk_path: str | None = None):
        self.disk_path = disk_path or self._default_disk_path()
        self._last_disk = psutil.disk_io_counters()
        self._last_net = psutil.net_io_counters()
        self._last_time = time.time()
        psutil.cpu_percent(interval=None)
        for proc in psutil.process_iter():
            try:
                proc.cpu_percent(interval=None)
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue

    def get_snapshot(self) -> MetricSnapshot:
        now = time.time()
        elapsed = max(now - self._last_time, 0.001)

        cpu_total = psutil.cpu_percent(interval=None)
        cpu_per_core = psutil.cpu_percent(interval=None, percpu=True)
        ram = psutil.virtual_memory()
        swap = psutil.swap_memory()

        disk_now = psutil.disk_io_counters()
        disk_read = max(0.0, (disk_now.read_bytes - self._last_disk.read_bytes) / elapsed)
        disk_write = max(0.0, (disk_now.write_bytes - self._last_disk.write_bytes) / elapsed)
        self._last_disk = disk_now

        disk_percent = psutil.disk_usage(self.disk_path).percent

        net_now = psutil.net_io_counters()
        net_sent = max(0.0, (net_now.bytes_sent - self._last_net.bytes_sent) / elapsed)
        net_recv = max(0.0, (net_now.bytes_recv - self._last_net.bytes_recv) / elapsed)
        self._last_net = net_now

        top_processes, process_count = self._get_top_processes()

        snapshot = MetricSnapshot(
            timestamp=now,
            cpu_total=cpu_total,
            cpu_per_core=[float(value) for value in cpu_per_core],
            ram_used=int(ram.used),
            ram_total=int(ram.total),
            ram_percent=float(ram.percent),
            swap_used=int(swap.used),
            disk_read_bytes=disk_read,
            disk_write_bytes=disk_write,
            disk_percent=float(disk_percent),
            net_sent_bytes=net_sent,
            net_recv_bytes=net_recv,
            temperature=self._get_temperature(),
            top_processes=top_processes,
            process_count=process_count,
            foreground_app=self._get_foreground_app(),
        )
        self._last_time = now
        return snapshot

    def _default_disk_path(self) -> str:
        return shutil.disk_usage(".") and "."

    def _get_temperature(self) -> float | None:
        try:
            sensors = psutil.sensors_temperatures()
        except (AttributeError, OSError):
            return None
        for key in ("coretemp", "cpu_thermal", "k10temp", "acpitz"):
            entries = sensors.get(key)
            if entries:
                return float(entries[0].current)
        for entries in sensors.values():
            if entries:
                return float(entries[0].current)
        return None

    def _get_top_processes(self, limit: int = 16) -> tuple[list[dict[str, Any]], int]:
        processes: list[dict[str, Any]] = []
        for proc in psutil.process_iter(["pid", "name", "cpu_percent", "memory_percent", "status"]):
            try:
                info = proc.info
                memory_info = proc.memory_info()
                io_counters = None
                try:
                    io = proc.io_counters()
                    io_counters = {
                        "read_bytes": int(getattr(io, "read_bytes", 0)),
                        "write_bytes": int(getattr(io, "write_bytes", 0)),
                    }
                except (psutil.NoSuchProcess, psutil.AccessDenied, AttributeError, OSError):
                    io_counters = None
                executable_path = None
                try:
                    executable_path = proc.exe()
                except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess, OSError):
                    executable_path = None
                processes.append(
                    {
                        "pid": int(info.get("pid", 0)),
                        "name": info.get("name") or "unknown",
                        "executable_path": executable_path,
                        "cpu_percent": float(info.get("cpu_percent") or 0.0),
                        "memory_percent": round(float(info.get("memory_percent") or 0.0), 2),
                        "ram_bytes": int(getattr(memory_info, "rss", 0)),
                        "ram_mb": round(int(getattr(memory_info, "rss", 0)) / (1024 * 1024), 1),
                        "status": info.get("status") or "unknown",
                        "thread_count": int(proc.num_threads()),
                        "io_counters": io_counters,
                    }
                )
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        processes.sort(key=lambda item: (item["cpu_percent"], item["memory_percent"]), reverse=True)
        return processes[:limit], len(processes)

    def _get_foreground_app(self) -> dict[str, Any] | None:
        if not sys.platform.startswith("win"):
            return None
        try:
            import ctypes
            from ctypes import wintypes

            user32 = ctypes.windll.user32
            hwnd = user32.GetForegroundWindow()
            if not hwnd:
                return None
            pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            title_length = user32.GetWindowTextLengthW(hwnd)
            title_buffer = ctypes.create_unicode_buffer(title_length + 1)
            user32.GetWindowTextW(hwnd, title_buffer, title_length + 1)
            proc = psutil.Process(int(pid.value))
            return {
                "pid": int(pid.value),
                "name": proc.name(),
                "executable_path": proc.exe(),
                "title": title_buffer.value,
            }
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess, OSError, AttributeError):
            return None
