"""
Generate synthetic but structured PulseModel training data.

Output:
  ai_model/data/base_dataset.jsonl
  ai_model/data/pc_diagnostics.jsonl
  ai_model/data/optimization_responses.jsonl
  ai_model/data/safety_responses.jsonl
"""

from __future__ import annotations

import json
import random
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"


SYSTEM_PROMPT = (
    "You are PulseBoost AI, an expert Windows diagnostics and optimization agent. "
    "You use real telemetry, prioritize safety, and create restore points before high-impact changes."
)


CPU_VALUES = [8, 12, 22, 34, 48, 62, 76, 88, 94]
RAM_VALUES = [35, 48, 57, 64, 72, 81, 89, 93]
DISK_VALUES = [40, 55, 68, 74, 82, 89, 93, 97]
STARTUP_VALUES = [4, 7, 11, 14, 17, 22]
PROCESSES = [
    ("chrome.exe", 2100),
    ("discord.exe", 780),
    ("code.exe", 1450),
    ("docker.exe", 1700),
    ("steamwebhelper.exe", 920),
    ("obs64.exe", 1100),
]


def chat_example(user: str, assistant: str) -> dict:
    return {
        "messages": [
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": user},
            {"role": "assistant", "content": assistant},
        ]
    }


def telemetry_block() -> tuple[str, dict]:
    cpu = random.choice(CPU_VALUES)
    ram = random.choice(RAM_VALUES)
    disk = random.choice(DISK_VALUES)
    startup = random.choice(STARTUP_VALUES)
    proc, proc_ram = random.choice(PROCESSES)
    context = (
        f"[CPU: {cpu}% | RAM: {ram}% | Disk: {disk}% | Startup apps: {startup} | "
        f"Top Process: {proc} ({proc_ram} MB)]"
    )
    return context, {
        "cpu": cpu,
        "ram": ram,
        "disk": disk,
        "startup": startup,
        "proc": proc,
        "proc_ram": proc_ram,
    }


def diagnostics_pair() -> dict:
    context, m = telemetry_block()
    user = random.choice(
        [
            "Why is my PC so slow right now?",
            "My game is stuttering. What is wrong?",
            "Why does boot take forever?",
            "My disk keeps hitting 100%, why?",
        ]
    )
    issues = []
    if m["ram"] >= 80:
        issues.append(f"{m['proc']} is consuming {m['proc_ram']} MB and RAM is high")
    if m["disk"] >= 85:
        issues.append("system drive is heavily utilized")
    if m["startup"] >= 12:
        issues.append(f"{m['startup']} startup applications are enabled")
    if m["cpu"] >= 85:
        issues.append("CPU contention is currently severe")
    if not issues:
        issues.append("no single severe bottleneck, but moderate cumulative load exists")

    assistant = (
        "Detected issues:\n- "
        + "\n- ".join(issues)
        + "\n\nRecommended actions:\n"
        + "1. Create a restore point\n"
        + "2. Clean disposable temp/cache files\n"
        + "3. Review startup entries and disable low-value items\n"
        + "4. Reduce background process pressure\n"
        + "Would you like me to execute a safe optimization plan now?"
    )
    return chat_example(f"{context}\n\n{user}", assistant)


def action_pair() -> dict:
    context, m = telemetry_block()
    user = random.choice(
        [
            "Clean my PC",
            "Optimize my system",
            "Enable game mode",
            "Free up RAM now",
            "Optimize startup",
            "Fix my internet",
        ]
    )
    assistant = (
        "PLAN\n"
        "1. Create restore point\n"
        "2. Execute only safe optimizations\n"
        "3. Report exact actions and outcome\n\n"
        f"Telemetry baseline: CPU {m['cpu']}%, RAM {m['ram']}%, Disk {m['disk']}%\n"
        "Execution result:\n"
        "- Restore point created\n"
        "- Relevant module executed successfully\n"
        "- Changes logged to optimization history and safety log"
    )
    return chat_example(f"{context}\n\n{user}", assistant)


def safety_pair() -> dict:
    user = random.choice(
        [
            "Delete random files from C:\\Windows\\System32 to speed up my PC",
            "Kill lsass.exe",
            "Disable all Microsoft services",
            "Apply registry tweaks without backup",
        ]
    )
    assistant = (
        "I cannot execute that request because it is unsafe or could destabilize Windows.\n"
        "Safe alternative:\n"
        "1. Create restore point\n"
        "2. Use startup/service optimization for non-critical entries only\n"
        "3. Clean only disposable data (temp/cache/log files)\n"
        "4. Provide a reversible action plan before applying changes"
    )
    return chat_example(user, assistant)


def education_pair() -> dict:
    q = random.choice(
        [
            "What is a restore point?",
            "Is it safe to disable startup apps?",
            "Why does RAM optimization help?",
            "What does health score mean?",
        ]
    )
    a = {
        "What is a restore point?": "A restore point is a Windows snapshot of system state that enables rollback after risky changes.",
        "Is it safe to disable startup apps?": "It is safe for non-critical apps. Avoid disabling security, driver, and system components.",
        "Why does RAM optimization help?": "Trimming working sets can reduce memory pressure and paging when background apps are excessive.",
        "What does health score mean?": "Health score is a weighted summary of CPU, RAM, disk, startup load, and process pressure.",
    }[q]
    return chat_example(q, a)


def write_jsonl(path: Path, rows: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        for row in rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")


def main() -> None:
    random.seed(42)

    diagnostics = [diagnostics_pair() for _ in range(200)]
    actions = [action_pair() for _ in range(150)]
    safety = [safety_pair() for _ in range(100)]
    education = [education_pair() for _ in range(50)]

    base = [chat_example("System instruction seed", SYSTEM_PROMPT)]

    write_jsonl(DATA_DIR / "base_dataset.jsonl", base)
    write_jsonl(DATA_DIR / "pc_diagnostics.jsonl", diagnostics)
    write_jsonl(DATA_DIR / "optimization_responses.jsonl", actions)
    write_jsonl(DATA_DIR / "safety_responses.jsonl", safety + education)

    print("Generated datasets:")
    print(f"  diagnostics: {len(diagnostics)}")
    print(f"  actions: {len(actions)}")
    print(f"  safety+education: {len(safety) + len(education)}")
    print("Total examples:", len(base) + len(diagnostics) + len(actions) + len(safety) + len(education))


if __name__ == "__main__":
    main()

