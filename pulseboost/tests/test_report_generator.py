import asyncio
import tempfile
import unittest
from pathlib import Path

from core.report_generator import ReportGenerator


class _FakeDatabase:
    async def latest_hardware_profile(self) -> dict:
        return {
            "machine_name": "PulseBoost Test Rig",
            "os_name": "Windows",
            "os_version": "11",
            "cpu_name": "Test CPU",
            "gpu_model": "Test GPU",
            "ram_total_bytes": 32 * 1024 * 1024 * 1024,
            "captured_at": 1700000000.0,
        }


class _FakeTemporaryTweaks:
    async def list_active(self) -> list[dict]:
        return [{"tweak_id": "disable_game_dvr"}]


class _FakeOptimizer:
    def __init__(self) -> None:
        self.temporary_tweaks = _FakeTemporaryTweaks()

    def catalog(self, snapshot=None, session_mode: str = "normal") -> list[dict]:
        return [
            {
                "id": "disable_game_dvr",
                "name": "Disable Game DVR Recording",
                "rationale": "Disable background recording overhead during active sessions.",
            }
        ]


class ReportGeneratorTests(unittest.TestCase):
    def test_export_markdown_report_writes_expected_sections(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            generator = ReportGenerator(documents_dir=Path(temp_dir))
            report_path = asyncio.run(
                generator.export_markdown_report(
                    database=_FakeDatabase(),
                    optimizer=_FakeOptimizer(),
                    state={"health_score": 82.4, "anomalies": []},
                    health_history=[{"timestamp": 1700000000.0, "health_score": 78.0, "cpu_total": 24.0, "ram_percent": 58.0}],
                )
            )

            self.assertTrue(report_path.exists())
            content = report_path.read_text(encoding="utf-8")
            self.assertIn("# PulseBoost System Report", content)
            self.assertIn("## Current Health Score", content)
            self.assertIn("Disable Game DVR Recording", content)
            self.assertIn("Last 7-Day Health History", content)


if __name__ == "__main__":
    unittest.main()
