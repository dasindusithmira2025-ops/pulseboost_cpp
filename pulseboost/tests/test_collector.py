import unittest

from core.tools.collector import MetricCollector


class CollectorTests(unittest.TestCase):
    def test_snapshot_has_expected_fields(self):
        collector = MetricCollector()
        snapshot = collector.get_snapshot()

        self.assertGreaterEqual(snapshot.cpu_total, 0)
        self.assertGreater(snapshot.ram_total, 0)
        self.assertIsInstance(snapshot.top_processes, list)


if __name__ == "__main__":
    unittest.main()
