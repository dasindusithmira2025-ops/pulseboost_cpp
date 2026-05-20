import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from core.performance_diagnostics import BottleneckAnalyzer, FrameTimeCapture


class FrameTimeCaptureTests(unittest.TestCase):
    def test_reads_presentmon_style_csv_for_foreground_process(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "presentmon.csv"
            path.write_text(
                "Application,ProcessID,msBetweenPresents\n"
                "game.exe,42,16.6\n"
                "game.exe,42,18.2\n"
                "other.exe,77,40.0\n",
                encoding="utf-8",
            )
            capture = FrameTimeCapture(path)

            result = capture.capture(
                foreground_app={"pid": 42, "name": "game.exe"},
                session_mode="gaming",
            )

            self.assertTrue(result["supported"])
            self.assertEqual(result["source"], "presentmon_csv")
            self.assertEqual(result["sample_count"], 2)
            self.assertAlmostEqual(result["current_frame_time_ms"], 18.2)
            self.assertIsNotNone(result["fps_average"])

    def test_reports_unavailable_without_csv_source(self):
        result = FrameTimeCapture().capture(foreground_app=None, session_mode="normal")

        self.assertFalse(result["supported"])
        self.assertEqual(result["status"], "unavailable")
        self.assertIn("PRESENTMON_CSV_PATH", result["reason"])

    def test_reports_unavailable_for_empty_csv(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "presentmon-empty.csv"
            path.write_text("Application,ProcessID,msBetweenPresents\n", encoding="utf-8")
            capture = FrameTimeCapture(path)

            result = capture.capture(
                foreground_app={"pid": 42, "name": "game.exe"},
                session_mode="benchmark",
            )

            self.assertFalse(result["supported"])
            self.assertIn("empty", result["reason"].lower())

    def test_reports_unavailable_for_stale_csv_marker(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "presentmon-stale.csv"
            path.write_text(
                "Application,ProcessID,msBetweenPresents\n"
                "game.exe,42,16.6\n"
                "game.exe,42,16.7\n",
                encoding="utf-8",
            )
            capture = FrameTimeCapture(path)
            marker = capture.snapshot_marker()

            result = capture.capture(
                foreground_app={"pid": 42, "name": "game.exe"},
                session_mode="benchmark",
                since_marker=marker,
            )

            self.assertFalse(result["supported"])
            self.assertIn("did not receive fresh rows", result["reason"])

    def test_reports_unavailable_for_unrecognized_frame_time_columns(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "presentmon-unknown-columns.csv"
            path.write_text(
                "Application,ProcessID,UnknownMetric\n"
                "game.exe,42,16.6\n",
                encoding="utf-8",
            )
            capture = FrameTimeCapture(path)

            result = capture.capture(
                foreground_app={"pid": 42, "name": "game.exe"},
                session_mode="benchmark",
            )

            self.assertFalse(result["supported"])
            self.assertIn("No recent frame-time rows matched", result["reason"])

    def test_percentile_returns_none_for_empty_values(self):
        self.assertIsNone(FrameTimeCapture._percentile([], 95))


class BottleneckAnalyzerTests(unittest.TestCase):
    def test_detects_gpu_bound_when_gpu_utilization_dominates(self):
        snapshot = SimpleNamespace(
            cpu_total=48.0,
            ram_percent=58.0,
            disk_read_bytes=1024.0,
            disk_write_bytes=2048.0,
            disk_percent=60.0,
            net_recv_bytes=0.0,
            net_sent_bytes=0.0,
            temperature=68.0,
            foreground_app={"pid": 10, "name": "game.exe"},
            top_processes=[{"pid": 10, "name": "game.exe", "cpu_percent": 28.0}],
        )

        result = BottleneckAnalyzer().analyze(
            snapshot=snapshot,
            session_mode="gaming",
            frame_time={"supported": True, "p95_frame_time_ms": 22.0},
            gpu_stats={"utilization_percent": 94.0, "temperature_c": 72.0},
        )

        self.assertEqual(result["current_bottleneck"], "GPU_BOUND")

    def test_detects_background_process_bound(self):
        snapshot = SimpleNamespace(
            cpu_total=68.0,
            ram_percent=55.0,
            disk_read_bytes=1024.0,
            disk_write_bytes=1024.0,
            disk_percent=60.0,
            net_recv_bytes=0.0,
            net_sent_bytes=0.0,
            temperature=68.0,
            foreground_app={"pid": 10, "name": "game.exe"},
            top_processes=[
                {"pid": 10, "name": "game.exe", "cpu_percent": 25.0},
                {"pid": 20, "name": "Indexer.exe", "cpu_percent": 24.0},
            ],
        )

        result = BottleneckAnalyzer().analyze(
            snapshot=snapshot,
            session_mode="gaming",
            frame_time={"supported": False, "p95_frame_time_ms": None},
            gpu_stats={"utilization_percent": 50.0, "temperature_c": 70.0},
        )

        self.assertEqual(result["current_bottleneck"], "BACKGROUND_PROCESS_BOUND")
        self.assertEqual(result["inputs"]["foreground_app"]["name"], "game.exe")


if __name__ == "__main__":
    unittest.main()
