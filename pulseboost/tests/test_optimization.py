from types import SimpleNamespace
import unittest

from core.cognition.efficiency_scorer import EfficiencyScorer
from core.cognition.scheduler import IntelligentScheduler
from core.cognition.waste_hunter import WasteHunter


class WasteHunterTests(unittest.TestCase):
    def test_detects_background_cpu_and_leak_candidates(self):
        hunter = WasteHunter(memory=None)
        timestamp = 1000.0
        for index in range(12):
            snapshot = SimpleNamespace(
                timestamp=timestamp + index,
                cpu_total=42.0,
                ram_percent=64.0,
                disk_percent=55.0,
                disk_read_bytes=1024.0,
                disk_write_bytes=1024.0,
                net_sent_bytes=0.0,
                net_recv_bytes=0.0,
                temperature=72.0,
                top_processes=[
                    {
                        "pid": 10,
                        "name": "SearchIndexer.exe",
                        "cpu_percent": 18.0,
                        "ram_bytes": 120 * 1024 * 1024,
                        "ram_mb": 120.0,
                        "status": "running",
                    },
                    {
                        "pid": 25,
                        "name": "python.exe",
                        "cpu_percent": 3.0,
                        "ram_bytes": (200 + index * 8) * 1024 * 1024,
                        "ram_mb": float(200 + index * 8),
                        "status": "running",
                    },
                ],
            )
            result = hunter.hunt(snapshot, "development")

        actions = {item["action"] for item in result["optimizations"]}
        self.assertIn("reduce_process_priority", actions)
        self.assertIn("warn_leak", actions)
        self.assertTrue(any(item["anomaly_level"] == "LEAK" for item in result["process_intelligence"]))


class EfficiencyScorerTests(unittest.TestCase):
    def test_efficiency_drops_with_waste(self):
        scorer = EfficiencyScorer()
        snapshot = SimpleNamespace(temperature=91.0, ram_percent=88.0)
        result = scorer.calculate(
            snapshot,
            [
                {"pillar": "cpu", "priority": "high", "priority_score": 30},
                {"pillar": "ram", "priority": "high", "priority_score": 45},
                {"pillar": "thermal", "priority": "immediate", "priority_score": 92},
            ],
        )
        self.assertLess(result["score"], 90)
        self.assertIn(result["grade"], {"B", "C", "D", "F"})


class SchedulerTests(unittest.TestCase):
    def test_scheduler_queues_deferrable_processes_when_active(self):
        scheduler = IntelligentScheduler(memory=None)
        snapshot = SimpleNamespace(
            cpu_total=22.0,
            ram_percent=68.0,
            top_processes=[
                {"pid": 1, "name": "SearchIndexer.exe", "cpu_percent": 9.0},
                {"pid": 2, "name": "Code.exe", "cpu_percent": 15.0},
            ],
        )
        state = scheduler.evaluate(snapshot)
        self.assertFalse(state["is_idle"])
        self.assertEqual(len(state["deferral_candidates"]), 1)
        self.assertTrue(state["queued_tasks"])


if __name__ == "__main__":
    unittest.main()
