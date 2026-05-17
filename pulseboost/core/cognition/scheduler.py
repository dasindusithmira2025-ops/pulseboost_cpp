"""
PulseBoost intelligent scheduler.
"""
from __future__ import annotations

import time
from collections import Counter
from datetime import datetime


class IntelligentScheduler:
    DEFERRABLE_PROCESSES = {
        "MsMpEng.exe",
        "TiWorker.exe",
        "SearchIndexer.exe",
        "SgrmBroker.exe",
        "OneDrive.exe",
        "Dropbox.exe",
        "GoogleDriveFS.exe",
    }

    def __init__(self, memory):
        self.memory = memory
        self.deferred_queue: list[dict] = []
        self.idle_hours: list[int] = []

    def evaluate(self, snapshot) -> dict:
        is_idle = snapshot.cpu_total < 8 and snapshot.ram_percent < 55
        hour = datetime.now().hour

        if is_idle:
            self.idle_hours.append(hour)
            self.idle_hours = self.idle_hours[-96:]

        candidates = []
        if not is_idle:
            for proc in snapshot.top_processes:
                if proc.get("name") in self.DEFERRABLE_PROCESSES and proc.get("cpu_percent", 0.0) > 4:
                    candidates.append(proc)

        if candidates:
            best_window = self._best_idle_hour()
            for proc in candidates[:3]:
                self.queue_task(
                    task_type="defer_process",
                    task_detail={"pid": proc["pid"], "name": proc["name"]},
                    scheduled_for=f"{best_window:02d}:00",
                )

        return {
            "is_idle": is_idle,
            "deferral_candidates": candidates,
            "optimal_maintenance_window": f"{self._best_idle_hour():02d}:00",
            "queued_tasks": list(self.deferred_queue),
        }

    def queue_task(self, task_type: str, task_detail: dict, scheduled_for: str) -> None:
        exists = next(
            (
                item
                for item in self.deferred_queue
                if item["task_type"] == task_type and item["task_detail"] == task_detail and item["status"] == "queued"
            ),
            None,
        )
        if exists:
            return
        self.deferred_queue.append(
            {
                "id": f"task_{int(time.time() * 1000)}_{len(self.deferred_queue)}",
                "created_at": time.time(),
                "task_type": task_type,
                "task_detail": task_detail,
                "scheduled_for": scheduled_for,
                "status": "queued",
            }
        )
        self.deferred_queue = self.deferred_queue[-20:]

    def _best_idle_hour(self) -> int:
        if not self.idle_hours:
            return 23
        counts = Counter(self.idle_hours)
        return counts.most_common(1)[0][0]
