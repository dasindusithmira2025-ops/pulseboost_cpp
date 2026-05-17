"""
PulseBoost learning loop support.
"""
from __future__ import annotations

import hashlib
import json


class LearningEngine:
    def __init__(self, memory):
        self.memory = memory

    def build_context_hash(self, snapshot, anomalies: list[dict], session_mode: str) -> str:
        payload = {
            "cpu_total": round(float(snapshot.cpu_total), 1),
            "ram_percent": round(float(snapshot.ram_percent), 1),
            "disk_percent": round(float(snapshot.disk_percent), 1),
            "session_mode": session_mode,
            "anomalies": sorted(item["metric"] for item in anomalies),
        }
        raw = json.dumps(payload, sort_keys=True)
        return hashlib.sha256(raw.encode("utf-8")).hexdigest()

    async def record_outcome(
        self,
        action_type: str,
        context_hash: str,
        health_before: float,
        health_after: float,
    ) -> float:
        delta = round(float(health_after - health_before), 1)
        await self.memory.upsert_learning(action_type=action_type, context_hash=context_hash, score_delta=delta)
        return delta
