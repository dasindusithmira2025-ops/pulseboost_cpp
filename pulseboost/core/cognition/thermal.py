"""
PulseBoost thermal intelligence.
"""
from __future__ import annotations


class ThermalIntelligence:
    def analyze(self, snapshot, session_mode: str) -> dict:
        temperature = getattr(snapshot, "temperature", None)
        top_processes = getattr(snapshot, "top_processes", []) or []
        hottest = top_processes[0] if top_processes else None
        machine_type = "laptop" if getattr(snapshot, "ram_total", 0) and getattr(snapshot, "process_count", 0) < 250 else "desktop"

        if temperature is None:
            return {
                "throttling_active": False,
                "throttling_imminent": False,
                "primary_heat_source": hottest["name"] if hottest else None,
                "heat_justified": True,
                "recommended_power_plan": "balanced",
                "cooling_health": "unknown",
                "actions": [],
                "user_message": "Thermal sensors are not available on this machine.",
                "machine_type": machine_type,
            }

        throttling_active = temperature >= 92
        throttling_imminent = temperature >= 86
        useful_work = session_mode in {"gaming", "heavy_compute"}
        heat_justified = useful_work or (hottest and hottest.get("cpu_percent", 0.0) > 40)
        recommended_power_plan = "balanced"
        if throttling_active:
            recommended_power_plan = "power_saver"
        elif useful_work and temperature < 80:
            recommended_power_plan = "high_performance"

        cooling_health = "good"
        if throttling_active:
            cooling_health = "critical"
        elif throttling_imminent:
            cooling_health = "degraded"

        actions = []
        if hottest and throttling_imminent:
            actions.append(
                {
                    "action": "reduce_process_priority",
                    "params": {"pid": hottest["pid"], "name": hottest["name"]},
                    "reason": f"{hottest['name']} is the primary heat source",
                }
            )

        message = (
            f"CPU temperature is {temperature:.0f}°C. "
            f"{'Thermal throttling is active.' if throttling_active else 'Thermal pressure is rising.' if throttling_imminent else 'Thermals are currently stable.'}"
        )
        return {
            "throttling_active": throttling_active,
            "throttling_imminent": throttling_imminent,
            "primary_heat_source": hottest["name"] if hottest else None,
            "heat_justified": bool(heat_justified),
            "recommended_power_plan": recommended_power_plan,
            "cooling_health": cooling_health,
            "actions": actions,
            "user_message": message,
            "machine_type": machine_type,
        }
