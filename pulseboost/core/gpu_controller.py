from __future__ import annotations

import subprocess
import time
from typing import Any

from core.audit_log import AuditLog
from core.capabilities import CapabilitySnapshot
from core.database import DatabaseService


class GPUController:
    SETTINGS_STATE_KEY = "gpu.settings_state"

    def __init__(
        self,
        *,
        database: DatabaseService,
        audit_log: AuditLog,
        capabilities: CapabilitySnapshot,
        hardware_profile: dict[str, Any] | None,
    ) -> None:
        self.database = database
        self.audit_log = audit_log
        self.capabilities = capabilities
        self.hardware_profile = hardware_profile or {}

    async def stats(self) -> dict[str, Any]:
        vendor = self._vendor()
        model = self.hardware_profile.get("gpu_model")
        supported = bool(self.capabilities.has_gpu_runtime and vendor in {"nvidia", "amd"})
        base_stats = {
            "timestamp": time.time(),
            "vendor": vendor,
            "model": model,
            "driver_version": None,
            "clock_mhz": None,
            "utilization_percent": None,
            "temperature_c": None,
            "memory_used_mb": None,
            "power_watts": None,
            "settings": self._settings_catalog(vendor),
        }
        if not supported:
            unsupported_payload = {
                **base_stats,
                "telemetry_supported": False,
                "reason": self._unsupported_reason(vendor),
            }
            await self.database.insert_gpu_snapshot(unsupported_payload)
            return unsupported_payload
        if vendor == "nvidia":
            live = self._read_nvidia_telemetry()
            if live:
                supported_payload = {
                    **base_stats,
                    **live,
                    "vendor": "nvidia",
                    "model": live.get("model") or model,
                    "telemetry_supported": True,
                    "reason": None,
                }
                await self.database.insert_gpu_snapshot(supported_payload)
                return supported_payload
            unsupported_payload = {
                **base_stats,
                "telemetry_supported": False,
                "reason": "NVIDIA GPU detected, but live telemetry could not be read from nvidia-smi.",
            }
            await self.database.insert_gpu_snapshot(unsupported_payload)
            return unsupported_payload
        unsupported_payload = {
            **base_stats,
            "telemetry_supported": False,
            "reason": self._unsupported_reason(vendor),
        }
        await self.database.insert_gpu_snapshot(unsupported_payload)
        return unsupported_payload

    async def bios_checklist(self) -> dict[str, Any]:
        vendor = self._vendor()
        items = [
            {
                "id": "resizable_bar",
                "title": "Resizable BAR / Smart Access Memory",
                "status": "advisory",
                "recommended": vendor in {"nvidia", "amd"},
                "reason": "Enable this in BIOS if your motherboard and GPU support it for modern game workloads.",
            },
            {
                "id": "above_4g_decoding",
                "title": "Above 4G Decoding",
                "status": "advisory",
                "recommended": vendor in {"nvidia", "amd"},
                "reason": "Often required for Resizable BAR and modern PCIe GPU mappings.",
            },
            {
                "id": "bios_update",
                "title": "BIOS currency",
                "status": "advisory",
                "recommended": True,
                "reason": "Keep BIOS current for stability, AGESA/microcode fixes, and GPU compatibility improvements.",
            },
            {
                "id": "xmp_expo",
                "title": "XMP / EXPO memory profile",
                "status": "advisory",
                "recommended": True,
                "reason": "Memory running below rated speed can reduce frame consistency and 1% lows.",
            },
        ]
        return {
            "timestamp": time.time(),
            "vendor": vendor,
            "model": self.hardware_profile.get("gpu_model"),
            "items": items,
        }

    async def apply_setting(self, setting_id: str, value: Any, *, confirm_risky: bool = False, triggered_by: str = "user") -> dict[str, Any]:
        vendor = self._vendor()
        catalog = {item["id"]: item for item in self._settings_catalog(vendor)}
        setting = catalog.get(setting_id)
        if not setting:
            return {"success": False, "supported": False, "reason": "Unknown GPU setting."}
        if setting["mode"] == "unsupported":
            return {"success": False, "supported": False, "reason": setting["reason"], "setting": setting}
        if setting["risk"] == "moderate" and not confirm_risky:
            return {
                "success": False,
                "supported": True,
                "blocked": True,
                "reason": "Explicit confirmation is required for this GPU setting.",
                "setting": setting,
            }

        settings_state = await self.database.get_setting(self.SETTINGS_STATE_KEY) or {"items": {}}
        items = dict(settings_state.get("items", {}))
        items[setting_id] = {"value": value, "saved_at": time.time(), "dry_run": True}
        await self.database.set_setting(self.SETTINGS_STATE_KEY, {"items": items}, source="gpu_controller")
        result = {
            "success": True,
            "supported": True,
            "applied": False,
            "dry_run": True,
            "setting_id": setting_id,
            "value": value,
            "setting": setting,
            "reason": "The setting surface is supported, but writes remain dry-run until a production-safe GPU/registry writer is enabled.",
        }
        await self.audit_log.record_event(
            module="gpu_controller",
            action="apply_setting",
            target=setting_id,
            before_value=None,
            after_value=result,
            rationale="Evaluated a safe GPU settings request through the vendor-aware capability gate.",
            validity_tag="VALIDATED",
            triggered_by=triggered_by,
            status="dry_run",
        )
        return result

    def _settings_catalog(self, vendor: str) -> list[dict[str, Any]]:
        settings = [
            {
                "id": "hardware_accelerated_gpu_scheduling",
                "label": "Hardware-Accelerated GPU Scheduling",
                "scope": "generic_registry",
                "risk": "safe",
                "mode": "dry_run_supported" if self.capabilities.can_edit_registry and self.capabilities.is_windows else "unsupported",
                "reason": (
                    "Hardware-Accelerated GPU Scheduling requires Windows registry access with administrator privileges on this machine."
                    if not (self.capabilities.can_edit_registry and self.capabilities.is_windows)
                    else "Safe generic graphics setting surface is available in dry-run mode."
                ),
            },
            {
                "id": "nvidia_low_latency_mode",
                "label": "NVIDIA Low Latency Mode",
                "scope": "vendor_runtime",
                "risk": "moderate",
                "mode": "unsupported" if vendor != "nvidia" or not self.capabilities.has_gpu_runtime else "dry_run_supported",
                "reason": (
                    "NVIDIA runtime integration is available in dry-run mode for this machine."
                    if vendor == "nvidia" and self.capabilities.has_gpu_runtime
                    else "NVIDIA runtime integration is unavailable for this machine/runtime."
                ),
            },
            {
                "id": "amd_anti_lag",
                "label": "AMD Anti-Lag",
                "scope": "vendor_runtime",
                "risk": "moderate",
                "mode": "unsupported" if vendor != "amd" or not self.capabilities.has_gpu_runtime else "dry_run_supported",
                "reason": (
                    "AMD runtime integration is available in dry-run mode for this machine."
                    if vendor == "amd" and self.capabilities.has_gpu_runtime
                    else "AMD runtime integration is unavailable for this machine/runtime."
                ),
            },
        ]
        for item in settings:
            item["supported"] = item["mode"] != "unsupported"
        return settings

    def _vendor(self) -> str:
        vendor = (self.hardware_profile.get("gpu_vendor") or "unknown").lower()
        if "nvidia" in vendor:
            return "nvidia"
        if "amd" in vendor or "advanced micro devices" in vendor or "radeon" in vendor:
            return "amd"
        return "unsupported"

    def _unsupported_reason(self, vendor: str) -> str:
        if vendor == "unsupported":
            return "Live GPU telemetry is unavailable because no supported NVIDIA or AMD adapter was detected from Windows display adapters."
        if vendor == "nvidia":
            return "NVIDIA GPU detected, but PulseBoost could not find the NVIDIA runtime tool nvidia-smi."
        if vendor == "amd":
            return "AMD GPU detected, but PulseBoost could not find a supported AMD runtime tool."
        return "Live GPU telemetry is unavailable because no supported vendor runtime is active on this machine."

    @staticmethod
    def _parse_number(value: str) -> float | int | None:
        cleaned = str(value).strip()
        if not cleaned or cleaned in {"[Not Supported]", "N/A"}:
            return None
        try:
            number = float(cleaned)
        except ValueError:
            return None
        if number.is_integer():
            return int(number)
        return round(number, 2)

    def _read_nvidia_telemetry(self) -> dict[str, Any] | None:
        try:
            result = subprocess.run(
                [
                    "nvidia-smi",
                    "--query-gpu=name,driver_version,utilization.gpu,temperature.gpu,memory.used,power.draw,clocks.current.graphics",
                    "--format=csv,noheader,nounits",
                ],
                capture_output=True,
                check=True,
                text=True,
                timeout=5,
            )
        except Exception:  # pragma: no cover
            return None

        line = next((item.strip() for item in result.stdout.splitlines() if item.strip()), "")
        if not line:
            return None
        parts = [item.strip() for item in line.split(",")]
        if len(parts) < 7:
            return None
        return {
            "model": parts[0] or None,
            "driver_version": parts[1] or None,
            "utilization_percent": self._parse_number(parts[2]),
            "temperature_c": self._parse_number(parts[3]),
            "memory_used_mb": self._parse_number(parts[4]),
            "power_watts": self._parse_number(parts[5]),
            "clock_mhz": self._parse_number(parts[6]),
        }
