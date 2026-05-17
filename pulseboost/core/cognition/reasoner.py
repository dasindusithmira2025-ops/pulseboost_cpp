"""
PulseBoost reasoner.
"""
from __future__ import annotations

from pathlib import Path


class Reasoner:
    def __init__(self, memory, prompt_path: str | Path = "prompts/reasoner_v1.txt"):
        self.memory = memory
        self.prompt_path = Path(prompt_path)

    async def evaluate(self, step: dict, snapshot, confirmed: bool = False) -> dict:
        action = step["action"]
        risk_level = step.get("risk_level", "SAFE")
        learning = await self.memory.fetch_learning(action_type=action, limit=5)
        params = step.get("params", {})

        if risk_level == "RISKY" and not confirmed:
            return {
                "proceed": False,
                "reasoning": "Risky actions require explicit user confirmation.",
                "adjusted_params": params,
                "confidence": 0.0,
                "abort_reason": "User confirmation required",
                "action": action,
            }

        if risk_level == "MODERATE" and not confirmed:
            return {
                "proceed": False,
                "reasoning": "Moderate actions are suggested, not auto-executed, unless policy changes.",
                "adjusted_params": params,
                "confidence": 0.52,
                "abort_reason": "Awaiting confirmation",
                "action": action,
                "learning": learning,
            }

        if getattr(snapshot, "cpu_total", 0) < 20 and action == "kill_process":
            return {
                "proceed": False,
                "reasoning": "The system is no longer under enough pressure to justify a process termination.",
                "adjusted_params": params,
                "confidence": 0.2,
                "abort_reason": "Pressure subsided",
                "action": action,
            }

        if action == "kill_process" and not params.get("pid"):
            return {
                "proceed": False,
                "reasoning": "Process termination requires a PID.",
                "adjusted_params": params,
                "confidence": 0.0,
                "abort_reason": "Missing PID",
                "action": action,
            }

        return {
            "proceed": True,
            "reasoning": "The situation still matches the planned remediation and the action is within safe policy."
            if not confirmed
            else "The action has been explicitly confirmed and remains appropriate for the current machine state.",
            "adjusted_params": params,
            "confidence": 0.74 if not confirmed else 0.82,
            "abort_reason": None,
            "action": action,
            "learning": learning,
            "confirmed": confirmed,
            "prompt_template": self._load_prompt(),
        }

    def _load_prompt(self) -> str:
        if self.prompt_path.exists():
            return self.prompt_path.read_text(encoding="utf-8")
        return ""
