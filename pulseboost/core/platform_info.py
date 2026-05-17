from __future__ import annotations

import importlib.util
import platform
import shutil
import subprocess
import sys
import time
from typing import Any

import psutil

from core.models import HardwareProfile

try:
    from ctypes import windll
except ImportError:  # pragma: no cover
    windll = None

try:
    from cpuinfo import get_cpu_info
except ImportError:  # pragma: no cover
    get_cpu_info = None


class PlatformInfoService:
    def __init__(self, machine_id: str, machine_name: str, desktop_runtime: str = "webview") -> None:
        self.machine_id = machine_id
        self.machine_name = machine_name
        self.desktop_runtime = desktop_runtime

    def collect(self) -> HardwareProfile:
        cpu_name = platform.processor() or "Unknown CPU"
        if get_cpu_info:
            try:
                info = get_cpu_info()
                cpu_name = info.get("brand_raw") or cpu_name
            except Exception:  # pragma: no cover
                pass

        nic_stats = psutil.net_if_stats()
        nic_details: list[dict[str, Any]] = []
        for name, addresses in psutil.net_if_addrs().items():
            nic_details.append(
                {
                    "name": name,
                    "address_count": len(addresses),
                    "is_up": bool(nic_stats.get(name).isup) if name in nic_stats else False,
                }
            )

        bios_notes = []
        if platform.system() != "Windows":
            bios_notes.append("Windows-specific BIOS advisory is unavailable on this platform.")
        gpu_details = self._detect_gpu_adapter()

        return HardwareProfile(
            machine_id=self.machine_id,
            machine_name=self.machine_name,
            captured_at=time.time(),
            os_name=platform.system(),
            os_version=platform.version(),
            os_build=platform.release(),
            cpu_name=cpu_name,
            cpu_logical_cores=psutil.cpu_count(logical=True) or 0,
            cpu_physical_cores=psutil.cpu_count(logical=False),
            ram_total_bytes=psutil.virtual_memory().total,
            gpu_vendor=gpu_details.get("vendor"),
            gpu_model=gpu_details.get("model"),
            motherboard=None,
            bios_advisory=bios_notes,
            nic_details=nic_details,
            supported_capabilities=[],
            desktop_runtime=self.desktop_runtime,
        )

    @staticmethod
    def is_admin() -> bool:
        if platform.system() != "Windows" or windll is None:
            return False
        try:
            return bool(windll.shell32.IsUserAnAdmin())
        except Exception:  # pragma: no cover
            return False

    @staticmethod
    def python_version() -> str:
        return sys.version.split()[0]

    @staticmethod
    def has_module(module_name: str) -> bool:
        return importlib.util.find_spec(module_name) is not None

    @staticmethod
    def gpu_runtime_available(profile: HardwareProfile) -> bool:
        vendor = (profile.gpu_vendor or "").lower()
        if "nvidia" in vendor:
            return bool(shutil.which("nvidia-smi"))
        if "amd" in vendor or "advanced micro devices" in vendor or "radeon" in vendor:
            return bool(shutil.which("amd-smi") or shutil.which("rocm-smi"))
        return False

    @staticmethod
    def _detect_gpu_adapter() -> dict[str, str | None]:
        if platform.system() != "Windows":
            return {"vendor": None, "model": None}

        commands = [tool for tool in ("powershell", "pwsh") if shutil.which(tool)]
        for command in commands:
            try:
                result = subprocess.run(
                    [
                        command,
                        "-NoProfile",
                        "-Command",
                        "Get-CimInstance Win32_VideoController | Select-Object Name,AdapterCompatibility | ConvertTo-Json -Compress",
                    ],
                    capture_output=True,
                    check=True,
                    text=True,
                    timeout=5,
                )
            except Exception:  # pragma: no cover
                continue

            payload = result.stdout.strip()
            if not payload:
                continue

            try:
                import json

                items = json.loads(payload)
            except Exception:  # pragma: no cover
                continue

            if isinstance(items, dict):
                items = [items]

            preferred = None
            for item in items:
                vendor = str(item.get("AdapterCompatibility") or "").strip()
                model = str(item.get("Name") or "").strip()
                if not model:
                    continue
                candidate = {"vendor": vendor or None, "model": model}
                vendor_lower = vendor.lower()
                model_lower = model.lower()
                if "nvidia" in vendor_lower or "amd" in vendor_lower or "radeon" in model_lower:
                    return candidate
                if preferred is None:
                    preferred = candidate

            if preferred:
                return preferred

        return {"vendor": None, "model": None}
