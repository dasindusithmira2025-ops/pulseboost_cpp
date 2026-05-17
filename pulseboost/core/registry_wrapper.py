from __future__ import annotations

import platform
from typing import Any

from config import get_settings
from core.audit_log import AuditLog
from core.models import CapabilitySnapshot
from core.revert_manager import RevertManager

try:
    import winreg
except ImportError:  # pragma: no cover
    winreg = None


ROOT_KEYS = {
    "HKEY_LOCAL_MACHINE": getattr(winreg, "HKEY_LOCAL_MACHINE", None),
    "HKLM": getattr(winreg, "HKEY_LOCAL_MACHINE", None),
    "HKEY_CURRENT_USER": getattr(winreg, "HKEY_CURRENT_USER", None),
    "HKCU": getattr(winreg, "HKEY_CURRENT_USER", None),
}

VALUE_TYPES = {
    "DWORD": getattr(winreg, "REG_DWORD", 4),
    "SZ": getattr(winreg, "REG_SZ", 1),
}


class SafeRegistry:
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
        return bool(winreg) and platform.system() == "Windows" and self.capabilities.can_edit_registry

    def _parse_path(self, path: str):
        parts = path.split("\\", 1)
        if len(parts) != 2 or parts[0] not in ROOT_KEYS or ROOT_KEYS[parts[0]] is None:
            raise ValueError(f"Unsupported registry root: {path}")
        return ROOT_KEYS[parts[0]], parts[1]

    def read_value(self, path: str, key: str) -> dict[str, Any]:
        if not self._supported():
            return {"supported": False, "value": None, "type": None}
        root, subkey = self._parse_path(path)
        try:
            with winreg.OpenKey(root, subkey, 0, winreg.KEY_READ) as handle:
                try:
                    value, value_type = winreg.QueryValueEx(handle, key)
                except FileNotFoundError:
                    return {"supported": True, "value": None, "type": None}
        except FileNotFoundError:
            return {"supported": True, "value": None, "type": None}
        return {"supported": True, "value": value, "type": value_type}

    async def set_value(
        self,
        path: str,
        key: str,
        value: Any,
        *,
        value_kind: str = "DWORD",
        triggered_by: str = "optimizer",
        reason: str = "Applied a validated registry tweak.",
    ) -> dict[str, Any]:
        if not self._supported():
            return {"success": False, "unsupported": True, "error": "Registry editing is not supported on this machine."}

        before = self.read_value(path, key)
        snapshot_id = None
        if self.revert_manager:
            snapshot_id = await self.revert_manager.capture_snapshot(
                target_type="registry_value",
                target_id=f"{path}:{key}",
                before_value={"path": path, "key": key, "value": before.get("value"), "type": before.get("type")},
                metadata={"reason": reason},
            )

        if self.settings.executor_dry_run:
            result = {
                "success": True,
                "dry_run": True,
                "path": path,
                "key": key,
                "value": value,
                "value_kind": value_kind,
                "revert_snapshot_id": snapshot_id,
            }
        else:
            root, subkey = self._parse_path(path)
            with winreg.CreateKeyEx(root, subkey, 0, winreg.KEY_SET_VALUE) as handle:
                winreg.SetValueEx(handle, key, 0, VALUE_TYPES[value_kind], value)
            result = {
                "success": True,
                "dry_run": False,
                "path": path,
                "key": key,
                "value": value,
                "value_kind": value_kind,
                "revert_snapshot_id": snapshot_id,
            }

        if self.audit_log:
            await self.audit_log.record_event(
                module="registry",
                action="set_value",
                target=f"{path}:{key}",
                before_value=before,
                after_value=result,
                rationale=reason,
                validity_tag="VALIDATED",
                triggered_by=triggered_by,
                status="success",
            )
        return result

    async def restore(self, snapshot_id: str, *, triggered_by: str = "optimizer_restore") -> dict[str, Any]:
        if not self.revert_manager:
            return {"success": False, "error": "No revert manager configured."}
        snapshot = await self.revert_manager.database.get_revert_snapshot(snapshot_id)
        if not snapshot:
            return {"success": False, "error": "Revert snapshot not found."}
        before = snapshot["before_value"]
        if self.settings.executor_dry_run:
            await self.revert_manager.mark_restored(snapshot_id, status="restored_dry_run")
            result = {"success": True, "dry_run": True, "restored": before}
        else:
            root, subkey = self._parse_path(before["path"])
            with winreg.CreateKeyEx(root, subkey, 0, winreg.KEY_SET_VALUE) as handle:
                if before["value"] is None:
                    try:
                        winreg.DeleteValue(handle, before["key"])
                    except FileNotFoundError:
                        # Value was already absent, which is the intended restored state.
                        pass
                else:
                    winreg.SetValueEx(handle, before["key"], 0, before["type"], before["value"])
            await self.revert_manager.mark_restored(snapshot_id, status="restored")
            result = {"success": True, "dry_run": False, "restored": before}

        if self.audit_log:
            await self.audit_log.record_event(
                module="registry",
                action="restore_value",
                target=f"{before['path']}:{before['key']}",
                before_value={"snapshot_id": snapshot_id},
                after_value=result,
                rationale="Restored a registry-backed tweak to its previous state.",
                validity_tag="VALIDATED",
                triggered_by=triggered_by,
                status="success",
            )
        return result
