import asyncio
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch

import psutil

from core.audit_log import AuditLog
from core.database import DatabaseService
from core.models import CapabilitySnapshot
from core.optimizer import SystemOptimizer
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard


def build_capabilities(*, can_manage_services: bool = True) -> CapabilitySnapshot:
    return CapabilitySnapshot(
        machine_id="machine-1",
        captured_at=1.0,
        os_name="Windows",
        os_version="test",
        is_windows=True,
        is_admin=can_manage_services,
        desktop_runtime="webview",
        python_version="3.11",
        has_powershell=True,
        has_wmi=False,
        has_temperature_sensors=False,
        has_gpu_runtime=False,
        supports_electron_runtime=False,
        supports_webview_runtime=True,
        can_manage_services=can_manage_services,
        can_edit_registry=can_manage_services,
        can_control_process_priority=True,
        can_control_affinity=True,
        notes=[],
    )


class OptimizerCoreTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.database = DatabaseService(Path(self.temp_dir.name) / "optimizer.db")
        asyncio.run(self.database.initialize())
        self.audit_log = AuditLog(self.database)
        self.revert_manager = RevertManager(self.database, self.audit_log)
        self.revert_manager.set_session_id("session-1")
        self.audit_log.set_session_id("session-1")
        self.optimizer = SystemOptimizer(
            database=self.database,
            audit_log=self.audit_log,
            revert_manager=self.revert_manager,
            safety_guard=SafetyGuard(),
            capabilities=build_capabilities(),
        )

    def tearDown(self):
        self.temp_dir.cleanup()

    def test_catalog_contains_validated_tweaks(self):
        catalog = self.optimizer.catalog()
        ids = {item["id"] for item in catalog}
        self.assertIn("search_indexer_priority", ids)
        self.assertIn("sysmain_session_manual", ids)
        self.assertIn("background_affinity_limit", ids)
        self.assertIn("disable_game_dvr", ids)
        self.assertIn("disable_app_capture", ids)
        self.assertIn("disable_network_throttling", ids)
        self.assertIn("disable_cortana_assist", ids)
        self.assertIn("disable_search_web_results", ids)
        self.assertIn("disable_cortana_consent", ids)

    def test_apply_and_revert_service_tweak_tracks_temporary_state(self):
        with patch.object(self.optimizer.services, "get_start_type", return_value="auto"):
            result = asyncio.run(self.optimizer.apply_tweak("sysmain_session_manual", triggered_by="test"))
        self.assertTrue(result["success"])
        active = asyncio.run(self.optimizer.temporary_tweaks.list_active())
        self.assertEqual(len(active), 1)
        revert_result = asyncio.run(self.optimizer.revert_tweak(tweak_id="sysmain_session_manual", triggered_by="test"))
        self.assertTrue(revert_result["success"])

    def test_apply_priority_tweak_uses_search_indexer_when_present(self):
        snapshot = SimpleNamespace(
            top_processes=[
                {"pid": 42, "name": "SearchIndexer.exe", "cpu_percent": 12.0},
                {"pid": 99, "name": "Code.exe", "cpu_percent": 20.0},
            ]
        )
        with patch("core.optimizer.psutil.pid_exists", return_value=True), patch(
            "core.process_priority_manager.ProcessPriorityManager.get_priority",
            return_value="normal",
        ):
            result = asyncio.run(self.optimizer.apply_tweak("search_indexer_priority", snapshot=snapshot, triggered_by="test"))
        self.assertTrue(result["success"])
        self.assertEqual(result["name"], "SearchIndexer.exe")

    def test_search_indexer_tweak_fails_cleanly_when_required_process_missing(self):
        snapshot = SimpleNamespace(top_processes=[{"pid": 99, "name": "Code.exe", "cpu_percent": 20.0}])
        with patch("core.optimizer.psutil.process_iter", return_value=[]):
            result = asyncio.run(self.optimizer.apply_tweak("search_indexer_priority", snapshot=snapshot, triggered_by="test"))
        self.assertFalse(result["success"])
        self.assertTrue(result.get("blocked"))
        self.assertIn("Required process is not running", result.get("error", ""))

    def test_recovery_restores_all_temporary_tweaks(self):
        with patch.object(self.optimizer.services, "get_start_type", return_value="auto"):
            asyncio.run(self.optimizer.apply_tweak("wsearch_session_manual", triggered_by="test"))
        results = asyncio.run(self.optimizer.restore_temporary_tweaks())
        self.assertEqual(len(results), 1)
        self.assertTrue(results[0]["result"]["success"])
        self.assertEqual(asyncio.run(self.optimizer.temporary_tweaks.list_active()), [])

    def test_apply_registry_tweak_routes_through_registry_wrapper(self):
        mocked_registry = AsyncMock(
            return_value={
                "success": True,
                "dry_run": True,
                "path": "HKCU\\System\\GameConfigStore",
                "key": "GameDVR_Enabled",
                "value": 0,
                "revert_snapshot_id": "snapshot-1",
            }
        )
        with patch.object(self.optimizer.registry, "set_value", mocked_registry):
            result = asyncio.run(self.optimizer.apply_tweak("disable_game_dvr", triggered_by="test"))
        self.assertTrue(result["success"])
        mocked_registry.assert_awaited_once_with(
            "HKCU\\System\\GameConfigStore",
            "GameDVR_Enabled",
            0,
            value_kind="DWORD",
            triggered_by="test",
            reason="Disabled Game DVR recording overhead for the active session.",
        )

    def test_affinity_tweak_invalid_pid_returns_safe_blocked_result(self):
        result = asyncio.run(
            self.optimizer.apply_tweak(
                "background_affinity_limit",
                params={"pid": -1, "name": "Invalid.exe", "cores": [0]},
                triggered_by="test",
            )
        )
        self.assertFalse(result["success"])
        self.assertTrue(result.get("blocked"))
        self.assertTrue(bool(result.get("error")))

    def test_affinity_tweak_process_race_returns_clean_error(self):
        with patch("core.optimizer.psutil.pid_exists", return_value=True), patch(
            "core.affinity_manager.psutil.pid_exists",
            return_value=True,
        ), patch(
            "core.affinity_manager.psutil.Process",
            side_effect=psutil.NoSuchProcess(pid=40444),
        ):
            result = asyncio.run(
                self.optimizer.apply_tweak(
                    "background_affinity_limit",
                    params={"pid": 40444, "name": "Missing.exe", "cores": [0]},
                    triggered_by="test",
                )
            )
        self.assertFalse(result["success"])
        self.assertTrue(result.get("blocked"))
        self.assertIn("process", result.get("error", "").lower())


if __name__ == "__main__":
    unittest.main()
