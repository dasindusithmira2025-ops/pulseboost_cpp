import asyncio
import json
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
from core.models import BenchmarkCapture
from core.performance_diagnostics import FrameTimeCapture


class SnapshotStub:
    def __init__(self, cpu_total: float, ram_percent: float | None = None, foreground_app=None):
        self.cpu_total = cpu_total
        self.ram_percent = ram_percent
        self.foreground_app = foreground_app


class CollectorStub:
    def __init__(self, cpu_values, ram_values=None, foreground_app=None):
        self.cpu_values = list(cpu_values)
        self.ram_values = list(ram_values or cpu_values)
        self.foreground_app = foreground_app
        self.index = 0

    def get_snapshot(self):
        value = self.cpu_values[min(self.index, len(self.cpu_values) - 1)]
        ram = self.ram_values[min(self.index, len(self.ram_values) - 1)]
        self.index += 1
        return SnapshotStub(value, ram_percent=ram, foreground_app=self.foreground_app)


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
        asyncio.run(self.database.close())
        self.temp_dir.cleanup()

    def build_engine(self, cpu_values, *, ram_values=None, frame_time_capture=None):
        return BenchmarkEngine(
            database=self.database,
            audit_log=self.audit_log,
            optimizer=OptimizerStub(),
            collector=CollectorStub(cpu_values, ram_values=ram_values, foreground_app={"pid": 42, "name": "game.exe"}),
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

    def repeated_presentmon_rows(
        self,
        *,
        name: str,
        pid: int,
        frame_time_ms: float,
        count: int,
    ) -> list[tuple[str, int, float]]:
        return [(name, pid, frame_time_ms) for _ in range(count)]

    def insert_legacy_benchmark_payload(self, payload: dict) -> None:
        benchmark_id = payload["benchmark_id"]

        async def _insert() -> None:
            async with self.database._connection() as db:  # noqa: SLF001 - legacy payload simulation in test
                await db.execute(
                    """
                    INSERT INTO benchmark_runs (
                        benchmark_id, session_id, created_at, workload_name, machine_hash,
                        baseline_label, optimized_label, status, notes
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        benchmark_id,
                        payload.get("session_id"),
                        payload.get("created_at"),
                        payload.get("workload_name"),
                        None,
                        "baseline",
                        "optimized",
                        "completed",
                        payload.get("notes", ""),
                    ),
                )
                await db.execute(
                    """
                    INSERT INTO benchmark_results (
                        result_id, benchmark_id, average_fps, fps_1_low, frametime_variance,
                        cpu_delta, gpu_delta, ping_delta, jitter_delta, verdict, confidence_score,
                        unsupported_metrics_json, payload_json
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        f"{benchmark_id}:result",
                        benchmark_id,
                        payload.get("avg_fps_delta"),
                        payload.get("one_percent_low_delta"),
                        payload.get("frame_time_variance_delta"),
                        payload.get("cpu_delta"),
                        payload.get("gpu_delta"),
                        payload.get("ping_delta"),
                        payload.get("jitter_delta"),
                        payload.get("verdict", "NO_MEASURABLE_IMPACT"),
                        None,
                        json.dumps(payload.get("unsupported_reasons") or []),
                        json.dumps(payload),
                    ),
                )
                await db.commit()

        asyncio.run(_insert())

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
        engine = self.build_engine([60.0, 58.0, 40.0, 38.0], ram_values=[70.0, 68.0, 63.0, 61.0])
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
        self.assertEqual(result["result"]["ram_delta"], -7.0)
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

    def test_benchmark_export_uses_real_ram_delta(self):
        engine = self.build_engine([55.0, 54.0, 50.0, 49.0], ram_values=[72.0, 70.0, 65.0, 63.0])
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
                        "workload_name": "RAM Export",
                        "tweak_set": [],
                        "duration_seconds": 2,
                        "notes": "export test",
                        "revert_after": True,
                    },
                )
                self.assertEqual(run_response.status_code, 200)
                benchmark_id = run_response.json()["result"]["benchmark_id"]
                export_response = client.get(f"/api/benchmark/results/{benchmark_id}/export")
                self.assertEqual(export_response.status_code, 200)
                self.assertIn("RAM: baseline=71.0%, optimized=64.0%, delta=-7.0%", export_response.text)

    def test_backend_serialization_adds_safe_defaults_for_legacy_results(self):
        legacy_payload = {
            "benchmark_id": "legacy-run",
            "workload_name": "Legacy Title",
            "created_at": 123.0,
            "duration_seconds": 6,
            "tweak_set": ["search_indexer_priority"],
            "baseline": {"cpu_percent": 58.0},
            "optimized": {"cpu_percent": 51.0},
            "cpu_delta": -7.0,
            "notes": "legacy payload",
            "verdict": "HELPED",
        }
        self.insert_legacy_benchmark_payload(legacy_payload)

        payload = asyncio.run(self.database.get_benchmark_result("legacy-run"))

        self.assertFalse(payload["frametime_supported"])
        self.assertIsNone(payload["frametime_source"])
        self.assertIsNone(payload["baseline_fps_average"])
        self.assertIsNone(payload["optimized_p95_frame_time_ms"])
        self.assertEqual(payload["baseline_frame_sample_count"], 0)
        self.assertEqual(payload["optimized_frame_sample_count"], 0)
        self.assertIsNone(payload["ram_delta"])
        self.assertFalse(payload["frame_time_evidence_unstable"])
        self.assertIsNone(payload["frame_time_evidence_reason"])

    def test_benchmark_run_persists_real_frametime_evidence_when_csv_configured(self):
        csv_path = Path(self.temp_dir.name) / "presentmon.csv"
        self.write_presentmon_csv(csv_path, [], mode="w")
        engine = self.build_engine([50.0, 50.0, 50.0, 50.0], ram_values=[68.0, 68.0, 64.0, 64.0], frame_time_capture=FrameTimeCapture(csv_path))
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )
        baseline_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=20.0, count=24)
        optimized_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=10.0, count=24)
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
        self.assertEqual(payload["baseline_frame_sample_count"], 24)
        self.assertEqual(payload["optimized_frame_sample_count"], 24)
        self.assertAlmostEqual(payload["baseline_fps_average"], 50.0, places=1)
        self.assertAlmostEqual(payload["optimized_fps_average"], 100.0, places=1)
        self.assertAlmostEqual(payload["baseline_fps_1_low"], 50.0, places=1)
        self.assertAlmostEqual(payload["optimized_fps_1_low"], 100.0, places=1)
        self.assertAlmostEqual(payload["baseline_p95_frame_time_ms"], 20.0, places=1)
        self.assertAlmostEqual(payload["optimized_p95_frame_time_ms"], 10.0, places=1)
        self.assertEqual(payload["ram_delta"], -4.0)
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

    def test_benchmark_run_marks_unstable_when_frame_sample_count_is_too_low(self):
        csv_path = Path(self.temp_dir.name) / "presentmon-low-samples.csv"
        self.write_presentmon_csv(csv_path, [], mode="w")
        engine = self.build_engine([45.0, 45.0, 45.0, 45.0], frame_time_capture=FrameTimeCapture(csv_path))
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )
        baseline_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=16.0, count=3)
        optimized_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=12.0, count=3)
        sleep_calls = {"count": 0}

        async def no_sleep(_seconds):
            sleep_calls["count"] += 1
            if sleep_calls["count"] == 1:
                self.append_presentmon_rows(csv_path, baseline_rows)
            elif sleep_calls["count"] == 2:
                self.append_presentmon_rows(csv_path, optimized_rows)
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(engine.run(orchestrator, workload_name="Low Sample Test", duration_seconds=2))

        payload = result["result"]
        self.assertEqual(payload["verdict"], "UNSTABLE")
        self.assertTrue(payload["frame_time_evidence_unstable"])
        self.assertIn("at least 20 frame samples are required", payload["frame_time_evidence_reason"])

    def test_benchmark_run_ignores_tiny_fps_improvement_under_three_percent(self):
        csv_path = Path(self.temp_dir.name) / "presentmon-tiny-fps.csv"
        self.write_presentmon_csv(csv_path, [], mode="w")
        engine = self.build_engine([44.0, 44.0, 44.0, 44.0], frame_time_capture=FrameTimeCapture(csv_path))
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )
        baseline_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=10.0, count=24)
        optimized_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=9.8, count=24)
        sleep_calls = {"count": 0}

        async def no_sleep(_seconds):
            sleep_calls["count"] += 1
            if sleep_calls["count"] == 1:
                self.append_presentmon_rows(csv_path, baseline_rows)
            elif sleep_calls["count"] == 2:
                self.append_presentmon_rows(csv_path, optimized_rows)
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(engine.run(orchestrator, workload_name="Tiny FPS Gain", duration_seconds=2))

        payload = result["result"]
        self.assertAlmostEqual(payload["fps_delta_percent"], 2.0, places=2)
        self.assertEqual(payload["verdict"], "NO_MEASURABLE_IMPACT")

    def test_benchmark_run_ignores_tiny_fps_improvement_sixty_to_sixty_point_five(self):
        result = self.build_engine([20.0, 14.0])._build_result(
            workload_name="Tiny FPS Delta",
            duration_seconds=2,
            tweak_set=[],
            session_id=None,
            baseline=self._capture(avg_fps=60.0, one_percent_low_fps=58.0, cpu_percent=20.0, ram_percent=62.0),
            optimized=self._capture(avg_fps=60.5, one_percent_low_fps=58.2, cpu_percent=20.0, ram_percent=62.0),
            notes="",
            apply_results=[],
            revert_results=[],
        )

        self.assertAlmostEqual(result.fps_delta_percent, 0.83, places=2)
        self.assertEqual(result.verdict, "NO_MEASURABLE_IMPACT")

    def test_benchmark_run_ignores_tiny_p95_improvement_under_three_percent(self):
        csv_path = Path(self.temp_dir.name) / "presentmon-tiny-p95.csv"
        self.write_presentmon_csv(csv_path, [], mode="w")
        engine = self.build_engine([44.0, 44.0, 44.0, 44.0], frame_time_capture=FrameTimeCapture(csv_path))
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )
        baseline_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=10.0, count=24)
        optimized_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=9.8, count=24)
        sleep_calls = {"count": 0}

        async def no_sleep(_seconds):
            sleep_calls["count"] += 1
            if sleep_calls["count"] == 1:
                self.append_presentmon_rows(csv_path, baseline_rows)
            elif sleep_calls["count"] == 2:
                self.append_presentmon_rows(csv_path, optimized_rows)
            return None

        with patch("core.benchmark_engine.asyncio.sleep", new=AsyncMock(side_effect=no_sleep)):
            result = asyncio.run(engine.run(orchestrator, workload_name="Tiny P95 Gain", duration_seconds=2))

        payload = result["result"]
        self.assertAlmostEqual(payload["p95_frame_time_delta_percent"], -2.0, places=2)
        self.assertEqual(payload["verdict"], "NO_MEASURABLE_IMPACT")

    def test_benchmark_run_marks_regression_when_fps_drops_and_p95_rises(self):
        csv_path = Path(self.temp_dir.name) / "presentmon-regression.csv"
        self.write_presentmon_csv(csv_path, [], mode="w")
        engine = self.build_engine([40.0, 40.0, 40.0, 40.0], frame_time_capture=FrameTimeCapture(csv_path))
        orchestrator = SimpleNamespace(
            last_snapshot=SimpleNamespace(foreground_app={"pid": 42, "name": "game.exe"}),
            current_state={"active_session": None},
        )
        baseline_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=10.0, count=24)
        optimized_rows = self.repeated_presentmon_rows(name="game.exe", pid=42, frame_time_ms=20.0, count=24)
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

    def test_percent_delta_returns_none_when_baseline_is_zero(self):
        engine = self.build_engine([40.0, 40.0])

        self.assertIsNone(engine._percent_delta(10.0, 0.0))

    def test_verdict_still_works_with_cpu_only_evidence(self):
        engine = self.build_engine([40.0, 40.0])
        result = engine._build_result(
            workload_name="CPU Only",
            duration_seconds=2,
            tweak_set=[],
            session_id=None,
            baseline=self._capture(cpu_percent=50.0, ram_percent=60.0, frametime_supported=False),
            optimized=self._capture(cpu_percent=42.0, ram_percent=60.0, frametime_supported=False),
            notes="",
            apply_results=[],
            revert_results=[],
        )

        self.assertEqual(result.verdict, "HELPED")
        self.assertEqual(result.cpu_delta, -8.0)

    def _capture(self, **overrides):
        payload = {
            "avg_fps": None,
            "one_percent_low_fps": None,
            "average_frame_time_ms": None,
            "p95_frame_time_ms": None,
            "frame_time_variance_ms": None,
            "frametime_supported": False,
            "frametime_source": None,
            "frametime_reason": None,
            "cpu_percent": None,
            "ram_percent": None,
            "gpu_percent": None,
            "ping_ms": None,
            "jitter_ms": None,
            "sample_count": 2,
            "frame_sample_count": 0,
            "unstable": False,
            "unsupported_reasons": [],
        }
        payload.update(overrides)
        return BenchmarkCapture(**payload)


if __name__ == "__main__":
    unittest.main()


