"""
PulseBoost predictive engine.
"""
from __future__ import annotations

from collections.abc import Sequence

import numpy as np


class Predictor:
    WINDOWS = {
        "ram_exhaustion": ("ram_percent", 95.0),
        "cpu_sustained_overload": ("cpu_total", 92.0),
        "disk_full": ("disk_percent", 97.0),
    }

    def forecast(self, history: Sequence) -> list[dict]:
        if len(history) < 5:
            return []

        predictions: list[dict] = []
        timestamps = np.array([item.timestamp for item in history], dtype=float)
        relative_time = timestamps - timestamps[0]

        for name, (metric, threshold) in self.WINDOWS.items():
            values = np.array([float(getattr(item, metric)) for item in history], dtype=float)
            smoothed = self._exp_smooth(values)
            slope, intercept = self._linear_fit(relative_time, smoothed)
            current = float(values[-1])
            prediction = self._project(name, metric, current, slope, intercept, relative_time[-1], threshold)
            if prediction:
                predictions.append(prediction)

        return predictions

    def _exp_smooth(self, values: np.ndarray, alpha: float = 0.35) -> np.ndarray:
        result = np.zeros_like(values)
        result[0] = values[0]
        for index in range(1, len(values)):
            result[index] = alpha * values[index] + (1 - alpha) * result[index - 1]
        return result

    def _linear_fit(self, x: np.ndarray, y: np.ndarray) -> tuple[float, float]:
        if len(x) < 2:
            return 0.0, float(y[-1])
        slope, intercept = np.polyfit(x, y, deg=1)
        return float(slope), float(intercept)

    def _project(
        self,
        name: str,
        metric: str,
        current: float,
        slope: float,
        intercept: float,
        current_time: float,
        threshold: float,
    ) -> dict | None:
        if slope <= 0:
            return None
        threshold_time = (threshold - intercept) / slope
        seconds_remaining = threshold_time - current_time
        if seconds_remaining <= 0 or seconds_remaining > 60 * 60 * 6:
            return None

        confidence = min(0.96, max(0.25, (current / max(threshold, 1)) * 0.55 + min(abs(slope) / 5, 0.3)))
        minutes = max(1, round(seconds_remaining / 60))
        action = {
            "ram_percent": "Close or reduce the largest memory consumer.",
            "cpu_total": "Inspect the top CPU process before sustained throttling begins.",
            "disk_percent": "Free disk space or rotate temporary files.",
        }[metric]
        return {
            "type": name,
            "metric": metric,
            "threshold": threshold,
            "current_value": round(current, 1),
            "time_to_threshold_seconds": round(seconds_remaining),
            "time_to_threshold_human": f"{minutes} minute{'s' if minutes != 1 else ''}",
            "confidence": round(confidence * 100, 1),
            "recommended_action": action,
        }
