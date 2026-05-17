from __future__ import annotations

import platform

from core.models import CapabilitySnapshot, CompatibilityResult


class CompatibilityService:
    def evaluate_action(self, action_type: str, capabilities: CapabilitySnapshot) -> CompatibilityResult:
        reasons: list[str] = []
        warnings: list[str] = []

        if action_type in {"reduce_process_priority", "kill_process"} and not capabilities.can_control_process_priority:
            reasons.append("Process priority control is unavailable on this machine.")

        if action_type == "drop_ram_cache" and platform.system() != "Linux":
            reasons.append("drop_ram_cache is Linux only.")

        if action_type in {"manage_service", "restart_service"} and not capabilities.can_manage_services:
            reasons.append("Service control requires Windows admin plus PowerShell capability.")

        if action_type in {"write_registry", "set_registry_value"} and not capabilities.can_edit_registry:
            reasons.append("Registry editing requires Windows admin capability.")

        if action_type == "flush_dns_cache" and capabilities.is_windows and not capabilities.has_powershell:
            warnings.append("Windows PowerShell is unavailable; DNS flushing may fail depending on system path state.")

        return CompatibilityResult(
            supported=not reasons,
            reasons=reasons,
            warnings=warnings,
        )
