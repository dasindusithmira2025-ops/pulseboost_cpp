"""
PulseBoost memory system.
"""
from __future__ import annotations

import csv
import hashlib
import io
import json
import math
import time
import uuid
import asyncio
from collections import deque
from pathlib import Path
from typing import Any

import aiosqlite
import numpy as np
from core.db.migrations import ensure_database_schema

try:
    import faiss  # type: ignore
except ImportError:  # pragma: no cover
    faiss = None

try:
    from sentence_transformers import SentenceTransformer
except ImportError:  # pragma: no cover
    SentenceTransformer = None


class MemorySystem:
    def __init__(self, db_path: str | Path = "data/memory.db", vector_path: str | Path = "data/vectors"):
        self.db_path = Path(db_path)
        self.vector_path = Path(vector_path)
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self.vector_path.mkdir(parents=True, exist_ok=True)
        self.working = deque(maxlen=2048)
        self._encoder = None
        self._vector_dimension = 384
        self._faiss_index = None
        self._faiss_meta: list[dict[str, Any]] = []

    async def initialize(self) -> None:
        await self._init_db()
        self._init_vector_store()

    async def _init_db(self) -> None:
        await asyncio.to_thread(ensure_database_schema, self.db_path)

    def _init_vector_store(self) -> None:
        index_path = self.vector_path / "events.index"
        meta_path = self.vector_path / "events_meta.json"
        if meta_path.exists():
            self._faiss_meta = json.loads(meta_path.read_text(encoding="utf-8"))
        if faiss and index_path.exists():
            self._faiss_index = faiss.read_index(str(index_path))
        elif faiss:
            self._faiss_index = faiss.IndexFlatL2(self._vector_dimension)

    def push_snapshot(self, snapshot) -> None:
        self.working.append(snapshot)

    def recent(self, limit: int = 20) -> list:
        return list(self.working)[-limit:]

    async def store_snapshot(self, snapshot, sample_period: int = 3, bucket_start: float | None = None) -> None:
        payload = snapshot.to_dict() if hasattr(snapshot, "to_dict") else dict(snapshot)
        cpu_per_core = payload["cpu_per_core"]
        async with aiosqlite.connect(self.db_path) as db:
            await db.execute(
                """
                INSERT INTO metrics (
                    timestamp, bucket_start, cpu_total, cpu_per_core, ram_used, ram_total, ram_percent,
                    swap_used, disk_read, disk_write, disk_percent, net_sent, net_recv,
                    temperature, health_score, session_mode, sample_period
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    payload["timestamp"],
                    bucket_start,
                    payload["cpu_total"],
                    json.dumps(cpu_per_core),
                    payload["ram_used"],
                    payload["ram_total"],
                    payload["ram_percent"],
                    payload["swap_used"],
                    payload["disk_read_bytes"],
                    payload["disk_write_bytes"],
                    payload["disk_percent"],
                    payload["net_sent_bytes"],
                    payload["net_recv_bytes"],
                    payload["temperature"],
                    payload["health_score"],
                    payload["session_mode"],
                    sample_period,
                ),
            )
            await db.commit()

    async def store_anomalies(self, anomalies: list[dict]) -> None:
        if not anomalies:
            return
        async with aiosqlite.connect(self.db_path) as db:
            await db.executemany(
                """
                INSERT INTO anomalies (
                    timestamp, metric, current_value, expected_mean, expected_std,
                    deviation_score, severity, action_taken
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                [
                    (
                        item["timestamp"],
                        item["metric"],
                        item["current_value"],
                        item["expected_mean"],
                        item["expected_std"],
                        item["deviation_score"],
                        item["severity"],
                        json.dumps(item.get("action_taken")) if item.get("action_taken") else None,
                    )
                    for item in anomalies
                ],
            )
            await db.commit()

    async def get_baseline(self, metric: str, hour_of_day: int, session_mode: str) -> dict | None:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT mean, std_dev, sample_count, last_updated
                FROM baselines
                WHERE metric = ? AND hour_of_day = ? AND session_mode = ?
                """,
                (metric, hour_of_day, session_mode),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "mean": row[0],
            "std_dev": row[1],
            "sample_count": row[2],
            "last_updated": row[3],
        }

    async def baseline_overview(self, session_mode: str) -> dict[str, dict[str, float]]:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT metric, AVG(sample_count), MAX(sample_count)
                FROM baselines
                WHERE session_mode = ?
                GROUP BY metric
                """,
                (session_mode,),
            )
            rows = await cursor.fetchall()
        return {
            row[0]: {
                "avg_sample_count": float(row[1]),
                "max_sample_count": float(row[2]),
            }
            for row in rows
        }

    async def update_baseline(self, metric: str, hour_of_day: int, session_mode: str, value: float, timestamp: float) -> None:
        baseline = await self.get_baseline(metric, hour_of_day, session_mode)
        if not baseline:
            mean = value
            std_dev = 0.0
            sample_count = 1
        else:
            sample_count = int(baseline["sample_count"]) + 1
            old_mean = float(baseline["mean"])
            old_std = float(baseline["std_dev"])
            old_m2 = max((sample_count - 2) * (old_std**2), 0.0)
            delta = value - old_mean
            mean = old_mean + delta / sample_count
            delta2 = value - mean
            m2 = old_m2 + delta * delta2
            variance = m2 / max(sample_count - 1, 1)
            std_dev = math.sqrt(max(variance, 0.0))

        async with aiosqlite.connect(self.db_path) as db:
            await db.execute(
                """
                INSERT INTO baselines (metric, hour_of_day, session_mode, mean, std_dev, sample_count, last_updated)
                VALUES (?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(metric, hour_of_day, session_mode)
                DO UPDATE SET mean = excluded.mean,
                              std_dev = excluded.std_dev,
                              sample_count = excluded.sample_count,
                              last_updated = excluded.last_updated
                """,
                (metric, hour_of_day, session_mode, mean, std_dev, sample_count, timestamp),
            )
            await db.commit()

    async def store_action(
        self,
        action_type: str,
        action_detail: dict,
        trigger_reason: str,
        ai_reasoning: str,
        health_before: float,
        health_after: float | None = None,
        score_delta: float | None = None,
        success: bool | None = None,
        error_message: str | None = None,
    ) -> None:
        async with aiosqlite.connect(self.db_path) as db:
            await db.execute(
                """
                INSERT INTO actions (
                    timestamp, action_type, action_detail, trigger_reason, ai_reasoning,
                    health_before, health_after, score_delta, success, error_message
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    time.time(),
                    action_type,
                    json.dumps(action_detail),
                    trigger_reason,
                    ai_reasoning,
                    health_before,
                    health_after,
                    score_delta,
                    None if success is None else int(success),
                    error_message,
                ),
            )
            await db.commit()

    async def recent_actions(self, limit: int = 20, include_all: bool = True) -> list[dict]:
        query = """
            SELECT timestamp, action_type, action_detail, trigger_reason,
                   ai_reasoning, health_before, health_after, score_delta,
                   success, error_message
            FROM actions
        """
        if not include_all:
            query += " WHERE success = 1"
        query += " ORDER BY timestamp DESC LIMIT ?"
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(query, (limit,))
            rows = await cursor.fetchall()
        return [
            {
                "timestamp": row[0],
                "action_type": row[1],
                "action_detail": json.loads(row[2]),
                "trigger_reason": row[3],
                "ai_reasoning": row[4],
                "health_before": row[5],
                "health_after": row[6],
                "score_delta": row[7],
                "success": None if row[8] is None else bool(row[8]),
                "error_message": row[9],
            }
            for row in rows
        ]

    async def export_actions_csv(self, limit: int = 500) -> str:
        rows = await self.recent_actions(limit=limit)
        buffer = io.StringIO()
        writer = csv.writer(buffer)
        writer.writerow(
            [
                "timestamp",
                "action_type",
                "trigger_reason",
                "health_before",
                "health_after",
                "score_delta",
                "success",
                "error_message",
            ]
        )
        for row in rows:
            writer.writerow(
                [
                    row["timestamp"],
                    row["action_type"],
                    row["trigger_reason"],
                    row["health_before"],
                    row["health_after"],
                    row["score_delta"],
                    row["success"],
                    row["error_message"],
                ]
            )
        return buffer.getvalue()

    async def upsert_learning(self, action_type: str, context_hash: str, score_delta: float) -> None:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT avg_score_delta, success_count, failure_count
                FROM learning
                WHERE action_type = ? AND context_hash = ?
                """,
                (action_type, context_hash),
            )
            existing = await cursor.fetchone()
            success = int(score_delta > 0)
            failure = int(score_delta < 0)
            if not existing:
                confidence = 0.5 + (0.1 if success else 0.0)
                await db.execute(
                    """
                    INSERT INTO learning (
                        action_type, context_hash, avg_score_delta, success_count,
                        failure_count, last_used, confidence
                    ) VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (action_type, context_hash, score_delta, success, failure, time.time(), confidence),
                )
            else:
                avg_delta, success_count, failure_count = existing
                total = success_count + failure_count + 1
                new_avg = ((avg_delta * (total - 1)) + score_delta) / total
                new_success = success_count + success
                new_failure = failure_count + failure
                confidence = min(0.99, max(0.1, 0.5 + (new_success - new_failure) * 0.05))
                await db.execute(
                    """
                    UPDATE learning
                    SET avg_score_delta = ?, success_count = ?, failure_count = ?,
                        last_used = ?, confidence = ?
                    WHERE action_type = ? AND context_hash = ?
                    """,
                    (new_avg, new_success, new_failure, time.time(), confidence, action_type, context_hash),
                )
            await db.commit()

    async def fetch_learning(self, action_type: str | None = None, limit: int = 20) -> list[dict]:
        query = """
            SELECT action_type, context_hash, avg_score_delta, success_count,
                   failure_count, last_used, confidence
            FROM learning
        """
        params: tuple[Any, ...] = ()
        if action_type:
            query += " WHERE action_type = ?"
            params = (action_type,)
        query += " ORDER BY confidence DESC, avg_score_delta DESC LIMIT ?"
        params = params + (limit,)
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(query, params)
            rows = await cursor.fetchall()
        return [
            {
                "action_type": row[0],
                "context_hash": row[1],
                "avg_score_delta": row[2],
                "success_count": row[3],
                "failure_count": row[4],
                "last_used": row[5],
                "confidence": row[6],
            }
            for row in rows
        ]

    async def timeline(self, limit: int = 300, window_seconds: int | None = None) -> list[dict]:
        query = """
            SELECT timestamp, sample_period, cpu_total, ram_percent, disk_percent,
                   net_sent, net_recv, temperature, health_score, session_mode
            FROM metrics
        """
        params: list[Any] = []
        if window_seconds is not None:
            query += " WHERE timestamp >= ?"
            params.append(time.time() - window_seconds)
        query += " ORDER BY timestamp DESC LIMIT ?"
        params.append(limit)
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(query, tuple(params))
            rows = await cursor.fetchall()
        return [
            {
                "timestamp": row[0],
                "sample_period": row[1],
                "cpu_total": row[2],
                "ram_percent": row[3],
                "disk_percent": row[4],
                "net_sent": row[5],
                "net_recv": row[6],
                "temperature": row[7],
                "health_score": row[8],
                "session_mode": row[9],
            }
            for row in reversed(rows)
        ]

    async def snapshot_nearest(self, timestamp: float) -> dict | None:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT timestamp, cpu_total, ram_percent, disk_percent, net_sent, net_recv,
                       temperature, health_score, session_mode, sample_period
                FROM metrics
                ORDER BY ABS(timestamp - ?)
                LIMIT 1
                """,
                (timestamp,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "timestamp": row[0],
            "cpu_total": row[1],
            "ram_percent": row[2],
            "disk_percent": row[3],
            "net_sent": row[4],
            "net_recv": row[5],
            "temperature": row[6],
            "health_score": row[7],
            "session_mode": row[8],
            "sample_period": row[9],
        }

    async def recent_anomalies(self, limit: int = 10, unresolved_only: bool = False) -> list[dict]:
        query = """
            SELECT timestamp, metric, current_value, expected_mean, expected_std,
                   deviation_score, severity, resolved, action_taken
            FROM anomalies
        """
        if unresolved_only:
            query += " WHERE resolved = 0"
        query += " ORDER BY timestamp DESC LIMIT ?"
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(query, (limit,))
            rows = await cursor.fetchall()
        items = []
        for row in rows:
            items.append(
                {
                    "timestamp": row[0],
                    "metric": row[1],
                    "current_value": row[2],
                    "expected_mean": row[3],
                    "expected_std": row[4],
                    "deviation_score": row[5],
                    "severity": row[6],
                    "resolved": bool(row[7]),
                    "action_taken": json.loads(row[8]) if row[8] else None,
                }
            )
        return items

    async def store_optimizations(
        self,
        optimizations: list[dict],
        *,
        efficiency_before: float | None = None,
        efficiency_after: float | None = None,
    ) -> None:
        if not optimizations:
            return
        async with aiosqlite.connect(self.db_path) as db:
            for item in optimizations:
                await db.execute(
                    """
                    INSERT INTO optimizations (
                        external_id, timestamp, pillar, title, finding, impact, action, params, risk,
                        auto_executed, user_approved, status, estimated_gain, actual_gain,
                        efficiency_before, efficiency_after
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    ON CONFLICT(external_id)
                    DO UPDATE SET
                        timestamp = excluded.timestamp,
                        pillar = excluded.pillar,
                        title = excluded.title,
                        finding = excluded.finding,
                        impact = excluded.impact,
                        action = excluded.action,
                        params = excluded.params,
                        risk = excluded.risk,
                        auto_executed = excluded.auto_executed,
                        status = CASE
                            WHEN optimizations.status IN ('done', 'dismissed') THEN optimizations.status
                            ELSE excluded.status
                        END,
                        estimated_gain = excluded.estimated_gain,
                        actual_gain = COALESCE(excluded.actual_gain, optimizations.actual_gain),
                        efficiency_before = COALESCE(excluded.efficiency_before, optimizations.efficiency_before),
                        efficiency_after = COALESCE(excluded.efficiency_after, optimizations.efficiency_after)
                    """,
                    (
                        item.get("id"),
                        item.get("timestamp", time.time()),
                        item["pillar"],
                        item["title"],
                        item["finding"],
                        item["impact"],
                        item["action"],
                        json.dumps(item.get("params", {})),
                        item.get("risk", "SAFE"),
                        int(bool(item.get("auto_execute"))),
                        None,
                        item.get("status", "pending"),
                        item.get("estimated_gain"),
                        item.get("actual_gain"),
                        efficiency_before,
                        efficiency_after,
                    ),
                )
            await db.commit()

    async def recent_optimizations(self, limit: int = 25) -> list[dict]:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT external_id, timestamp, pillar, title, finding, impact, action, params, risk,
                       auto_executed, user_approved, status, estimated_gain, actual_gain,
                       efficiency_before, efficiency_after
                FROM optimizations
                ORDER BY timestamp DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [
            {
                "id": row[0],
                "timestamp": row[1],
                "pillar": row[2],
                "title": row[3],
                "finding": row[4],
                "impact": row[5],
                "action": row[6],
                "params": json.loads(row[7]),
                "risk": row[8],
                "auto_execute": bool(row[9]),
                "user_approved": None if row[10] is None else bool(row[10]),
                "status": row[11],
                "estimated_gain": row[12],
                "actual_gain": row[13],
                "efficiency_before": row[14],
                "efficiency_after": row[15],
            }
            for row in rows
        ]

    async def get_optimization(self, external_id: str) -> dict | None:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT external_id, timestamp, pillar, title, finding, impact, action, params, risk,
                       auto_executed, user_approved, status, estimated_gain, actual_gain,
                       efficiency_before, efficiency_after
                FROM optimizations
                WHERE external_id = ?
                LIMIT 1
                """,
                (external_id,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "id": row[0],
            "timestamp": row[1],
            "pillar": row[2],
            "title": row[3],
            "finding": row[4],
            "impact": row[5],
            "action": row[6],
            "params": json.loads(row[7]),
            "risk": row[8],
            "auto_execute": bool(row[9]),
            "user_approved": None if row[10] is None else bool(row[10]),
            "status": row[11],
            "estimated_gain": row[12],
            "actual_gain": row[13],
            "efficiency_before": row[14],
            "efficiency_after": row[15],
        }

    async def set_optimization_status(
        self,
        external_id: str,
        *,
        status: str,
        user_approved: bool | None = None,
        actual_gain: str | None = None,
        efficiency_after: float | None = None,
    ) -> None:
        async with aiosqlite.connect(self.db_path) as db:
            await db.execute(
                """
                UPDATE optimizations
                SET status = ?,
                    user_approved = COALESCE(?, user_approved),
                    actual_gain = COALESCE(?, actual_gain),
                    efficiency_after = COALESCE(?, efficiency_after)
                WHERE external_id = ?
                """,
                (
                    status,
                    None if user_approved is None else int(user_approved),
                    actual_gain,
                    efficiency_after,
                    external_id,
                ),
            )
            await db.commit()

    async def store_process_intelligence(self, process_rows: list[dict], timestamp: float) -> None:
        if not process_rows:
            return
        async with aiosqlite.connect(self.db_path) as db:
            await db.executemany(
                """
                INSERT INTO process_intelligence (
                    timestamp, pid, name, cpu_percent, ram_bytes, waste_score, anomaly_level, is_leak_candidate
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                [
                    (
                        timestamp,
                        row["pid"],
                        row["name"],
                        row["cpu_percent"],
                        row["ram_bytes"],
                        row["waste_score"],
                        row.get("anomaly_level"),
                        int(row.get("anomaly_level") == "LEAK"),
                    )
                    for row in process_rows
                ],
            )
            await db.commit()

    async def latest_process_intelligence(self, limit: int = 20) -> list[dict]:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT p1.timestamp, p1.pid, p1.name, p1.cpu_percent, p1.ram_bytes, p1.waste_score,
                       p1.anomaly_level, p1.is_leak_candidate
                FROM process_intelligence p1
                INNER JOIN (
                    SELECT pid, MAX(timestamp) AS max_timestamp
                    FROM process_intelligence
                    GROUP BY pid
                ) latest
                ON latest.pid = p1.pid AND latest.max_timestamp = p1.timestamp
                ORDER BY p1.waste_score DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [
            {
                "timestamp": row[0],
                "pid": row[1],
                "name": row[2],
                "cpu_percent": row[3],
                "ram_bytes": row[4],
                "waste_score": row[5],
                "anomaly_level": row[6],
                "is_leak_candidate": bool(row[7]),
            }
            for row in rows
        ]

    async def store_scheduled_tasks(self, queued_tasks: list[dict]) -> None:
        if not queued_tasks:
            return
        async with aiosqlite.connect(self.db_path) as db:
            for task in queued_tasks:
                detail_json = json.dumps(task.get("task_detail", {}), sort_keys=True)
                existing = await db.execute(
                    """
                    SELECT 1
                    FROM scheduled_tasks
                    WHERE task_type = ? AND task_detail = ? AND scheduled_for = ? AND status = ?
                    LIMIT 1
                    """,
                    (
                        task["task_type"],
                        detail_json,
                        task["scheduled_for"],
                        task.get("status", "queued"),
                    ),
                )
                if await existing.fetchone():
                    continue
                await db.execute(
                    """
                    INSERT INTO scheduled_tasks (created_at, task_type, task_detail, scheduled_for, status, completed_at)
                    VALUES (?, ?, ?, ?, ?, ?)
                    """,
                    (
                        task.get("created_at", time.time()),
                        task["task_type"],
                        detail_json,
                        task["scheduled_for"],
                        task.get("status", "queued"),
                        task.get("completed_at"),
                    ),
                )
            await db.commit()

    async def recent_scheduled_tasks(self, limit: int = 20) -> list[dict]:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT created_at, task_type, task_detail, scheduled_for, status, completed_at
                FROM scheduled_tasks
                ORDER BY created_at DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [
            {
                "created_at": row[0],
                "task_type": row[1],
                "task_detail": json.loads(row[2]),
                "scheduled_for": row[3],
                "status": row[4],
                "completed_at": row[5],
            }
            for row in rows
        ]

    async def store_efficiency_snapshot(self, timestamp: float, efficiency: dict[str, Any]) -> None:
        breakdown = efficiency.get("breakdown", {})
        async with aiosqlite.connect(self.db_path) as db:
            await db.execute(
                """
                INSERT INTO efficiency_history (
                    timestamp, overall_score, cpu_score, ram_score, disk_score, network_score, thermal_score
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    timestamp,
                    efficiency.get("score", 100.0),
                    breakdown.get("cpu", 100.0),
                    breakdown.get("ram", 100.0),
                    breakdown.get("disk", 100.0),
                    breakdown.get("network", 100.0),
                    breakdown.get("thermal", 100.0),
                ),
            )
            await db.commit()

    async def mark_anomaly_resolved(self, anomaly_timestamp: float, metric: str, action_taken: dict | None = None) -> None:
        async with aiosqlite.connect(self.db_path) as db:
            await db.execute(
                """
                UPDATE anomalies
                SET resolved = 1,
                    resolved_at = ?,
                    action_taken = COALESCE(?, action_taken)
                WHERE timestamp = ? AND metric = ? AND resolved = 0
                """,
                (time.time(), json.dumps(action_taken) if action_taken else None, anomaly_timestamp, metric),
            )
            await db.commit()

    async def increment_usage(self, counter_key: str, counter_date: str) -> int:
        async with aiosqlite.connect(self.db_path) as db:
            await db.execute(
                """
                INSERT INTO usage_counters (counter_date, counter_key, value)
                VALUES (?, ?, 1)
                ON CONFLICT(counter_date, counter_key)
                DO UPDATE SET value = value + 1
                """,
                (counter_date, counter_key),
            )
            cursor = await db.execute(
                "SELECT value FROM usage_counters WHERE counter_date = ? AND counter_key = ?",
                (counter_date, counter_key),
            )
            row = await cursor.fetchone()
            await db.commit()
        return int(row[0])

    async def usage_value(self, counter_key: str, counter_date: str) -> int:
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                "SELECT value FROM usage_counters WHERE counter_date = ? AND counter_key = ?",
                (counter_date, counter_key),
            )
            row = await cursor.fetchone()
        return int(row[0]) if row else 0

    async def enforce_retention(
        self,
        *,
        keep_3s_hours: int = 24,
        keep_1m_days: int = 7,
        keep_5m_days: int = 30,
    ) -> dict[str, int]:
        now = time.time()
        cutoff_3s = now - (keep_3s_hours * 3600)
        cutoff_1m = now - (keep_1m_days * 86400)
        cutoff_5m = now - (keep_5m_days * 86400)

        created_1m = await self._rollup_window(cutoff_1m, cutoff_3s, bucket_size=60, source_period=3)
        created_5m = await self._rollup_window(cutoff_5m, cutoff_1m, bucket_size=300, source_period=60)

        async with aiosqlite.connect(self.db_path) as db:
            await db.execute("DELETE FROM metrics WHERE sample_period = 3 AND timestamp < ?", (cutoff_3s,))
            await db.execute("DELETE FROM metrics WHERE sample_period = 60 AND timestamp < ?", (cutoff_1m,))
            await db.execute("DELETE FROM metrics WHERE timestamp < ?", (cutoff_5m,))
            await db.commit()

        return {"rollup_1m": created_1m, "rollup_5m": created_5m}

    async def _rollup_window(self, start_ts: float, end_ts: float, *, bucket_size: int, source_period: int) -> int:
        if end_ts <= start_ts:
            return 0
        async with aiosqlite.connect(self.db_path) as db:
            cursor = await db.execute(
                """
                SELECT timestamp, cpu_total, cpu_per_core, ram_used, ram_total, ram_percent, swap_used,
                       disk_read, disk_write, disk_percent, net_sent, net_recv, temperature,
                       health_score, session_mode
                FROM metrics
                WHERE timestamp >= ? AND timestamp < ? AND sample_period = ?
                ORDER BY timestamp ASC
                """,
                (start_ts, end_ts, source_period),
            )
            rows = await cursor.fetchall()

            buckets: dict[int, list] = {}
            for row in rows:
                bucket_start = int(row[0] // bucket_size) * bucket_size
                buckets.setdefault(bucket_start, []).append(row)

            created = 0
            for bucket_start, items in buckets.items():
                check = await db.execute(
                    "SELECT 1 FROM metrics WHERE bucket_start = ? AND sample_period = ? LIMIT 1",
                    (bucket_start, bucket_size),
                )
                if await check.fetchone():
                    continue
                aggregated = self._aggregate_metric_rows(items)
                await db.execute(
                    """
                    INSERT INTO metrics (
                        timestamp, bucket_start, cpu_total, cpu_per_core, ram_used, ram_total,
                        ram_percent, swap_used, disk_read, disk_write, disk_percent, net_sent,
                        net_recv, temperature, health_score, session_mode, sample_period
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        aggregated["timestamp"],
                        bucket_start,
                        aggregated["cpu_total"],
                        json.dumps(aggregated["cpu_per_core"]),
                        aggregated["ram_used"],
                        aggregated["ram_total"],
                        aggregated["ram_percent"],
                        aggregated["swap_used"],
                        aggregated["disk_read_bytes"],
                        aggregated["disk_write_bytes"],
                        aggregated["disk_percent"],
                        aggregated["net_sent_bytes"],
                        aggregated["net_recv_bytes"],
                        aggregated["temperature"],
                        aggregated["health_score"],
                        aggregated["session_mode"],
                        bucket_size,
                    ),
                )
                created += 1
            await db.commit()
        return created

    def _aggregate_metric_rows(self, rows: list[tuple]) -> dict[str, Any]:
        latest = rows[-1]
        cpu_per_core_entries = [json.loads(item[2]) for item in rows]
        max_cores = max(len(item) for item in cpu_per_core_entries)
        core_avgs = []
        for index in range(max_cores):
            values = [entry[index] for entry in cpu_per_core_entries if index < len(entry)]
            core_avgs.append(round(sum(values) / len(values), 2))

        session_mode = latest[14]
        return {
            "timestamp": latest[0],
            "cpu_total": round(sum(item[1] for item in rows) / len(rows), 2),
            "cpu_per_core": core_avgs,
            "ram_used": round(sum(item[3] for item in rows) / len(rows), 2),
            "ram_total": latest[4],
            "ram_percent": round(sum(item[5] for item in rows) / len(rows), 2),
            "swap_used": round(sum(item[6] for item in rows) / len(rows), 2),
            "disk_read_bytes": round(sum(item[7] for item in rows) / len(rows), 2),
            "disk_write_bytes": round(sum(item[8] for item in rows) / len(rows), 2),
            "disk_percent": round(sum(item[9] for item in rows) / len(rows), 2),
            "net_sent_bytes": round(sum(item[10] for item in rows) / len(rows), 2),
            "net_recv_bytes": round(sum(item[11] for item in rows) / len(rows), 2),
            "temperature": round(sum(item[12] for item in rows if item[12] is not None) / max(1, len([item for item in rows if item[12] is not None])), 2)
            if any(item[12] is not None for item in rows)
            else None,
            "health_score": round(sum(item[13] for item in rows) / len(rows), 2),
            "session_mode": session_mode,
        }

    def store_event_vector(self, summary: str, score_delta: float) -> None:
        embedding = self._embed(summary)
        metadata = {
            "id": str(uuid.uuid4()),
            "timestamp": time.time(),
            "summary": summary,
            "embedding_text": summary,
            "score_delta": score_delta,
        }
        self._faiss_meta.append(metadata)
        if faiss and self._faiss_index is not None:
            self._faiss_index.add(np.array([embedding], dtype="float32"))
            faiss.write_index(self._faiss_index, str(self.vector_path / "events.index"))
        (self.vector_path / "events_meta.json").write_text(json.dumps(self._faiss_meta, indent=2), encoding="utf-8")

    def find_similar_events(self, query: str, limit: int = 3) -> list[dict]:
        if not self._faiss_meta:
            return []
        if faiss and self._faiss_index is not None and self._faiss_index.ntotal > 0:
            query_embedding = np.array([self._embed(query)], dtype="float32")
            distances, indices = self._faiss_index.search(query_embedding, limit)
            results = []
            for distance, index in zip(distances[0], indices[0]):
                if index < len(self._faiss_meta):
                    item = dict(self._faiss_meta[index])
                    item["distance"] = float(distance)
                    results.append(item)
            return results

        query_embedding = self._embed(query)
        scored = []
        for item in self._faiss_meta:
            distance = float(np.linalg.norm(self._embed(item["summary"]) - query_embedding))
            scored.append((distance, item))
        scored.sort(key=lambda pair: pair[0])
        return [{**item, "distance": distance} for distance, item in scored[:limit]]

    def _embed(self, text: str) -> np.ndarray:
        encoder = self._get_encoder()
        if encoder:
            return np.asarray(encoder.encode([text])[0], dtype="float32")
        vector = np.zeros(self._vector_dimension, dtype="float32")
        for token in text.lower().split():
            index = int(hashlib.sha256(token.encode("utf-8")).hexdigest(), 16) % self._vector_dimension
            vector[index] += 1.0
        norm = np.linalg.norm(vector)
        if norm:
            vector = vector / norm
        return vector.astype("float32")

    def _get_encoder(self):
        if self._encoder is not None:
            return self._encoder
        if not SentenceTransformer:
            return None
        try:
            self._encoder = SentenceTransformer("all-MiniLM-L6-v2")
        except Exception:  # pragma: no cover
            self._encoder = False
        return self._encoder if self._encoder is not False else None
