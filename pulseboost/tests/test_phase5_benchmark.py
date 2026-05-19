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
from core.performance_diagnostics import FrameTimeCapture


class SnapshotStub:
    def __init__(self, cpu_total: float, foreground_app=None):
        self.cpu_total = cpu_total
        self.foreground_app = foreground_app


class CollectorStub:
    def __init__(self, cpu_values, foreground_app=None):
        self.cpu_values = list(cpu_values)
        self.foreground_app = foreground_app
        self.index = 0

    def get_snapshot(self):
        value = self.cpu_values[min(self.index, len(self.cpu_values) - 1)]
        self.index += 1
        return SnapshotStub(value, foreground_app=self.foreground_app)


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

    def build_engine(self, cpu_values, *, frame_time_capture=None):
        return BenchmarkEngine(
            database=self.database,
            audit_log=self.audit_log,
            optimizer=OptimizerStub(),
            collector=CollectorStub(cpu_values, foreground_app={"pid": 42, "name": "game.exe"}),
            frame_time_capture=frame_time_capture,
        )

    def write_presentmon_csv(self, path: Path, rows: list[tuple[str, int, float]], *, mode: str = "a") -> None:
        prefix = ""
        if mode == "w":
            prefix = "Application,ProcessID,msBetweenPresents\n"
        payload = "".join(f"{name},{pid},{frame_time}\n" for name, pid, frame_time in rows)
        path.write_text(prefix + payload, encoding="utf-8") if mode == "w" else path.write_text(path.read_text(encoding="utf-8") + payload, encoding="utf-8")

    def append_presentmon_rows(self, path: Path, rows: list[tuple[str, int, float]]) -> None:
        with path.open("a", encoding="utf-8") as handle:
            for name, pid, frame_time in rows:
                handle.write(f"{name},{pid},{frame_time}\n")

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
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": {"id": "session-1"}},
        )

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
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )

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
        app.state.orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )

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
                self.assertIn("frametime_supported", payload[0])
                self.assertIn("baseline_fps_average", payload[0])

    def test_benchmark_run_persists_real_frametime_evidence_when_csv_configured(self):
        csv_path = Path(self.temp_dir.name) / "presentmon.csv"
        self.write_presentmon_csv(csv_path, [], mode="w")
        engine = self.build_engine([50.0, 50.0, 50.0, 50.0], frame_time_capture=FrameTimeCapture(csv_path))
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )
        baseline_rows = [("game.exe", 42, 20.0), ("game.exe", 42, 16.0)]
        optimized_rows = [("game.exe", 42, 10.0), ("game.exe", 42, 12.0)]
        sleep_calls = {"count": 0}

        async def no_sleep(_seconds):
            sleep_calls["count"] += 1
            if sleep_calls["count"] == 1:
                self.append_presentmon_rows(csv_path, baseline_rows)
            elif sleep_calls["count"] == 2:
                self.append_presentmon_rows(csv_path, optimized_rows)
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(engine.run(orchestrator, workload_name="Frame Test", duration_seconds=2))

        payload = result["result"]
        self.assertTrue(payload["frametime_supported"])
        self.assertEqual(payload["frametime_source"], "presentmon_csv")
        self.assertAlmostEqual(payload["baseline_fps_average"], 56.2, places=1)
        self.assertAlmostEqual(payload["optimized_fps_average"], 91.7, places=1)
        self.assertAlmostEqual(payload["baseline_fps_1_low"], 50.1, places=1)
        self.assertAlmostEqual(payload["optimized_fps_1_low"], 83.5, places=1)
        self.assertAlmostEqual(payload["baseline_p95_frame_time_ms"], 19.8, places=1)
        self.assertAlmostEqual(payload["optimized_p95_frame_time_ms"], 11.9, places=1)
        self.assertEqual(payload["verdict"], "HELPED")

    def test_benchmark_run_reports_unavailable_frametime_reason_when_csv_missing(self):
        engine = self.build_engine([42.0, 42.0, 42.0, 42.0], frame_time_capture=FrameTimeCapture())
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )

        async def no_sleep(_seconds):
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(engine.run(orchestrator, workload_name="No Source", duration_seconds=2))

        payload = result["result"]
        self.assertFalse(payload["frametime_supported"])
        self.assertIsNone(payload["frametime_source"])
        self.assertIsNone(payload["baseline_fps_average"])
        self.assertIn("PRESENTMON_CSV_PATH", payload["frame_time_reason"])
        self.assertEqual(payload["verdict"], "NO_MEASURABLE_IMPACT")

    def test_benchmark_run_marks_regression_when_fps_drops_and_p95_rises(self):
        csv_path = Path(self.temp_dir.name) / "presentmon-regression.csv"
        self.write_presentmon_csv(csv_path, [], mode="w")
        engine = self.build_engine([40.0, 40.0, 40.0, 40.0], frame_time_capture=FrameTimeCapture(csv_path))
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )
        baseline_rows = [("game.exe", 42, 10.0), ("game.exe", 42, 12.0)]
        optimized_rows = [("game.exe", 42, 20.0), ("game.exe", 42, 24.0)]
        sleep_calls = {"count": 0}

        async def no_sleep(_seconds):
            sleep_calls["count"] += 1
            if sleep_calls["count"] == 1:
                self.append_presentmon_rows(csv_path, baseline_rows)
            elif sleep_calls["count"] == 2:
                self.append_presentmon_rows(csv_path, optimized_rows)
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(engine.run(orchestrator, workload_name="Regression Test", duration_seconds=2))

        self.assertEqual(result["result"]["verdict"], "REGRESSION")


if __name__ == "__main__":
    unittest.main()


