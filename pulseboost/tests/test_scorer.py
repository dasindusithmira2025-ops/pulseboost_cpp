import unittest

from core.cognition.scorer import HealthScorer


class Snapshot:
    cpu_total = 45
    ram_percent = 51
    disk_percent = 62
    net_sent_bytes = 2048
    net_recv_bytes = 1024


class HealthScorerTests(unittest.TestCase):
    def test_calculate_returns_smoothed_score_and_breakdown(self):
        scorer = HealthScorer()
        score, breakdown = scorer.calculate(Snapshot(), [{"severity": "LOW"}])

        self.assertIsInstance(score, float)
        self.assertGreater(score, 0)
        self.assertIn("cpu", breakdown)
        self.assertLessEqual(score, 100)


if __name__ == "__main__":
    unittest.main()
