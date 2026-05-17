"""
PulseBoost session mode detection heuristics.
"""
from __future__ import annotations

from dataclasses import dataclass


DEVELOPMENT_HINTS = ("code", "pycharm", "idea", "docker", "node", "npm", "pnpm", "vite", "python", "git")
GAMING_HINTS = ("steam", "epic", "valorant", "fortnite", "dota", "elden", "league", "game")
COMPUTE_HINTS = ("python", "jupyter", "tensor", "torch", "cuda", "ollama", "ffmpeg", "render")


@dataclass
class SessionDetection:
    session_mode: str
    confidence: float
    evidence: list[str]


class SessionDetector:
    def detect(self, snapshot, hour_of_day: int) -> SessionDetection:
        processes = snapshot.top_processes
        names = " ".join((proc.get("name") or "").lower() for proc in processes)
        evidence: list[str] = []

        if snapshot.cpu_total < 5 and snapshot.ram_percent < 45 and len(processes) < 6:
            evidence.append("Very low CPU and RAM activity.")
            return SessionDetection("idle", 0.92, evidence)

        if any(token in names for token in GAMING_HINTS) and snapshot.cpu_total > 45:
            evidence.append("Game-related process names detected under sustained load.")
            return SessionDetection("gaming", 0.8, evidence)

        if any(token in names for token in DEVELOPMENT_HINTS):
            evidence.append("Developer tooling appears in the active process list.")
            confidence = 0.68 if snapshot.cpu_total < 75 else 0.78
            return SessionDetection("development", confidence, evidence)

        if snapshot.cpu_total > 85 or (snapshot.cpu_total > 70 and snapshot.ram_percent > 75):
            evidence.append("Sustained high compute footprint.")
            if any(token in names for token in COMPUTE_HINTS):
                evidence.append("Compute or training-related process names detected.")
            return SessionDetection("heavy_compute", 0.83, evidence)

        if 0 <= hour_of_day <= 5 and snapshot.cpu_total < 20:
            evidence.append("Quiet off-hours activity pattern.")
            return SessionDetection("idle", 0.58, evidence)

        evidence.append("No dominant workload signature was detected.")
        return SessionDetection("normal", 0.55, evidence)
