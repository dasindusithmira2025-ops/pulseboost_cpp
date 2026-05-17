from __future__ import annotations

import platform
import subprocess

from config import get_settings
from core.audit_log import AuditLog
from core.models import CapabilitySnapshot
from core.revert_manager import RevertManager


START_TYPE_MAP = {
    "auto": "auto",
    "automatic": "auto",
    "manual": "demand",
    "demand": "demand",
    "disabled": "disabled",
}

ALLOWED_SERVICES = {"SysMain", "WSearch"}


class SafeServiceWrapper:
    def __init__(
        self,
        *,
        audit_log: AuditLog | None,
        revert_manager: RevertManager | None,
        capabilities: CapabilitySnapshot,
    ) -> None:
        self.audit_log = audit_log
        self.revert_manager = revert_manager
        self.capabilities = capabilities
        self.settings = get_settings()

    def _supported(self) -> bool:
        return platform.system() == "Windows" and self.capabilities.can_manage_services

    def get_start_type(self, service_name: str) -> str | None:
        if not self._supported():
            return None
        result = subprocess.run(["sc", "qc", service_name], capture_output=True, text=True, check=False)
        output = result.stdout or ""
        for line in output.splitlines():
            if "START_TYPE" in line:
                parts = line.split(":", 1)[-1].strip().split()
                if parts:
                    code = parts[0]
                    return {
                        "2": "auto",
                        "3": "demand",
                        "4": "disabled",
                    }.get(code, parts[-1].lower())
        return None

    async def set_start_type(
        self,
        service_name: str,
        start_type: str,
        *,
        triggered_by: str = "optimizer",
        reason: str = "Applied a validated service tweak.",
    ) -> dict:
        if service_name not in ALLOWED_SERVICES:
            return {"success": False, "error": f"{service_name} is not in the validated service allow-list."}
        if not self._supported():
            return {"success": False, "unsupported": True, "error": "Service control is not supported on this machine."}
        desired = START_TYPE_MAP.get(start_type.lower())
        if not desired:
            return {"success": False, "error": f"Unsupported start type: {start_type}"}

        before = self.get_start_type(service_name)
        snapshot_id = None
        if self.revert_manager:
            snapshot_id = await self.revert_manager.capture_snapshot(
                target_type="service_start_type",
                target_id=service_name,
                before_value={"service_name": service_name, "start_type": before},
                metadata={"reason": reason},
            )

        if self.settings.executor_dry_run:
            result = {"success": True, "dry_run": True, "service_name": service_name, "start_type": desired, "revert_snapshot_id": snapshot_id}
        else:
            completed = subprocess.run(["sc", "config", service_name, "start=", desired], capture_output=True, text=True, check=False)
            success = completed.returncode == 0
            result = {
                "success": success,
                "dry_run": False,
                "service_name": service_name,
                "start_type": desired,
                "stdout": completed.stdout,
                "stderr": completed.stderr,
                "revert_snapshot_id": snapshot_id,
            }

        if self.audit_log:
            await self.audit_log.record_event(
                module="services",
                action="set_start_type",
                target=service_name,
                before_value={"start_type": before},
                after_value=result,
                rationale=reason,
                validity_tag="VALIDATED",
                triggered_by=triggered_by,
                status="success" if result.get("success") else "failed",
            )
        return result

    async def restore(self, snapshot_id: str, *, triggered_by: str = "optimizer_restore") -> dict:
        if not self.revert_manager:
            return {"success": False, "error": "No revert manager configured."}
        snapshot = await self.revert_manager.database.get_revert_snapshot(snapshot_id)
        if not snapshot:
            return {"success": False, "error": "Revert snapshot not found."}
        before = snapshot["before_value"]
        start_type = before.get("start_type") or "demand"
        if self.settings.executor_dry_run:
            await self.revert_manager.mark_restored(snapshot_id, status="restored_dry_run")
            result = {"success": True, "dry_run": True, "service_name": before["service_name"], "start_type": start_type}
        else:
            completed = subprocess.run(["sc", "config", before["service_name"], "start=", start_type], capture_output=True, text=True, check=False)
            await self.revert_manager.mark_restored(snapshot_id, status="restored" if completed.returncode == 0 else "restore_failed")
            result = {
                "success": completed.returncode == 0,
                "dry_run": False,
                "service_name": before["service_name"],
                "start_type": start_type,
                "stdout": completed.stdout,
                "stderr": completed.stderr,
            }
        if self.audit_log:
            await self.audit_log.record_event(
                module="services",
                action="restore_start_type",
                target=before["service_name"],
                before_value={"snapshot_id": snapshot_id},
                after_value=result,
                rationale="Restored service start type to the previous value.",
                validity_tag="VALIDATED",
                triggered_by=triggered_by,
                status="success" if result.get("success") else "failed",
            )
        return result
