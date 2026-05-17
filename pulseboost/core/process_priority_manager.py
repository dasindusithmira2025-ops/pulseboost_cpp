from __future__ import annotations

import platform
from typing import Any

import psutil

from config import get_settings
from core.audit_log import AuditLog
from core.models import CapabilitySnapshot
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard


WINDOWS_PRIORITIES = {
    "below_normal": psutil.BELOW_NORMAL_PRIORITY_CLASS,
    "normal": psutil.NORMAL_PRIORITY_CLASS,
}


class ProcessPriorityManager:
    def __init__(
        self,
        *,
        audit_log: AuditLog | None,
        revert_manager: RevertManager | None,
        safety_guard: SafetyGuard,
        capabilities: CapabilitySnapshot,
    ) -> None:
        self.audit_log = audit_log
        self.revert_manager = revert_manager
        self.safety_guard = safety_guard
        self.capabilities = capabilities
        self.settings = get_settings()

    def get_priority(self, pid: int) -> Any:
        try:
            return psutil.Process(pid).nice()
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return None

    async def set_priority(
        self,
        pid: int,
        name: str,
        priority: str,
        *,
        triggered_by: str = "optimizer",
        reason: str = "Applied a validated process priority tweak.",
    ) -> dict:
        if not self.capabilities.can_control_process_priority:
            return {"success": False, "unsupported": True, "error": "Process priority control is unavailable."}
        self.safety_guard.assert_process_action("reduce_process_priority", pid, name)
        if not psutil.pid_exists(pid):
            return {"success": False, "blocked": True, "error": "Target process is no longer running."}

        before = {"pid": pid, "name": name, "priority": self.get_priority(pid)}
        if before["priority"] is None:
            return {"success": False, "blocked": True, "error": "Process priority could not be read for the selected target."}
        snapshot_id = None
        if self.revert_manager:
            snapshot_id = await self.revert_manager.capture_snapshot(
                target_type="process_priority",
                target_id=f"{pid}:{name}",
                before_value=before,
                metadata={"reason": reason},
            )

        if self.settings.executor_dry_run:
            result = {"success": True, "dry_run": True, "pid": pid, "name": name, "priority": priority, "revert_snapshot_id": snapshot_id}
        else:
            try:
                process = psutil.Process(pid)
                if platform.system() == "Windows":
                    process.nice(WINDOWS_PRIORITIES[priority])
                else:
                    process.nice(10 if priority == "below_normal" else 0)
            except psutil.NoSuchProcess:
                return {"success": False, "blocked": True, "error": "Target process ended before priority could be applied."}
            except psutil.AccessDenied:
                return {"success": False, "blocked": True, "error": "Access denied while applying process priority."}
            except (KeyError, ValueError, psutil.Error):
                return {"success": False, "blocked": True, "error": "Priority request was rejected for the selected process."}
            result = {"success": True, "dry_run": False, "pid": pid, "name": name, "priority": priority, "revert_snapshot_id": snapshot_id}

        if self.audit_log:
            await self.audit_log.record_event(
                module="process_priority",
                action="set_priority",
                target=f"{name}:{pid}",
                before_value=before,
                after_value=result,
                rationale=reason,
                validity_tag="VALIDATED",
                triggered_by=triggered_by,
                status="success",
            )
        return result

    async def restore(self, snapshot_id: str, *, triggered_by: str = "optimizer_restore") -> dict:
        if not self.revert_manager:
            return {"success": False, "error": "No revert manager configured."}
        snapshot = await self.revert_manager.database.get_revert_snapshot(snapshot_id)
        if not snapshot:
            return {"success": False, "error": "Revert snapshot not found."}
        before = snapshot["before_value"]
        if self.settings.executor_dry_run:
            await self.revert_manager.mark_restored(snapshot_id, status="restored_dry_run")
            result = {"success": True, "dry_run": True, "pid": before["pid"], "name": before["name"], "priority": before["priority"]}
        else:
            try:
                psutil.Process(before["pid"]).nice(before["priority"])
            except psutil.NoSuchProcess:
                await self.revert_manager.mark_restored(snapshot_id, status="target_missing")
                return {"success": False, "blocked": True, "error": "Target process ended before priority restore."}
            except psutil.AccessDenied:
                await self.revert_manager.mark_restored(snapshot_id, status="access_denied")
                return {"success": False, "blocked": True, "error": "Access denied while restoring process priority."}
            except (ValueError, psutil.Error):
                await self.revert_manager.mark_restored(snapshot_id, status="restore_failed")
                return {"success": False, "blocked": True, "error": "Priority restore was rejected for the selected process."}
            await self.revert_manager.mark_restored(snapshot_id, status="restored")
            result = {"success": True, "dry_run": False, "pid": before["pid"], "name": before["name"], "priority": before["priority"]}
        if self.audit_log:
            await self.audit_log.record_event(
                module="process_priority",
                action="restore_priority",
                target=f"{before['name']}:{before['pid']}",
                before_value={"snapshot_id": snapshot_id},
                after_value=result,
                rationale="Restored process priority to the previous value.",
                validity_tag="VALIDATED",
                triggered_by=triggered_by,
                status="success",
            )
        return result
