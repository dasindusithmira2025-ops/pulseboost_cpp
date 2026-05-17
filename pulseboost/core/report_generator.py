from __future__ import annotations

from datetime import datetime
from pathlib import Path
from typing import Any


class ReportGenerator:
    def __init__(self, documents_dir: Path | None = None) -> None:
        self.documents_dir = documents_dir or (Path.home() / "Documents")

    async def export_markdown_report(
        self,
        *,
        database,
        optimizer,
        state: dict[str, Any],
        health_history: list[dict[str, Any]],
    ) -> Path:
        profile = await database.latest_hardware_profile() or {}
        applied_tweaks = await optimizer.temporary_tweaks.list_active()
        catalog = {item["id"]: item for item in optimizer.catalog(snapshot=None, session_mode=state.get("session_mode", "normal"))}

        report_text = self._render_report(
            profile=profile,
            state=state,
            health_history=health_history,
            applied_tweaks=applied_tweaks,
            catalog=catalog,
        )

        self.documents_dir.mkdir(parents=True, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_path = self.documents_dir / f"PulseBoost_Report_{timestamp}.md"
        report_path.write_text(report_text, encoding="utf-8")
        return report_path

    def _render_report(
        self,
        *,
        profile: dict[str, Any],
        state: dict[str, Any],
        health_history: list[dict[str, Any]],
        applied_tweaks: list[dict[str, Any]],
        catalog: dict[str, dict[str, Any]],
    ) -> str:
        generated_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        health_score = float(state.get("health_score") or 0.0)
        pending_issues = state.get("anomalies") or []

        spec_lines = [
            f"- Machine: {profile.get('machine_name') or 'unknown'}",
            f"- OS: {profile.get('os_name') or 'unknown'} {profile.get('os_version') or ''}".strip(),
            f"- CPU: {profile.get('cpu_name') or 'unknown'}",
            f"- GPU: {profile.get('gpu_model') or 'unknown'}",
            f"- RAM: {self._format_ram_gb(profile.get('ram_total_bytes'))}",
            f"- Captured at: {self._format_ts(profile.get('captured_at'))}",
        ]

        tweak_lines: list[str] = []
        for tweak in applied_tweaks:
            tweak_id = str(tweak.get("tweak_id") or "unknown")
            meta = catalog.get(tweak_id, {})
            tweak_lines.append(
                f"- `{tweak_id}`: {meta.get('name') or 'Unknown tweak'} - "
                f"{meta.get('rationale') or 'No rationale available.'}"
            )
        if not tweak_lines:
            tweak_lines.append("- No temporary tweaks are currently applied.")

        history_lines: list[str] = []
        for point in health_history[:200]:
            score_value = point.get("score", point.get("health_score"))
            cpu_value = point.get("cpu_load", point.get("cpu_total"))
            history_lines.append(
                f"- {self._format_ts(point.get('timestamp'))}: "
                f"score={self._format_float(score_value)}, "
                f"cpu={self._format_float(cpu_value)}%, "
                f"ram={self._format_float(point.get('ram_percent'))}%"
            )
        if not history_lines:
            history_lines.append("- No health history is available for the last 7 days.")

        issue_lines: list[str] = []
        for issue in pending_issues[:50]:
            issue_lines.append(
                f"- [{issue.get('severity') or 'UNKNOWN'}] {issue.get('metric') or 'unknown'}: "
                f"value={self._format_float(issue.get('current_value'))}, "
                f"expected={self._format_float(issue.get('expected_mean'))}"
            )
        if not issue_lines:
            issue_lines.append("- No active issues are currently flagged.")

        return "\n".join(
            [
                "# PulseBoost System Report",
                "",
                f"Generated at: {generated_at}",
                "",
                "## System Specs",
                *spec_lines,
                "",
                "## Current Health Score",
                f"- Score: {health_score:.1f}/100",
                "",
                "## Applied Tweaks",
                *tweak_lines,
                "",
                "## Last 7-Day Health History",
                *history_lines,
                "",
                "## Pending Issues",
                *issue_lines,
                "",
            ]
        )

    @staticmethod
    def _format_float(value: Any) -> str:
        try:
            if value is None:
                return "n/a"
            return f"{float(value):.1f}"
        except (TypeError, ValueError):
            return "n/a"

    @staticmethod
    def _format_ram_gb(value: Any) -> str:
        try:
            if value is None:
                return "unknown"
            return f"{float(value) / (1024 ** 3):.1f} GB"
        except (TypeError, ValueError):
            return "unknown"

    @staticmethod
    def _format_ts(value: Any) -> str:
        try:
            if value is None:
                return "unknown"
            return datetime.fromtimestamp(float(value)).strftime("%Y-%m-%d %H:%M:%S")
        except (TypeError, ValueError, OSError):
            return "unknown"
