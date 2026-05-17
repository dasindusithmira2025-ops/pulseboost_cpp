from __future__ import annotations

import asyncio
import json


class MetricsService:
    async def stream(self, orchestrator, interval_seconds: float = 1.0):
        while True:
            state = orchestrator.current_state
            payload = {
                "timestamp": (state.get("metrics") or {}).get("timestamp"),
                "health_score": state.get("health_score"),
                "session_mode": state.get("session_mode"),
                "metrics": state.get("metrics"),
                "active_session": state.get("active_session"),
            }
            yield f"event: metrics\ndata: {json.dumps(payload, default=str)}\n\n"
            await asyncio.sleep(interval_seconds)
