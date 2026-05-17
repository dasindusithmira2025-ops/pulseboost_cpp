"""
PulseBoost planner.
"""
from __future__ import annotations

import uuid
from pathlib import Path

from core.tools.system_tools import action_catalog


class Planner:
    def __init__(self, memory, prompt_path: str | Path = "prompts/planner_v1.txt"):
        self.memory = memory
        self.prompt_path = Path(prompt_path)

    async def generate(self, anomalies: list[dict], snapshot, health_score: float) -> dict:
        if not anomalies:
            return {
                "plan_id": str(uuid.uuid4()),
                "goal": "maintain stability",
                "steps": [],
                "estimated_health_improvement": 0,
                "total_risk_level": "SAFE",
            }

        primary = sorted(anomalies, key=lambda item: item["deviation_score"], reverse=True)[0]
        learning = await self.memory.fetch_learning(limit=10)
        steps = self._heuristic_steps(primary, snapshot)
        total_risk = "SAFE"
        if any(step["risk_level"] == "RISKY" for step in steps):
            total_risk = "RISKY"
        elif any(step["risk_level"] == "MODERATE" for step in steps):
            total_risk = "MODERATE"

        return {
            "plan_id": str(uuid.uuid4()),
            "goal": f"stabilize {primary['metric']}",
            "steps": steps,
            "estimated_health_improvement": min(35, int(100 - health_score)),
            "total_risk_level": total_risk,
            "context": {
                "anomaly": primary,
                "learning": learning,
                "available_actions": action_catalog(),
                "prompt_template": self._load_prompt(),
            },
        }

    def _heuristic_steps(self, anomaly: dict, snapshot) -> list[dict]:
        metric = anomaly["metric"]
        offender = snapshot.top_processes[0] if snapshot.top_processes else None

        if metric in {"disk_percent", "disk_write_bytes"}:
            return [
                {
                    "step_id": 1,
                    "action": "clear_temp_files",
                    "params": {},
                    "rationale": "Temp accumulation is a common safe first step for disk pressure.",
                    "depends_on": [],
                    "risk_level": "SAFE",
                    "requires_confirmation": False,
                    "rollback": "No rollback needed; only temporary files are targeted.",
                }
            ]

        if metric in {"cpu_total", "ram_percent"} and offender:
            return [
                {
                    "step_id": 1,
                    "action": "kill_process",
                    "params": {"pid": offender["pid"], "name": offender["name"]},
                    "rationale": "The top resource offender is the fastest way to reduce pressure if the user approves.",
                    "depends_on": [],
                    "risk_level": "MODERATE",
                    "requires_confirmation": True,
                    "rollback": "Restart the application if it was intentionally running.",
                }
            ]

        if metric in {"net_recv_bytes", "net_sent_bytes"}:
            return [
                {
                    "step_id": 1,
                    "action": "flush_dns_cache",
                    "params": {},
                    "rationale": "Resolver instability is one of the few safe network interventions available locally.",
                    "depends_on": [],
                    "risk_level": "SAFE",
                    "requires_confirmation": False,
                    "rollback": "No rollback required.",
                }
            ]

        return [
            {
                "step_id": 1,
                "action": "flush_dns_cache",
                "params": {},
                "rationale": "A safe network hygiene action can help clear transient resolver instability.",
                "depends_on": [],
                "risk_level": "SAFE",
                "requires_confirmation": False,
                "rollback": "No rollback required.",
            }
        ]

    def risk_for_action(self, action_type: str) -> str:
        catalog = {item["action"]: item["risk"] for item in action_catalog()}
        return catalog.get(action_type, "SAFE")

    def _load_prompt(self) -> str:
        if self.prompt_path.exists():
            return self.prompt_path.read_text(encoding="utf-8")
        return ""
