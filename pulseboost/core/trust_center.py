from __future__ import annotations

from typing import Any

from core.capabilities import CapabilitySnapshot
from core.database import DatabaseService
from core.optimizer import SystemOptimizer
from core.safety_guard import SafetyGuard


class TrustCenterService:
    def __init__(
        self,
        *,
        database: DatabaseService,
        capabilities: CapabilitySnapshot,
        safety_guard: SafetyGuard,
        optimizer: SystemOptimizer,
    ) -> None:
        self.database = database
        self.capabilities = capabilities
        self.safety_guard = safety_guard
        self.optimizer = optimizer

    async def status(self, foundation: dict[str, Any]) -> dict[str, Any]:
        runtime_current = await self.database.get_runtime_state("current") or {}
        last_clean_exit = await self.database.get_runtime_state("last_clean_exit") or {}
        recovery = await self.database.get_runtime_state("recovery") or {}
        expert_mode_payload = await self.database.get_setting("expert_mode")
        expert_mode = expert_mode_payload if isinstance(expert_mode_payload, dict) else {"enabled": False}
        pending_reverts = await self.database.count_pending_revert_snapshots()
        active_temporary_tweaks = await self.optimizer.temporary_tweaks.list_active()

        unsupported_capabilities = []
        if not self.capabilities.is_admin:
            unsupported_capabilities.append("Administrator privileges are unavailable.")
        if not self.capabilities.can_edit_registry:
            unsupported_capabilities.append("Registry-backed tweaks are unavailable.")
        if not self.capabilities.can_manage_services:
            unsupported_capabilities.append("Service control is unavailable.")
        if not self.capabilities.has_gpu_runtime:
            unsupported_capabilities.append("GPU runtime integration is unavailable.")

        audit_entries = await self.database.list_audit_entries(limit=300)
        permission_audit = self._permission_audit_rows(audit_entries)
        touch_matrix = [
            {
                "category": "System Registry",
                "can_touch": True,
                "note": "YES, audited",
            },
            {
                "category": "Network Settings",
                "can_touch": True,
                "note": "YES, dry-run in some cases",
            },
            {
                "category": "GPU Driver Settings",
                "can_touch": False,
                "note": "NO, read only",
            },
            {
                "category": "BIOS Settings",
                "can_touch": False,
                "note": "NO, display only",
            },
            {
                "category": "Personal Files",
                "can_touch": False,
                "note": "NEVER",
            },
        ]

        payload = {
            "admin_status": self.capabilities.is_admin,
            "last_clean_exit": last_clean_exit,
            "crash_recovery_status": recovery,
            "current_runtime": runtime_current,
            "rollback_readiness": {
                "pending_revert_snapshots": pending_reverts,
                "active_temporary_tweaks": len(active_temporary_tweaks),
                "ready": pending_reverts >= 0,
            },
            "protected_process_rules": sorted(self.safety_guard.PROTECTED_PROCESSES),
            "dangerous_tweaks_disabled": not bool(expert_mode.get("enabled", False)),
            "expert_mode_state": bool(expert_mode.get("enabled", False)),
            "unsupported_capabilities": unsupported_capabilities,
            "touch_matrix": touch_matrix,
            "permission_audit": permission_audit,
            "safeguard_summary": [
                foundation.get("recovery", {}).get("rationale", "Recovery state available."),
                "Protected process rules are active.",
                "Temporary tweaks are tracked for rollback.",
                "Unsupported features remain explicitly disabled instead of faking success.",
            ],
        }
        await self.database.insert_trust_center_state(payload)
        return payload

    def _permission_audit_rows(self, entries: list[dict[str, Any]]) -> list[dict[str, Any]]:
        category_rules = {
            "registry": lambda entry: "registry" in str(entry.get("module", "")).lower() or "tweak" in str(entry.get("action", "")).lower(),
            "network": lambda entry: "network" in str(entry.get("module", "")).lower() or "qos" in str(entry.get("action", "")).lower(),
            "gpu_driver": lambda entry: "gpu" in str(entry.get("module", "")).lower(),
            "bios": lambda entry: "bios" in str(entry.get("module", "")).lower(),
            "files": lambda _entry: False,
        }
        labels = {
            "registry": "System Registry",
            "network": "Network Settings",
            "gpu_driver": "GPU Driver Settings",
            "bios": "BIOS Settings",
            "files": "Personal Files",
        }
        rows: list[dict[str, Any]] = []
        for category, matcher in category_rules.items():
            matching = [entry for entry in entries if matcher(entry)]
            last_action = matching[0] if matching else None
            last_reverted = next((entry for entry in matching if bool(entry.get("reverted"))), None)
            rows.append(
                {
                    "category_key": category,
                    "category_label": labels[category],
                    "last_action": {
                        "action": last_action.get("action"),
                        "target": last_action.get("target"),
                        "timestamp": last_action.get("timestamp"),
                    } if last_action else None,
                    "last_reverted": {
                        "action": last_reverted.get("action"),
                        "target": last_reverted.get("target"),
                        "timestamp": last_reverted.get("revert_timestamp") or last_reverted.get("timestamp"),
                    } if last_reverted else None,
                    "undo_supported": category in {"registry", "network"},
                }
            )
        return rows
