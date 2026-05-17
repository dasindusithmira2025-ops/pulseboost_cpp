"""
PulseBoost leak analyzer.
"""
from __future__ import annotations


class LeakAnalyzer:
    def analyze(self, pid: int, name: str, history: list[dict]) -> dict | None:
        if len(history) < 10:
            return None

        ram_values = [int(item["ram"]) for item in history]
        if max(ram_values) <= 0:
            return None

        slope = self._linear_slope(ram_values)
        slope_mb_per_min = slope / (1024 * 1024)
        current_mb = ram_values[-1] / (1024 * 1024)
        projected_mb = current_mb + (slope_mb_per_min * 30)
        cpu_average = sum(float(item["cpu"]) for item in history[-5:]) / 5
        is_leak = slope_mb_per_min > 5 and current_mb > 80 and cpu_average < 20
        confidence = min(0.95, max(0.2, 0.35 + (slope_mb_per_min / 14)))

        return {
            "pid": pid,
            "name": name,
            "growth_rate_mb_per_min": round(slope_mb_per_min, 2),
            "current_ram_mb": round(current_mb, 1),
            "projected_30min_mb": round(projected_mb, 1),
            "confidence": round(confidence, 2),
            "is_leak": is_leak,
            "reasoning": "Sustained memory growth with low foreground activity." if is_leak else "Growth is within expected working-set churn.",
            "recommended_action": "warn" if is_leak else "monitor",
        }

    def _linear_slope(self, values: list[int]) -> float:
        count = len(values)
        if count < 2:
            return 0.0
        x_mean = (count - 1) / 2
        y_mean = sum(values) / count
        numerator = sum((idx - x_mean) * (value - y_mean) for idx, value in enumerate(values))
        denominator = sum((idx - x_mean) ** 2 for idx in range(count))
        if denominator == 0:
            return 0.0
        return numerator / denominator
