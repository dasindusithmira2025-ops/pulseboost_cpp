import asyncio
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch

from fastapi import FastAPI
from tests.test_client_utils import managed_test_client

from api.routes import router
from core.audit_log import AuditLog
from core.benchmark_engine import BenchmarkEngine
from core.database import DatabaseService


class SnapshotStub:
    def __init__(self, cpu_total: float):
        self.cpu_total = cpu_total


class CollectorStub:
    def __init__(self, cpu_values):
        self.cpu_values = list(cpu_values)
        self.index = 0

    def get_snapshot(self):
        value = self.cpu_values[min(self.index, len(self.cpu_values) - 1)]
        self.index += 1
        return SnapshotStub(value)


class OptimizerStub:
    async def apply_tweak(self, tweak_id, snapshot=None, triggered_by="user"):
        return {"success": True, "tweak_id": tweak_id, "revert_snapshot_id": f"snap-{tweak_id}"}

    async def revert_tweak(self, tweak_id=None, snapshot_id=None, triggered_by="user"):
        return {"success": True, "tweak_id": tweak_id, "snapshot_id": snapshot_id}


class BenchmarkPhase5Tests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.database = DatabaseService(Path(self.temp_dir.name) / "phase5.db")
        asyncio.run(self.database.initialize())
        self.audit_log = AuditLog(self.database)

    def tearDown(self):
        self.temp_dir.cleanup()

    def build_engine(self, cpu_values):
        return BenchmarkEngine(
            database=self.database,
            audit_log=self.audit_log,
            optimizer=OptimizerStub(),
            collector=CollectorStub(cpu_values),
        )

    def test_capture_window_uses_live_gpu_metric_when_available(self):
        engine = BenchmarkEngine(
            database=self.database,
            audit_log=self.audit_log,
            optimizer=OptimizerStub(),
            collector=CollectorStub([40.0, 38.0]),
            gpu_controller=AsyncMock(stats=AsyncMock(return_value={
                "telemetry_supported": True,
                "utilization_percent": 61,
            })),
        )

        async def no_sleep(_seconds):
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            capture = asyncio.run(engine._capture_window(2))

        self.assertEqual(capture.gpu_percent, 61)
        self.assertFalse(any("GPU utilization" in item for item in capture.unsupported_reasons))

    def test_capture_window_reports_exact_gpu_reason_when_unavailable(self):
        engine = BenchmarkEngine(
            database=self.database,
            audit_log=self.audit_log,
            optimizer=OptimizerStub(),
            collector=CollectorStub([40.0, 38.0]),
            gpu_controller=AsyncMock(stats=AsyncMock(return_value={
                "telemetry_supported": False,
                "utilization_percent": None,
                "reason": "NVIDIA GPU detected, but PulseBoost could not find the NVIDIA runtime tool nvidia-smi.",
            })),
        )

        async def no_sleep(_seconds):
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            capture = asyncio.run(engine._capture_window(2))

        self.assertTrue(any("nvidia-smi" in item for item in capture.unsupported_reasons))

    def test_capture_window_uses_fast_network_probe_budget_in_benchmark_mode(self):
        diagnostics_mock = AsyncMock(return_value={"targets": {"public": {"supported": False}}})
        network_optimizer = AsyncMock()
        network_optimizer.diagnostics = diagnostics_mock
        engine = BenchmarkEngine(
            database=self.database,
            audit_log=self.audit_log,
            optimizer=OptimizerStub(),
            collector=CollectorStub([55.0, 54.0]),
            network_optimizer=network_optimizer,
        )

        async def no_sleep(_seconds):
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            asyncio.run(engine._capture_window(2))

        diagnostics_mock.assert_awaited_once_with(
            session_mode="benchmark",
            active_session=None,
            probe_attempts=1,
            probe_timeout_seconds=0.5,
        )

    def test_benchmark_run_persists_helped_result(self):
        engine = self.build_engine([60.0, 58.0, 40.0, 38.0])
        orchestrator = SimpleNamespace(last_snapshot=None, current_state={"active_session": {"id": "session-1"}})

        async def no_sleep(_seconds):
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(
                engine.run(
                    orchestrator,
                    workload_name="Apex Legends",
                    tweak_set=["search_indexer_priority"],
                    duration_seconds=2,
                    notes="Phase 5 quick proof run",
                )
            )

        self.assertTrue(result["success"])
        self.assertEqual(result["result"]["verdict"], "HELPED")
        stored = asyncio.run(self.database.list_benchmark_results())
        self.assertEqual(len(stored), 1)
        self.assertEqual(stored[0]["workload_name"], "Apex Legends")
        self.assertEqual(stored[0]["session_id"], "session-1")

    def test_benchmark_run_marks_unstable_on_high_variance(self):
        engine = self.build_engine([20.0, 21.0, 10.0, 60.0])
        orchestrator = SimpleNamespace(last_snapshot=None, current_state={"active_session": None})

        async def no_sleep(_seconds):
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(engine.run(orchestrator, workload_name="Stress Test", duration_seconds=2))

        self.assertTrue(result["success"])
        self.assertEqual(result["result"]["verdict"], "UNSTABLE")

    def test_benchmark_api_run_and_list(self):
        engine = self.build_engine([55.0, 54.0, 52.0, 51.0])
        app = FastAPI()
        app.include_router(router)
        app.state.benchmark_engine = engine
        app.state.orchestrator = SimpleNamespace(last_snapshot=None, current_state={"active_session": None})

        async def no_sleep(_seconds):
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            with managed_test_client(app) as client:
                run_response = client.post(
                    "/api/benchmark/run",
                    json={
                        "workload_name": "Rocket League",
                        "tweak_set": [],
                        "duration_seconds": 2,
                        "notes": "api test",
                        "revert_after": True,
                    },
                )
                self.assertEqual(run_response.status_code, 200)
                results_response = client.get("/api/benchmark/results")
                self.assertEqual(results_response.status_code, 200)
                payload = results_response.json()
                self.assertEqual(len(payload), 1)
                self.assertEqual(payload[0]["workload_name"], "Rocket League")


if __name__ == "__main__":
    unittest.main()


