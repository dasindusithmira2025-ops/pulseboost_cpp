"""
PulseBoost smart suggestion engine.
"""
from __future__ import annotations

from typing import Any


def _float(value: Any) -> float | None:
    try:
        if value is None:
            return None
        return float(value)
    except (TypeError, ValueError):
        return None


def generate_smart_suggestions(
    *,
    metrics: dict[str, Any],
    session_mode: str,
    thermal_state: dict[str, Any] | None = None,
    active_session: dict[str, Any] | None = None,
) -> list[dict[str, Any]]:
    suggestions: list[dict[str, Any]] = []
    thermal_state = thermal_state or {}
    active_session = active_session or {}

    cpu_load = _float(metrics.get("cpu_total"))
    ram_percent = _float(metrics.get("ram_percent"))
    gpu_temp = _float(metrics.get("gpu_temp")) or _float(thermal_state.get("gpu_temp"))

    if session_mode == "normal" and cpu_load is not None and cpu_load > 30:
        suggestions.append(
            {
                "id": "cpu_idle_high",
                "severity": "warning",
                "text": "Check background processes - something is consuming CPU at idle.",
                "action": {"label": "Fix This", "target": "audit"},
            }
        )

    if gpu_temp is not None and gpu_temp > 83:
        suggestions.append(
            {
                "id": "gpu_temp_high",
                "severity": "warning",
                "text": "GPU running hot - check fan curves or airflow.",
                "action": {"label": "Fix This", "target": "pulseai", "prompt": "My GPU temperature is high. Help me cool it safely."},
            }
        )

    if ram_percent is not None and ram_percent > 80:
        suggestions.append(
            {
                "id": "ram_pressure",
                "severity": "warning",
                "text": "RAM pressure detected - consider closing background apps.",
                "action": {"label": "Fix This", "target": "audit"},
            }
        )

    ram_speed_current = _float(metrics.get("ram_speed_mhz"))
    ram_speed_spec = _float(metrics.get("ram_speed_spec_mhz"))
    if ram_speed_current is not None and ram_speed_spec is not None and ram_speed_current < (ram_speed_spec * 0.9):
        suggestions.append(
            {
                "id": "xmp_not_enabled",
                "severity": "info",
                "text": "Your RAM may not be running at full speed - check XMP/DOCP in BIOS.",
                "action": {"label": "Fix This", "target": "pulseai", "prompt": "How do I safely enable XMP/DOCP on my motherboard?"},
            }
        )

    pagefile_drive = str(metrics.get("pagefile_drive") or "").upper()
    is_system_ssd = bool(metrics.get("system_drive_is_ssd"))
    if ram_percent is not None and ram_percent > 80 and pagefile_drive.startswith("C:") and is_system_ssd:
        suggestions.append(
            {
                "id": "pagefile_ssd_pressure",
                "severity": "info",
                "text": "Pagefile is active on your SSD - heavy use can reduce drive lifespan.",
                "action": {"label": "Fix This", "target": "pulseai", "prompt": "Help me tune pagefile settings for high RAM pressure."},
            }
        )

    active_game = str(active_session.get("game_name") or "").strip()
    active_profile = str(active_session.get("profile_name") or active_session.get("profile") or "").strip()
    if active_game and not active_profile:
        suggestions.append(
            {
                "id": "missing_game_profile",
                "severity": "info",
                "text": f"We detected {active_game} but no profile exists - create one for better performance.",
                "action": {"label": "Fix This", "target": "profiles"},
            }
        )

    return suggestions[:3]
