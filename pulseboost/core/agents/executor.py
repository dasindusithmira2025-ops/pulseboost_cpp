"""
PulseBoost action executor.
"""
from __future__ import annotations

import os
import platform
import shutil
import subprocess
from pathlib import Path
from typing import Any

import psutil

from config import get_settings
from core.audit_log import AuditLog
from core.compatibility import CompatibilityService
from core.models import CapabilitySnapshot
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard, SafetyViolation
from core.tools.system_tools import temp_directory


class Executor:
    def __init__(
        self,
        memory,
        *,
        audit_log: AuditLog | None = None,
        revert_manager: RevertManager | None = None,
        safety_guard: SafetyGuard | None = None,
        compatibility: CompatibilityService | None = None,
        capabilities: CapabilitySnapshot | None = None,
    ):
        self.memory = memory
        self.settings = get_settings()
        self.audit_log = audit_log
        self.revert_manager = revert_manager
        self.safety_guard = safety_guard or SafetyGuard()
        self.compatibility = compatibility or CompatibilityService()
        self.capabilities = capabilities

    async def run(self, thought: dict, health_before: float, trigger_reason: str = "automated_remediation") -> dict:
        action = thought.get("action")
        params = thought.get("adjusted_params", {})
        reasoning = thought.get("reasoning", "")
        before_state: dict[str, Any] | None = None
        snapshot_id: str | None = None

        try:
            compatibility = self._check_compatibility(action)
            if compatibility and not compatibility.supported:
                raise RuntimeError("; ".join(compatibility.reasons))

            if action == "clear_temp_files":
                result = self._clear_temp()
            elif action == "flush_dns_cache":
                result = self._flush_dns_cache()
            elif action == "kill_process":
                self.safety_guard.assert_process_action("kill_process", params["pid"], params["name"])
                before_state = self._process_snapshot(params["pid"], params["name"])
                snapshot_id = await self._capture_revert_snapshot("process", f"{params['pid']}:{params['name']}", before_state)
                result = self._kill_process(params["pid"], params["name"])
            elif action == "reduce_process_priority":
                self.safety_guard.assert_process_action("reduce_process_priority", params["pid"], params.get("name", "unknown"))
                before_state = self._process_snapshot(params["pid"], params.get("name", "unknown"))
                snapshot_id = await self._capture_revert_snapshot(
                    "process_priority",
                    f"{params['pid']}:{params.get('name', 'unknown')}",
                    before_state,
                )
                result = self._reduce_process_priority(params["pid"], params.get("name", "unknown"))
            elif action == "drop_ram_cache":
                result = self._drop_ram_cache()
            elif action in {"warn_leak", "investigate_process", "identify_disk_writer"}:
                result = {"success": True, "dry_run": True, "info_only": True, "action": action, "params": params}
            else:
                result = {"success": False, "error": f"Unknown action: {action}"}
        except SafetyViolation as exc:
            result = {"success": False, "error": str(exc), "blocked_by_safety_guard": True}
        except Exception as exc:  # pragma: no cover
            result = {"success": False, "error": str(exc)}

        await self.memory.store_action(
            action_type=action or "unknown",
            action_detail={"params": params, "dry_run": self.settings.executor_dry_run, "result": result},
            trigger_reason=trigger_reason,
            ai_reasoning=reasoning,
            health_before=health_before,
            success=result.get("success"),
            error_message=result.get("error"),
        )
        if self.audit_log:
            await self.audit_log.record_event(
                module="executor",
                action=action or "unknown",
                target=self._audit_target(action, params),
                before_value=before_state,
                after_value={"result": result, "revert_snapshot_id": snapshot_id},
                rationale=reasoning or f"Executed {action or 'unknown'} via PulseBoost executor.",
                validity_tag="VALIDATED" if not result.get("blocked_by_safety_guard") else "SAFETY_BLOCKED",
                triggered_by=trigger_reason,
                status="success" if result.get("success") else "blocked" if result.get("blocked_by_safety_guard") else "failed",
            )
        return result

    def set_capabilities(self, capabilities: CapabilitySnapshot) -> None:
        self.capabilities = capabilities

    def _check_compatibility(self, action: str | None):
        if not action or not self.capabilities:
            return None
        return self.compatibility.evaluate_action(action, self.capabilities)

    async def _capture_revert_snapshot(self, target_type: str, target_id: str, before_value: dict[str, Any] | None) -> str | None:
        if not self.revert_manager or before_value is None:
            return None
        return await self.revert_manager.capture_snapshot(
            target_type=target_type,
            target_id=target_id,
            before_value=before_value,
            metadata={"dry_run": self.settings.executor_dry_run},
        )

    def _process_snapshot(self, pid: int, name: str) -> dict[str, Any]:
        process = psutil.Process(pid)
        snapshot = {
            "pid": pid,
            "name": name,
            "status": process.status(),
        }
        try:
            snapshot["priority"] = process.nice()
        except Exception:  # pragma: no cover
            snapshot["priority"] = None
        return snapshot

    def _audit_target(self, action: str | None, params: dict[str, Any]) -> str:
        if action in {"kill_process", "reduce_process_priority"}:
            return f"{params.get('name', 'unknown')}:{params.get('pid', 'n/a')}"
        if action == "clear_temp_files":
            return str(temp_directory())
        return action or "unknown"

    def _clear_temp(self) -> dict:
        target = Path(temp_directory())
        deleted_files = 0
        reclaimed_bytes = 0
        for entry in target.iterdir():
            try:
                if entry.is_file():
                    reclaimed_bytes += entry.stat().st_size
                    if not self.settings.executor_dry_run:
                        entry.unlink()
                    deleted_files += 1
                elif entry.is_dir() and entry.name.lower().startswith("pulseboost"):
                    if not self.settings.executor_dry_run:
                        shutil.rmtree(entry, ignore_errors=True)
            except OSError:
                continue
        return {
            "success": True,
            "dry_run": self.settings.executor_dry_run,
            "deleted_files": deleted_files,
            "reclaimed_bytes": reclaimed_bytes,
        }

    def _flush_dns_cache(self) -> dict:
        if self.settings.executor_dry_run:
            return {"success": True, "dry_run": True, "command": self._dns_command()}
        command = self._dns_command()
        subprocess.run(command, check=True, shell=False)
        return {"success": True, "dry_run": False, "command": command}

    def _dns_command(self) -> list[str]:
        system = platform.system()
        if system == "Windows":
            return ["ipconfig", "/flushdns"]
        if system == "Darwin":
            return ["dscacheutil", "-flushcache"]
        return ["systemd-resolve", "--flush-caches"]

    def _kill_process(self, pid: int, name: str) -> dict:
        process = psutil.Process(pid)
        if self.settings.executor_dry_run:
            return {"success": True, "dry_run": True, "pid": pid, "name": name}
        process.terminate()
        process.wait(timeout=5)
        return {"success": True, "dry_run": False, "pid": pid, "name": name}

    def _drop_ram_cache(self) -> dict:
        if platform.system() != "Linux":
            return {"success": False, "error": "drop_ram_cache is Linux only"}
        command = ["sync"]
        if self.settings.executor_dry_run:
            return {"success": True, "dry_run": True, "command": command}
        subprocess.run(command, check=True, shell=False)
        os.sync()
        return {"success": True, "dry_run": False}

    def _reduce_process_priority(self, pid: int, name: str) -> dict:
        process = psutil.Process(pid)
        if self.settings.executor_dry_run:
            return {"success": True, "dry_run": True, "pid": pid, "name": name}
        if platform.system() == "Windows":
            process.nice(psutil.BELOW_NORMAL_PRIORITY_CLASS)
        else:
            process.nice(10)
        return {"success": True, "dry_run": False, "pid": pid, "name": name}
