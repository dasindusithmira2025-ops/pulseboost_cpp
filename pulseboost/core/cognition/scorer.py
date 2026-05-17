"""
PulseBoost health score calculator.
"""
from __future__ import annotations

from collections import deque


class HealthScorer:
    WEIGHTS = {
        "cpu": 0.25,
        "ram": 0.30,
        "disk": 0.20,
        "network": 0.10,
        "anomaly": 0.15,
    }

    def __init__(self, smoothing_samples: int = 5):
        self._history = deque(maxlen=smoothing_samples)

    def calculate(self, snapshot, anomalies: list[dict]) -> tuple[float, dict[str, float]]:
        cpu_score = self._score_metric(snapshot.cpu_total, low=55, high=80, critical=95)
        ram_score = self._score_metric(snapshot.ram_percent, low=65, high=85, critical=96)
        disk_score = self._score_metric(snapshot.disk_percent, low=75, high=90, critical=98)
        net_score = self._score_network(snapshot.net_sent_bytes, snapshot.net_recv_bytes)
        anomaly_score = self._score_anomalies(anomalies)

        raw = (
            cpu_score * self.WEIGHTS["cpu"]
            + ram_score * self.WEIGHTS["ram"]
            + disk_score * self.WEIGHTS["disk"]
            + net_score * self.WEIGHTS["network"]
            + anomaly_score * self.WEIGHTS["anomaly"]
        )
        self._history.append(raw)
        smoothed = round(sum(self._history) / len(self._history), 1)
        breakdown = {
            "cpu": round(cpu_score * self.WEIGHTS["cpu"], 1),
            "ram": round(ram_score * self.WEIGHTS["ram"], 1),
            "disk": round(disk_score * self.WEIGHTS["disk"], 1),
            "network": round(net_score * self.WEIGHTS["network"], 1),
            "anomaly": round(anomaly_score * self.WEIGHTS["anomaly"], 1),
        }
        return smoothed, breakdown

    def _score_metric(self, value: float, low: float, high: float, critical: float) -> float:
        if value <= low:
            return 100.0
        if value <= high:
            return 100 - ((value - low) / max(high - low, 0.001)) * 25
        if value <= critical:
            return 75 - ((value - high) / max(critical - high, 0.001)) * 55
        return max(0.0, 20 - (value - critical) * 2.5)

    def _score_network(self, net_sent: float, net_recv: float) -> float:
        network_pressure = max(net_sent, net_recv) / (1024 * 1024)
        if network_pressure < 20:
            return 100.0
        if network_pressure < 80:
            return max(65.0, 100 - network_pressure * 0.4)
        return max(35.0, 80 - network_pressure * 0.2)

    def _score_anomalies(self, anomalies: list[dict]) -> float:
        penalties = {"LOW": 3, "MEDIUM": 7, "HIGH": 14, "CRITICAL": 24}
        penalty = sum(penalties.get(item.get("severity", "LOW"), 0) for item in anomalies)
        return max(0.0, 100.0 - penalty)
