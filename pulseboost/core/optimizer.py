from __future__ import annotations

import time
from typing import Any

import psutil

from config import get_settings
from core.affinity_manager import AffinityManager
from core.audit_log import AuditLog
from core.database import DatabaseService
from core.game_detection import GameDetector
from core.models import CapabilitySnapshot, TweakObject
from core.process_priority_manager import ProcessPriorityManager
from core.registry_wrapper import SafeRegistry
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard, SafetyViolation
from core.service_wrapper import SafeServiceWrapper
from core.temporary_tweaks import TemporaryTweakManager


class SystemOptimizer:
    def __init__(
        self,
        *,
        database: DatabaseService,
        audit_log: AuditLog,
        revert_manager: RevertManager,
        safety_guard: SafetyGuard,
        capabilities: CapabilitySnapshot,
    ) -> None:
        self.database = database
        self.audit_log = audit_log
        self.revert_manager = revert_manager
        self.safety_guard = safety_guard
        self.capabilities = capabilities
        self.settings = get_settings()
        self.registry = SafeRegistry(audit_log=audit_log, revert_manager=revert_manager, capabilities=capabilities)
        self.services = SafeServiceWrapper(audit_log=audit_log, revert_manager=revert_manager, capabilities=capabilities)
        self.priority = ProcessPriorityManager(
            audit_log=audit_log,
            revert_manager=revert_manager,
            safety_guard=safety_guard,
            capabilities=capabilities,
        )
        self.affinity = AffinityManager(
            audit_log=audit_log,
            revert_manager=revert_manager,
            safety_guard=safety_guard,
            capabilities=capabilities,
        )
        self.temporary_tweaks = TemporaryTweakManager(database)
        self.game_hooks = GameDetector()

    def catalog(self, snapshot=None, session_mode: str = "normal") -> list[dict[str, Any]]:
        active_game = self.game_hooks.detect_active_game(snapshot, session_mode) if snapshot else None
        active_game_name = None
        if active_game and active_game.get("detected"):
            active_game_name = active_game.get("name") or active_game.get("game_name")
        return [
            TweakObject(
                id="search_indexer_priority",
                name="Throttle Windows Search",
                category="process",
                rationale="Lower SearchIndexer priority during active sessions so background indexing does not fight foreground work.",
                validity="VALIDATED",
                impact="Reduces background CPU contention.",
                compatibility_note="Requires a live SearchIndexer.exe process; temporary session tweak.",
                temporary=True,
                requires_admin=False,
            ).to_dict(),
            TweakObject(
                id="sysmain_session_manual",
                name="Set SysMain Manual",
                category="service",
                rationale="Temporarily move SysMain to manual start to reduce background disk churn during active sessions.",
                validity="VALIDATED",
                impact="May reduce disk pressure during gaming or heavy work.",
                compatibility_note="Windows admin required. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
            TweakObject(
                id="wsearch_session_manual",
                name="Set Windows Search Manual",
                category="service",
                rationale="Temporarily move Windows Search to manual start when active sessions are more important than indexing latency.",
                validity="VALIDATED",
                impact="Reduces indexing-related CPU and disk usage.",
                compatibility_note="Windows admin required. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
            TweakObject(
                id="background_affinity_limit",
                name="Limit Background Affinity",
                category="process",
                rationale="Limit a background workload to a subset of CPU cores when it should not consume the full machine.",
                validity="HARDWARE_SPECIFIC",
                impact="Can reduce foreground contention on multi-core systems.",
                compatibility_note=(
                    f"Suggested for {active_game_name} session coexistence."
                    if active_game_name
                    else "Requires a target process and CPU affinity support."
                ),
                temporary=True,
                requires_admin=False,
            ).to_dict(),
            TweakObject(
                id="disable_game_dvr",
                name="Disable Game DVR Recording",
                category="registry",
                registry_path="HKCU\\System\\GameConfigStore",
                key="GameDVR_Enabled",
                rationale="Disable background Game DVR recording overhead during active sessions.",
                validity="VALIDATED",
                impact="Can reduce capture overhead and frame-time spikes in capture-heavy titles.",
                compatibility_note="Requires registry access. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
            TweakObject(
                id="disable_app_capture",
                name="Disable Xbox App Capture",
                category="registry",
                registry_path="HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR",
                key="AppCaptureEnabled",
                rationale="Disable app capture so gaming sessions avoid background recording contention.",
                validity="VALIDATED",
                impact="Can reduce background capture hooks and recording overhead.",
                compatibility_note="Requires registry access. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
            TweakObject(
                id="disable_network_throttling",
                name="Disable Multimedia Network Throttling",
                category="registry",
                registry_path="HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile",
                key="NetworkThrottlingIndex",
                rationale="Lift Windows multimedia network throttling during active game sessions.",
                validity="HARDWARE_SPECIFIC",
                impact="Can improve packet pacing consistency for low-latency sessions.",
                compatibility_note="Requires admin registry access. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
            TweakObject(
                id="disable_cortana_assist",
                name="Disable Cortana Assist",
                category="registry",
                registry_path="HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
                key="AllowCortana",
                rationale="Reduce Windows Search assistant background activity while performance mode is active.",
                validity="LEGACY",
                impact="May reduce background assistant-related indexing and telemetry work.",
                compatibility_note="Policy behavior varies across Windows versions. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
            TweakObject(
                id="disable_search_web_results",
                name="Disable Search Web Results",
                category="registry",
                registry_path="HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search",
                key="BingSearchEnabled",
                rationale="Disable search web-result blending so search assistants stay out of gaming sessions.",
                validity="LEGACY",
                impact="Can reduce web assistant search background activity in supported builds.",
                compatibility_note="Policy behavior varies by Windows build. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
            TweakObject(
                id="disable_cortana_consent",
                name="Disable Cortana Consent",
                category="registry",
                registry_path="HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search",
                key="CortanaConsent",
                rationale="Disable Cortana consent for performance-focused session configurations.",
                validity="LEGACY",
                impact="Can reduce assistant-related background behavior in supported builds.",
                compatibility_note="Policy behavior varies by Windows build. Reverted on session cleanup.",
                temporary=True,
                requires_admin=True,
            ).to_dict(),
        ]

    async def apply_tweak(
        self,
        tweak_id: str,
        *,
        snapshot=None,
        params: dict[str, Any] | None = None,
        triggered_by: str = "user",
    ) -> dict[str, Any]:
        params = params or {}
        try:
            if tweak_id == "search_indexer_priority":
                pid, name = self._resolve_process(snapshot, params, preferred_names=["SearchIndexer.exe"])
                result = await self.priority.set_priority(
                    pid,
                    name,
                    "below_normal",
                    triggered_by=triggered_by,
                    reason="Lowered SearchIndexer priority for the active session.",
                )
            elif tweak_id == "sysmain_session_manual":
                result = await self.services.set_start_type(
                    "SysMain",
                    "manual",
                    triggered_by=triggered_by,
                    reason="Set SysMain to manual start for the active session.",
                )
            elif tweak_id == "wsearch_session_manual":
                result = await self.services.set_start_type(
                    "WSearch",
                    "manual",
                    triggered_by=triggered_by,
                    reason="Set Windows Search to manual start for the active session.",
                )
            elif tweak_id == "background_affinity_limit":
                pid, name = self._resolve_process(snapshot, params, preferred_names=[])
                cores = params.get("cores") or self._recommended_affinity()
                result = await self.affinity.set_affinity(
                    pid,
                    name,
                    cores,
                    triggered_by=triggered_by,
                    reason="Limited background process affinity for the active session.",
                )
            elif tweak_id == "disable_game_dvr":
                result = await self.registry.set_value(
                    "HKCU\\System\\GameConfigStore",
                    "GameDVR_Enabled",
                    0,
                    value_kind="DWORD",
                    triggered_by=triggered_by,
                    reason="Disabled Game DVR recording overhead for the active session.",
                )
            elif tweak_id == "disable_app_capture":
                result = await self.registry.set_value(
                    "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR",
                    "AppCaptureEnabled",
                    0,
                    value_kind="DWORD",
                    triggered_by=triggered_by,
                    reason="Disabled Xbox app capture overhead for the active session.",
                )
            elif tweak_id == "disable_network_throttling":
                result = await self.registry.set_value(
                    "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile",
                    "NetworkThrottlingIndex",
                    0xFFFFFFFF,
                    value_kind="DWORD",
                    triggered_by=triggered_by,
                    reason="Disabled multimedia network throttling for latency-sensitive sessions.",
                )
            elif tweak_id == "disable_cortana_assist":
                result = await self.registry.set_value(
                    "HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
                    "AllowCortana",
                    0,
                    value_kind="DWORD",
                    triggered_by=triggered_by,
                    reason="Disabled Cortana assist policy for the active session performance mode.",
                )
            elif tweak_id == "disable_search_web_results":
                result = await self.registry.set_value(
                    "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search",
                    "BingSearchEnabled",
                    0,
                    value_kind="DWORD",
                    triggered_by=triggered_by,
                    reason="Disabled Windows Search web-result blending for the active session.",
                )
            elif tweak_id == "disable_cortana_consent":
                result = await self.registry.set_value(
                    "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search",
                    "CortanaConsent",
                    0,
                    value_kind="DWORD",
                    triggered_by=triggered_by,
                    reason="Disabled Cortana consent flag for the active session performance mode.",
                )
            else:
                return {"success": False, "error": f"Unknown tweak: {tweak_id}"}
        except (ValueError, SafetyViolation) as exc:
            return {
                "success": False,
                "blocked": True,
                "error": str(exc),
                "tweak_id": tweak_id,
            }

        if result.get("success") and result.get("revert_snapshot_id"):
            await self.temporary_tweaks.register(
                {
                    "tweak_id": tweak_id,
                    "snapshot_id": result["revert_snapshot_id"],
                    "target": result.get("service_name") or result.get("name") or result.get("pid"),
                    "applied_at": time.time(),
                }
            )
        return result

    async def revert_tweak(
        self,
        *,
        tweak_id: str | None = None,
        snapshot_id: str | None = None,
        triggered_by: str = "user",
    ) -> dict[str, Any]:
        active = await self.temporary_tweaks.list_active()
        record = None
        if snapshot_id:
            record = next((item for item in active if item.get("snapshot_id") == snapshot_id), None)
        elif tweak_id:
            record = next((item for item in reversed(active) if item.get("tweak_id") == tweak_id), None)
        if not record:
            return {"success": False, "error": "No matching temporary tweak found."}

        result = await self._restore_snapshot(record["snapshot_id"], triggered_by=triggered_by)
        if result.get("success"):
            await self.temporary_tweaks.remove(record["snapshot_id"])
        return result

    async def restore_temporary_tweaks(self, *, triggered_by: str = "session_recovery") -> list[dict[str, Any]]:
        results = []
        for record in list(await self.temporary_tweaks.list_active()):
            result = await self._restore_snapshot(record["snapshot_id"], triggered_by=triggered_by)
            if result.get("success"):
                await self.temporary_tweaks.remove(record["snapshot_id"])
            results.append({"record": record, "result": result})
        return results

    async def _restore_snapshot(self, snapshot_id: str, *, triggered_by: str) -> dict[str, Any]:
        snapshot = await self.revert_manager.database.get_revert_snapshot(snapshot_id)
        if not snapshot:
            return {"success": False, "error": "Revert snapshot not found."}
        target_type = snapshot["target_type"]
        if target_type == "service_start_type":
            return await self.services.restore(snapshot_id, triggered_by=triggered_by)
        if target_type == "process_priority":
            return await self.priority.restore(snapshot_id, triggered_by=triggered_by)
        if target_type == "process_affinity":
            return await self.affinity.restore(snapshot_id, triggered_by=triggered_by)
        if target_type == "registry_value":
            return await self.registry.restore(snapshot_id, triggered_by=triggered_by)
        return {"success": False, "error": f"Unsupported revert target: {target_type}"}

    def _resolve_process(self, snapshot, params: dict[str, Any], preferred_names: list[str]) -> tuple[int, str]:
        invalid_process_names = {"system idle process"}
        preferred_lookup = {name.strip().lower() for name in preferred_names if str(name).strip()}

        pid_param = params.get("pid")
        name_param = params.get("name")
        if pid_param is not None or name_param is not None:
            if pid_param is None or not name_param:
                raise ValueError("Both pid and name are required when selecting a target process.")
            try:
                pid = int(pid_param)
            except (TypeError, ValueError) as exc:
                raise ValueError("Invalid process identifier.") from exc
            name = str(name_param).strip()
            if pid <= 0 or not name:
                raise ValueError("Invalid process identifier.")
            if not psutil.pid_exists(pid):
                raise ValueError("Target process is no longer running.")
            return pid, name

        if snapshot is not None:
            for process in getattr(snapshot, "top_processes", []) or []:
                raw_pid = process.get("pid")
                name = str(process.get("name") or "").strip()
                try:
                    pid = int(raw_pid)
                except (TypeError, ValueError):
                    continue
                if pid <= 0 or not name or name.lower() in invalid_process_names:
                    continue
                if preferred_lookup and name.lower() not in preferred_lookup:
                    continue
                if not psutil.pid_exists(pid):
                    continue
                return pid, name

        if preferred_lookup:
            try:
                for process in psutil.process_iter(attrs=["pid", "name"]):
                    pid = int(process.info.get("pid") or 0)
                    name = str(process.info.get("name") or "").strip()
                    if pid <= 0 or not name:
                        continue
                    if name.lower() not in preferred_lookup:
                        continue
                    return pid, name
            except psutil.Error:
                pass
            preferred_label = ", ".join(preferred_names)
            raise ValueError(f"Required process is not running: {preferred_label}.")

        raise ValueError("No valid target process was found. Provide pid and name in tweak parameters.")

    def _recommended_affinity(self) -> list[int]:
        logical_cores = psutil.cpu_count(logical=True) or 1
        keep = max(1, logical_cores // 2)
        return list(range(keep))
