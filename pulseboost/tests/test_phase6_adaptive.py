import asyncio
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from fastapi import FastAPI
from tests.test_client_utils import managed_test_client

from api.routes import router
from core.adaptive_engine import AdaptiveEngine
from core.audit_log import AuditLog
from core.database import DatabaseService


class OptimizerStub:
    def __init__(self, *, success=True):
        self.success = success
        self.calls = []
        self.temporary_tweaks = SimpleNamespace(list_active=self._list_active)

    async def _list_active(self):
        return []

    async def apply_tweak(self, tweak_id, snapshot=None, triggered_by="user"):
        self.calls.append((tweak_id, triggered_by))
        if self.success:
            return {"success": True, "tweak_id": tweak_id, "revert_snapshot_id": f"snap-{tweak_id}"}
        return {"success": False, "tweak_id": tweak_id, "error": "simulated failure"}


class SnapshotStub:
    def __init__(self, cpu_total=80.0, disk_read_bytes=0.0, disk_write_bytes=0.0, top_processes=None):
        self.cpu_total = cpu_total
        self.disk_read_bytes = disk_read_bytes
        self.disk_write_bytes = disk_write_bytes
        self.top_processes = top_processes or []


class AdaptivePhase6Tests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.database = DatabaseService(Path(self.temp_dir.name) / "phase6.db")
        asyncio.run(self.database.initialize())
        self.audit_log = AuditLog(self.database)

    def tearDown(self):
        self.temp_dir.cleanup()

    def build_engine(self, *, success=True):
        optimizer = OptimizerStub(success=success)
        engine = AdaptiveEngine(database=self.database, audit_log=self.audit_log, optimizer=optimizer)
        asyncio.run(engine.initialize())
        return engine, optimizer

    def test_rule_applies_tweak_and_logs_action(self):
        engine, optimizer = self.build_engine()
        snapshot = SnapshotStub(
            cpu_total=84.0,
            top_processes=[{"name": "SearchIndexer.exe", "cpu_percent": 12.0}],
        )

        result = asyncio.run(engine.evaluate(snapshot, "gaming", active_session_id="session-1"))

        self.assertTrue(result["executed_actions"])
        self.assertEqual(optimizer.calls[0][0], "search_indexer_priority")
        recent = asyncio.run(self.database.list_adaptive_actions())
        self.assertEqual(len(recent), 1)
        self.assertEqual(recent[0]["session_id"], "session-1")

    def test_cooldown_prevents_repeat_spam(self):
        engine, optimizer = self.build_engine()
        snapshot = SnapshotStub(
            cpu_total=84.0,
            top_processes=[{"name": "SearchIndexer.exe", "cpu_percent": 12.0}],
        )

        asyncio.run(engine.evaluate(snapshot, "gaming", active_session_id=None))
        second = asyncio.run(engine.evaluate(snapshot, "gaming", active_session_id=None))

        self.assertEqual(len(optimizer.calls), 1)
        self.assertEqual(second["executed_actions"], [])

    def test_failure_path_is_visible_and_logged(self):
        engine, optimizer = self.build_engine(success=False)
        snapshot = SnapshotStub(
            cpu_total=84.0,
            top_processes=[{"name": "SearchIndexer.exe", "cpu_percent": 12.0}],
        )

        result = asyncio.run(engine.evaluate(snapshot, "gaming", active_session_id=None))

        self.assertEqual(len(optimizer.calls), 1)
        self.assertFalse(result["executed_actions"][0]["success"])
        recent = asyncio.run(self.database.list_adaptive_actions())
        self.assertFalse(recent[0]["success"])

    def test_toggle_api_updates_engine_state(self):
        engine, _optimizer = self.build_engine()
        app = FastAPI()
        app.include_router(router)
        app.state.adaptive_engine = engine

        with managed_test_client(app) as client:
            response = client.post("/api/adaptive/toggle", json={"enabled": False})
            self.assertEqual(response.status_code, 200)
            self.assertFalse(response.json()["enabled"])
            status_response = client.get("/api/adaptive/status")
            self.assertEqual(status_response.status_code, 200)
            self.assertFalse(status_response.json()["enabled"])


if __name__ == "__main__":
    unittest.main()


