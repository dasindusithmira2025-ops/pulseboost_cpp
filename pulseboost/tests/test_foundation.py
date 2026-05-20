import asyncio
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from core.audit_log import AuditLog
from core.capabilities import CapabilityService
from core.database import DatabaseService
from core.platform_info import PlatformInfoService
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard, SafetyViolation
from core.session_recovery import SessionRecovery


class FoundationTests(unittest.TestCase):
    def test_database_bootstrap_and_app_state(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "foundation.db")
            asyncio.run(database.initialize())
            asyncio.run(database.set_app_state("test.key", {"value": 3}))
            state = asyncio.run(database.get_app_state("test.key"))
            self.assertEqual(state["value"], 3)

    def test_database_uses_wal_mode_for_concurrent_runtime_access(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "foundation.db")
            asyncio.run(database.initialize())

            async def journal_mode() -> str:
                async with database._connection() as db:
                    cursor = await db.execute("PRAGMA journal_mode")
                    row = await cursor.fetchone()
                    return str(row[0]).lower()

            self.assertEqual(asyncio.run(journal_mode()), "wal")

    def test_audit_and_revert_persist(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "foundation.db")
            asyncio.run(database.initialize())
            audit_log = AuditLog(database)
            revert_manager = RevertManager(database, audit_log)

            snapshot_id = asyncio.run(
                revert_manager.capture_snapshot(
                    target_type="process_priority",
                    target_id="55:Code.exe",
                    before_value={"priority": "normal"},
                )
            )
            asyncio.run(
                audit_log.record_event(
                    module="tests",
                    action="apply",
                    target="Code.exe:55",
                    before_value={"priority": "normal"},
                    after_value={"priority": "below_normal"},
                    rationale="test entry",
                )
            )

            snapshot = asyncio.run(database.get_revert_snapshot(snapshot_id))
            entries = asyncio.run(database.list_audit_entries())
            self.assertEqual(snapshot["target_id"], "55:Code.exe")
            self.assertGreaterEqual(len(entries), 2)

    def test_session_recovery_detects_unclean_exit(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = Path(temp_dir) / "foundation.db"
            database = DatabaseService(db_path)
            asyncio.run(database.initialize())

            recovery_a = SessionRecovery(database)
            decision_a = asyncio.run(recovery_a.begin_session("desktop-webview"))
            self.assertFalse(decision_a.recovery_required)

            recovery_b = SessionRecovery(database)
            decision_b = asyncio.run(recovery_b.begin_session("desktop-webview"))
            self.assertTrue(decision_b.recovery_required)

            asyncio.run(recovery_b.complete_session())
            current = asyncio.run(database.get_app_state("runtime.current"))
            self.assertTrue(current["clean_exit"])

    def test_capabilities_persist(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "foundation.db")
            asyncio.run(database.initialize())
            platform_info = PlatformInfoService("machine-1", "Test Machine", desktop_runtime="webview")
            service = CapabilityService(database, platform_info)

            snapshot = asyncio.run(service.refresh())
            latest_capabilities = asyncio.run(database.latest_capability_snapshot())
            latest_hardware = asyncio.run(database.latest_hardware_profile())

            self.assertEqual(snapshot.machine_id, "machine-1")
            self.assertEqual(latest_capabilities["machine_id"], "machine-1")
            self.assertEqual(latest_hardware["machine_name"], "Test Machine")

    def test_capabilities_detect_gpu_runtime_when_vendor_runtime_exists(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "foundation.db")
            asyncio.run(database.initialize())
            platform_info = PlatformInfoService("machine-1", "Test Machine", desktop_runtime="electron")
            service = CapabilityService(database, platform_info)

            with patch.object(PlatformInfoService, "_detect_gpu_adapter", return_value={"vendor": "NVIDIA", "model": "RTX Test"}), \
                 patch.object(PlatformInfoService, "gpu_runtime_available", return_value=True):
                snapshot = asyncio.run(service.refresh())

            self.assertTrue(snapshot.has_gpu_runtime)
            self.assertIn("gpu_runtime", asyncio.run(database.latest_hardware_profile())["supported_capabilities"])
            latest_hardware = asyncio.run(database.latest_hardware_profile())
            self.assertEqual(latest_hardware["gpu_vendor"], "NVIDIA")
            self.assertEqual(latest_hardware["gpu_model"], "RTX Test")

    def test_safety_guard_blocks_protected_process(self):
        guard = SafetyGuard()
        with self.assertRaises(SafetyViolation):
            guard.assert_process_action("kill_process", 123, "lsass.exe")


if __name__ == "__main__":
    unittest.main()
