"""
PulseBoost safe action catalog.
"""
from __future__ import annotations

import platform
import tempfile


ACTIONS = {
    "clear_temp_files": {
        "description": "Delete files in system temp directories.",
        "risk": "SAFE",
        "auto_approve": True,
    },
    "flush_dns_cache": {
        "description": "Clear DNS resolver cache.",
        "risk": "SAFE",
        "auto_approve": True,
    },
    "drop_ram_cache": {
        "description": "Drop page cache (Linux only).",
        "risk": "SAFE",
        "auto_approve": True,
        "platform": "Linux",
    },
    "kill_process": {
        "description": "Terminate a process by PID.",
        "risk": "MODERATE",
        "auto_approve": False,
        "required_params": ["pid", "name"],
    },
    "restart_service": {
        "description": "Restart a named service.",
        "risk": "RISKY",
        "auto_approve": False,
        "required_params": ["service_name"],
    },
    "set_service_start_type": {
        "description": "Change a validated Windows service start type.",
        "risk": "MODERATE",
        "auto_approve": False,
        "required_params": ["service_name", "start_type"],
        "platform": "Windows",
    },
    "set_process_affinity": {
        "description": "Limit a process to a subset of CPU cores.",
        "risk": "MODERATE",
        "auto_approve": False,
        "required_params": ["pid", "name", "cores"],
    },
    "set_registry_value": {
        "description": "Write a validated registry-backed tweak.",
        "risk": "RISKY",
        "auto_approve": False,
        "required_params": ["path", "key", "value", "value_kind"],
        "platform": "Windows",
    },
}


def action_catalog() -> list[dict]:
    current_platform = platform.system()
    actions: list[dict] = []
    for name, config in ACTIONS.items():
        allowed_platform = config.get("platform")
        if allowed_platform and allowed_platform != current_platform:
            continue
        actions.append({"action": name, **config})
    return actions


def temp_directory() -> str:
    return tempfile.gettempdir()
