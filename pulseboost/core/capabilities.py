from __future__ import annotations

import psutil
import shutil
from pathlib import Path

from core.database import DatabaseService
from core.models import CapabilitySnapshot, HardwareProfile
from core.platform_info import PlatformInfoService


class CapabilityService:
    def __init__(self, database: DatabaseService, platform_info: PlatformInfoService) -> None:
        self.database = database
        self.platform_info = platform_info
        self.last_snapshot: CapabilitySnapshot | None = None
        self.last_profile = None

    async def refresh(self) -> CapabilitySnapshot:
        profile = self.platform_info.collect()
        is_windows = profile.os_name.lower() == "windows"
        is_admin = self.platform_info.is_admin()
        has_powershell = bool(shutil.which("powershell") or shutil.which("pwsh"))
        has_wmi = self.platform_info.has_module("wmi")
        supports_webview_runtime = self.platform_info.has_module("webview")
        supports_electron_runtime = self._electron_runtime_available(profile)
        has_gpu_runtime = self.platform_info.gpu_runtime_available(profile)

        try:
            has_temperature_sensors = bool(psutil.sensors_temperatures())
        except Exception:  # pragma: no cover
            has_temperature_sensors = False

        can_control_affinity = hasattr(psutil.Process(), "cpu_affinity")
        can_control_process_priority = True

        notes: list[str] = []
        if not is_windows:
            notes.append("Non-Windows platform detected; Windows optimization modules remain unavailable.")
        if not is_admin:
            notes.append("Admin privileges not detected; registry/service actions must stay gated.")
        if not supports_electron_runtime:
            notes.append("Electron desktop runtime is not installed for this machine.")
        elif profile.desktop_runtime != "electron":
            notes.append("Current desktop runtime is not Electron; Electron launch remains optional until the desktop shell is installed on this machine.")
        if profile.gpu_vendor and not has_gpu_runtime:
            notes.append(f"{profile.gpu_vendor} GPU detected, but no supported vendor runtime tool is available to PulseBoost.")

        snapshot = CapabilitySnapshot(
            machine_id=profile.machine_id,
            captured_at=profile.captured_at,
            os_name=profile.os_name,
            os_version=profile.os_version,
            is_windows=is_windows,
            is_admin=is_admin,
            desktop_runtime=profile.desktop_runtime,
            python_version=self.platform_info.python_version(),
            has_powershell=has_powershell,
            has_wmi=has_wmi,
            has_temperature_sensors=has_temperature_sensors,
            has_gpu_runtime=has_gpu_runtime,
            supports_electron_runtime=supports_electron_runtime,
            supports_webview_runtime=supports_webview_runtime,
            can_manage_services=is_windows and has_powershell and is_admin,
            can_edit_registry=is_windows and is_admin,
            can_control_process_priority=can_control_process_priority,
            can_control_affinity=can_control_affinity,
            notes=notes,
        )

        profile.supported_capabilities = [
            name
            for name, enabled in {
                "services": snapshot.can_manage_services,
                "registry": snapshot.can_edit_registry,
                "process_priority": snapshot.can_control_process_priority,
                "process_affinity": snapshot.can_control_affinity,
                "temperature": snapshot.has_temperature_sensors,
                "gpu_runtime": snapshot.has_gpu_runtime,
                "wmi": snapshot.has_wmi,
            }.items()
            if enabled
        ]

        await self.database.save_hardware_profile(profile)
        await self.database.save_capability_snapshot(snapshot)
        self.last_profile = profile
        self.last_snapshot = snapshot
        return snapshot

    def _electron_runtime_available(self, profile: HardwareProfile) -> bool:
        if profile.desktop_runtime == "electron":
            return True

        pulseboost_root = Path(__file__).resolve().parents[1]
        bundled_candidates = (
            pulseboost_root / "ui" / "node_modules" / ".bin" / "electron.cmd",
            pulseboost_root / "ui" / "node_modules" / "electron" / "dist" / "electron.exe",
        )
        if any(candidate.exists() for candidate in bundled_candidates):
            return True

        return bool(shutil.which("electron"))
