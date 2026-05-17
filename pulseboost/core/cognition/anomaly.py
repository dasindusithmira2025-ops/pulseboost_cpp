"""
PulseBoost anomaly detector.
"""
from __future__ import annotations

import time


SEVERITY_THRESHOLDS = {
    "LOW": 2.0,
    "MEDIUM": 2.5,
    "HIGH": 3.5,
    "CRITICAL": 5.0,
}

MONITORED_METRICS = (
    "cpu_total",
    "ram_percent",
    "disk_percent",
    "disk_read_bytes",
    "disk_write_bytes",
    "net_recv_bytes",
    "net_sent_bytes",
)


class AnomalyDetector:
    def __init__(self, memory, minimum_samples: int = 50):
        self.memory = memory
        self.minimum_samples = minimum_samples

    async def analyze(self, snapshot, session_mode: str) -> list[dict]:
        anomalies: list[dict] = []
        hour = time.localtime(snapshot.timestamp).tm_hour
        values = snapshot.to_dict() if hasattr(snapshot, "to_dict") else vars(snapshot)
        if not values:
            values = {
                metric: getattr(snapshot, metric, None)
                for metric in MONITORED_METRICS
            }
            values["timestamp"] = getattr(snapshot, "timestamp", time.time())

        for metric in MONITORED_METRICS:
            value = values.get(metric)
            if value is None:
                continue

            baseline = await self.memory.get_baseline(metric, hour, session_mode)
            if not baseline or baseline["sample_count"] < self.minimum_samples:
                continue

            deviation = (value - baseline["mean"]) / max(baseline["std_dev"], 0.001)
            if deviation < SEVERITY_THRESHOLDS["LOW"]:
                continue

            severity = "LOW"
            for level in ("MEDIUM", "HIGH", "CRITICAL"):
                if deviation >= SEVERITY_THRESHOLDS[level]:
                    severity = level

            anomalies.append(
                {
                    "metric": metric,
                    "current_value": round(float(value), 2),
                    "expected_mean": round(float(baseline["mean"]), 2),
                    "expected_std": round(float(baseline["std_dev"]), 2),
                    "deviation_score": round(float(deviation), 2),
                    "severity": severity,
                    "timestamp": snapshot.timestamp,
                    "expected_range": {
                        "min": round(float(baseline["mean"] - (2.5 * baseline["std_dev"])), 2),
                        "max": round(float(baseline["mean"] + (2.5 * baseline["std_dev"])), 2),
                    },
                }
            )

        return anomalies
