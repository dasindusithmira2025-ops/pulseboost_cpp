import asyncio
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from core.audit_log import AuditLog
from core.database import DatabaseService
from core.game_detection import GameDetector
from core.metrics import MetricsService
from core.session_manager import SessionManager


class SnapshotStub:
    def __init__(self, timestamp: float, top_processes: list[dict]):
        self.timestamp = timestamp
        self.top_processes = top_processes

    def to_dict(self):
        return {"timestamp": self.timestamp, "top_processes": self.top_processes}


class Phase3Tests(unittest.TestCase):
    def test_session_manager_persists_game_session(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "phase3.db")
            asyncio.run(database.initialize())
            audit_log = AuditLog(database)
            session_manager = SessionManager(database, audit_log, GameDetector())

            gaming = SnapshotStub(1.0, [{"name": "valorant.exe", "pid": 10, "cpu_percent": 50.0}])
            normal = SnapshotStub(2.0, [{"name": "Code.exe", "pid": 11, "cpu_percent": 12.0}])

            asyncio.run(session_manager.update(gaming, "gaming", 96.0))
            self.assertIsNotNone(session_manager.current_session)
            asyncio.run(session_manager.update(normal, "normal", 91.0))

            sessions = asyncio.run(database.list_sessions())
            self.assertEqual(len(sessions), 1)
            self.assertIsNotNone(sessions[0]["ended_at"])

    def test_metrics_service_emits_sse_payload(self):
        service = MetricsService()
        orchestrator = SimpleNamespace(
            current_state={
                "metrics": {"timestamp": 1.0, "cpu_total": 12.0},
                "health_score": 97.0,
                "session_mode": "normal",
                "active_session": None,
            }
        )

        async def get_first():
            generator = service.stream(orchestrator, interval_seconds=0)
            return await anext(generator)

        event = asyncio.run(get_first())
        self.assertIn("event: metrics", event)
        self.assertIn('"health_score": 97.0', event)


if __name__ == "__main__":
    unittest.main()
