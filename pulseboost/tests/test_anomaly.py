import asyncio
import unittest

from core.cognition.anomaly import AnomalyDetector


class FakeMemory:
    async def get_baseline(self, metric, hour, session_mode):
        return {"mean": 20.0, "std_dev": 5.0, "sample_count": 80, "last_updated": 0.0}


class Snapshot:
    timestamp = 0.0
    cpu_total = 40.0
    ram_percent = 80.0
    disk_percent = 60.0
    disk_read_bytes = 0.0
    disk_write_bytes = 0.0
    net_recv_bytes = 0.0
    net_sent_bytes = 0.0

    def to_dict(self):
        return self.__dict__


class AnomalyTests(unittest.TestCase):
    def test_anomaly_detected_for_large_deviation(self):
        detector = AnomalyDetector(FakeMemory(), minimum_samples=10)
        anomalies = asyncio.run(detector.analyze(Snapshot(), "normal"))
        self.assertTrue(anomalies)
        self.assertIn("severity", anomalies[0])


if __name__ == "__main__":
    unittest.main()
