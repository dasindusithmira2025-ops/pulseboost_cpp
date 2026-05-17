"""
PulseBoost efficiency scorer.
"""
from __future__ import annotations


class EfficiencyScorer:
    WEIGHTS = {
        "cpu": 0.30,
        "ram": 0.25,
        "disk": 0.20,
        "network": 0.10,
        "thermal": 0.15,
    }

    def calculate(self, snapshot, waste_findings: list[dict]) -> dict[str, object]:
        pillar_penalties = {"cpu": 0.0, "ram": 0.0, "disk": 0.0, "network": 0.0, "thermal": 0.0}
        for finding in waste_findings:
            pillar = finding.get("pillar")
            if pillar not in pillar_penalties:
                continue
            priority = float(finding.get("priority_score", 0.0))
            severity_factor = {
                "immediate": 1.2,
                "high": 1.0,
                "medium": 0.7,
                "low": 0.45,
            }.get(finding.get("priority", "medium"), 0.7)
            pillar_penalties[pillar] += min(45.0, priority * severity_factor * 0.25)

        if getattr(snapshot, "temperature", None):
            thermal_pressure = max(0.0, float(snapshot.temperature) - 75.0)
            pillar_penalties["thermal"] += min(35.0, thermal_pressure * 1.6)

        if getattr(snapshot, "ram_percent", 0.0) > 85:
            pillar_penalties["ram"] += min(20.0, (float(snapshot.ram_percent) - 85.0) * 1.8)

        breakdown = {
            pillar: round(max(0.0, 100.0 - penalty), 1)
            for pillar, penalty in pillar_penalties.items()
        }
        score = round(
            sum(breakdown[pillar] * weight for pillar, weight in self.WEIGHTS.items()),
            1,
        )
        return {
            "score": score,
            "breakdown": breakdown,
            "grade": self._grade(score),
        }

    def _grade(self, score: float) -> str:
        if score >= 90:
            return "A"
        if score >= 75:
            return "B"
        if score >= 60:
            return "C"
        if score >= 45:
            return "D"
        return "F"
