import asyncio
import tempfile
import unittest
from pathlib import Path

from core.cognition.memory import MemorySystem


class MemoryTests(unittest.TestCase):
    def test_memory_initializes_and_tracks_baseline(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = Path(temp_dir) / "memory.db"
            vector_path = Path(temp_dir) / "vectors"
            memory = MemorySystem(db_path=db_path, vector_path=vector_path)
            asyncio.run(memory.initialize())
            asyncio.run(memory.update_baseline("cpu_total", 10, "normal", 25.0, 1.0))
            baseline = asyncio.run(memory.get_baseline("cpu_total", 10, "normal"))

            self.assertIsNotNone(baseline)
            self.assertEqual(baseline["sample_count"], 1)

    def test_export_actions_csv_has_header(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = Path(temp_dir) / "memory.db"
            vector_path = Path(temp_dir) / "vectors"
            memory = MemorySystem(db_path=db_path, vector_path=vector_path)
            asyncio.run(memory.initialize())
            asyncio.run(
                memory.store_action(
                    action_type="clear_temp_files",
                    action_detail={"dry_run": True},
                    trigger_reason="test",
                    ai_reasoning="test entry",
                    health_before=90.0,
                    success=True,
                )
            )
            csv_text = asyncio.run(memory.export_actions_csv())

            self.assertIn("action_type", csv_text)
            self.assertIn("clear_temp_files", csv_text)


if __name__ == "__main__":
    unittest.main()
