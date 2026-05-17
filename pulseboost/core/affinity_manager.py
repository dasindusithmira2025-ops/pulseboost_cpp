from __future__ import annotations

import psutil

from config import get_settings
from core.audit_log import AuditLog
from core.models import CapabilitySnapshot
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard


class AffinityManager:
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

    def get_affinity(self, pid: int) -> list[int] | None:
        try:
            process = psutil.Process(pid)
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return None
        if not hasattr(process, "cpu_affinity"):
            return None
        try:
            return list(process.cpu_affinity())
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return None

    async def set_affinity(
        self,
        pid: int,
        name: str,
        cores: list[int],
        *,
        triggered_by: str = "optimizer",
        reason: str = "Applied a validated CPU affinity tweak.",
    ) -> dict:
        if not self.capabilities.can_control_affinity:
            return {"success": False, "unsupported": True, "error": "CPU affinity control is unavailable."}
        self.safety_guard.assert_process_action("set_affinity", pid, name)
        if not psutil.pid_exists(pid):
            return {"success": False, "blocked": True, "error": "Target process is no longer running."}
        before = {"pid": pid, "name": name, "affinity": self.get_affinity(pid)}
        if before["affinity"] is None:
            return {"success": False, "blocked": True, "error": "Process affinity could not be read for the selected target."}
        snapshot_id = None
        if self.revert_manager:
            snapshot_id = await self.revert_manager.capture_snapshot(
                target_type="process_affinity",
                target_id=f"{pid}:{name}",
                before_value=before,
                metadata={"reason": reason},
            )

        if self.settings.executor_dry_run:
            result = {"success": True, "dry_run": True, "pid": pid, "name": name, "affinity": cores, "revert_snapshot_id": snapshot_id}
        else:
            try:
                process = psutil.Process(pid)
                process.cpu_affinity(cores)
            except psutil.NoSuchProcess:
                return {"success": False, "blocked": True, "error": "Target process ended before affinity could be applied."}
            except psutil.AccessDenied:
                return {"success": False, "blocked": True, "error": "Access denied while applying process affinity."}
            except (ValueError, psutil.Error):
                return {"success": False, "blocked": True, "error": "Affinity request was rejected for the selected process."}
            result = {"success": True, "dry_run": False, "pid": pid, "name": name, "affinity": cores, "revert_snapshot_id": snapshot_id}

        if self.audit_log:
            await self.audit_log.record_event(
                module="affinity",
                action="set_affinity",
                target=f"{name}:{pid}",
                before_value=before,
                after_value=result,
                rationale=reason,
                validity_tag="HARDWARE_SPECIFIC",
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
            result = {"success": True, "dry_run": True, "pid": before["pid"], "name": before["name"], "affinity": before["affinity"]}
        else:
            try:
                process = psutil.Process(before["pid"])
                process.cpu_affinity(before["affinity"])
            except psutil.NoSuchProcess:
                await self.revert_manager.mark_restored(snapshot_id, status="target_missing")
                return {"success": False, "blocked": True, "error": "Target process ended before affinity restore."}
            except psutil.AccessDenied:
                await self.revert_manager.mark_restored(snapshot_id, status="access_denied")
                return {"success": False, "blocked": True, "error": "Access denied while restoring process affinity."}
            except (ValueError, psutil.Error):
                await self.revert_manager.mark_restored(snapshot_id, status="restore_failed")
                return {"success": False, "blocked": True, "error": "Affinity restore was rejected for the selected process."}
            await self.revert_manager.mark_restored(snapshot_id, status="restored")
            result = {"success": True, "dry_run": False, "pid": before["pid"], "name": before["name"], "affinity": before["affinity"]}
        if self.audit_log:
            await self.audit_log.record_event(
                module="affinity",
                action="restore_affinity",
                target=f"{before['name']}:{before['pid']}",
                before_value={"snapshot_id": snapshot_id},
                after_value=result,
                rationale="Restored CPU affinity to the previous value.",
                validity_tag="HARDWARE_SPECIFIC",
                triggered_by=triggered_by,
                status="success",
            )
        return result
