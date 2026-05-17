from __future__ import annotations

from core.models import CompatibilityResult


class SafetyViolation(RuntimeError):
    pass


class SafetyGuard:
    PROTECTED_PROCESSES = {
        "msmpeng.exe",
        "audiodg.exe",
        "nvcontainer.exe",
        "dwm.exe",
        "csrss.exe",
        "lsass.exe",
    }

    def is_process_protected(self, name: str | None) -> bool:
        return bool(name) and name.lower() in self.PROTECTED_PROCESSES

    def validate_process_action(self, action_type: str, pid: int, name: str) -> CompatibilityResult:
        reasons: list[str] = []
        warnings: list[str] = []
        if pid <= 0:
            reasons.append("Invalid process identifier.")
        if self.is_process_protected(name):
            reasons.append(f"{name} is protected and cannot be targeted by {action_type}.")
        if action_type == "kill_process":
            warnings.append("Process termination is not revertible.")
        return CompatibilityResult(supported=not reasons, reasons=reasons, warnings=warnings)

    def assert_process_action(self, action_type: str, pid: int, name: str) -> None:
        result = self.validate_process_action(action_type, pid, name)
        if not result.supported:
            raise SafetyViolation("; ".join(result.reasons))
