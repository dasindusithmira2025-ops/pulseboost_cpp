from __future__ import annotations

import asyncio
import json
import logging
import time
import uuid
from contextlib import asynccontextmanager, suppress
from pathlib import Path
from typing import Any

import aiosqlite

from core.db.migrations import ensure_database_schema
from core.models import (
    AuditEntry,
    BenchmarkResult,
    CapabilitySnapshot,
    HardwareProfile,
    RevertSnapshot,
    SessionRecord,
    normalize_benchmark_result_payload,
)


LOGGER = logging.getLogger(__name__)


class DBPool:
    def __init__(self, db_path: str | Path, max_connections: int = 5) -> None:
        self._path = Path(db_path).expanduser()
        self._semaphore = asyncio.Semaphore(max_connections)
        self._connections: list[aiosqlite.Connection] = []
        self._lock = asyncio.Lock()

    async def _create_connection(self) -> aiosqlite.Connection:
        connection = await aiosqlite.connect(self._path)
        await DatabaseService.configure_connection(connection)
        return connection

    async def acquire(self) -> aiosqlite.Connection:
        await self._semaphore.acquire()
        try:
            connection = await self._create_connection()
        except Exception:
            self._semaphore.release()
            raise
        async with self._lock:
            self._connections.append(connection)
        return connection

    async def release(self, connection: aiosqlite.Connection) -> None:
        try:
            if getattr(connection, "in_transaction", False):
                with suppress(Exception):
                    await connection.rollback()
            with suppress(Exception):
                await connection.close()
            async with self._lock:
                if connection in self._connections:
                    self._connections.remove(connection)
        finally:
            self._semaphore.release()

    async def close_all(self) -> None:
        async with self._lock:
            connections = list(self._connections)
            self._connections.clear()
        for connection in connections:
            with suppress(Exception):
                await connection.close()


class DatabaseService:
    def __init__(self, db_path: str | Path, *, use_pool: bool = False) -> None:
        self.db_path = Path(db_path).expanduser()
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self.legacy_artifact_path = self._default_legacy_artifact_path()
        self.migration_report: list[str] = []
        self._use_pool = use_pool
        self._pool_size = 5
        self._pool: DBPool | None = None

    async def initialize(self) -> None:
        self.migration_report = await asyncio.to_thread(
            ensure_database_schema,
            self.db_path,
            legacy_artifact_path=self.legacy_artifact_path,
        )
        if self._use_pool and self._pool is None:
            self._pool = DBPool(self.db_path, max_connections=self._pool_size)
        await self._set_metadata("runtime_identity", {"db_path": str(self.db_path)}, value_type="json", source="runtime")
        await self._set_metadata_if_missing("created_at", time.time(), value_type="real", source="runtime")
        await self._set_metadata("last_opened_at", time.time(), value_type="real", source="runtime")
        await self.mark_clean_exit(False)

    async def _create_connection(self) -> aiosqlite.Connection:
        connection = await aiosqlite.connect(self.db_path)
        await self.configure_connection(connection)
        return connection

    @staticmethod
    async def configure_connection(connection: aiosqlite.Connection) -> None:
        await connection.execute("PRAGMA journal_mode = WAL")
        await connection.execute("PRAGMA synchronous = NORMAL")
        await connection.execute("PRAGMA foreign_keys = ON")
        await connection.execute("PRAGMA busy_timeout = 15000")

    @asynccontextmanager
    async def _connection(self):
        if not self._use_pool:
            connection = await self._create_connection()
            try:
                yield connection
            finally:
                if getattr(connection, "in_transaction", False):
                    with suppress(Exception):
                        await connection.rollback()
                with suppress(Exception):
                    await connection.close()
            return

        if self._pool is None:
            self._pool = DBPool(self.db_path, max_connections=self._pool_size)
        connection = await self._pool.acquire()
        try:
            yield connection
        finally:
            await self._pool.release(connection)

    async def close(self) -> None:
        if self._pool is None:
            return
        await self._pool.close_all()
        self._pool = None

    def _default_legacy_artifact_path(self) -> Path:
        resolved = self.db_path.resolve()
        if len(resolved.parents) >= 3:
            workspace_candidate = resolved.parents[2] / "data" / resolved.name
            if workspace_candidate != resolved:
                return workspace_candidate
        if len(resolved.parents) >= 2:
            project_candidate = resolved.parents[1] / "data" / resolved.name
            if project_candidate != resolved:
                return project_candidate
        return resolved.parent / f"legacy-{resolved.name}"

    @staticmethod
    def _compat_state_meta_key(state_key: str) -> str:
        return f"compat.app_state.{state_key}"

    async def _set_metadata(self, meta_key: str, value: Any, *, value_type: str, source: str) -> None:
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO app_metadata (meta_key, meta_value, value_type, updated_at, source)
                VALUES (?, ?, ?, ?, ?)
                ON CONFLICT(meta_key)
                DO UPDATE SET meta_value = excluded.meta_value,
                              value_type = excluded.value_type,
                              updated_at = excluded.updated_at,
                              source = excluded.source
                """,
                (meta_key, json.dumps(value), value_type, time.time(), source),
            )
            await db.commit()

    async def _set_metadata_if_missing(self, meta_key: str, value: Any, *, value_type: str, source: str) -> None:
        async with self._connection() as db:
            cursor = await db.execute(
                "SELECT 1 FROM app_metadata WHERE meta_key = ? LIMIT 1",
                (meta_key,),
            )
            if await cursor.fetchone():
                return
            await db.execute(
                """
                INSERT INTO app_metadata (meta_key, meta_value, value_type, updated_at, source)
                VALUES (?, ?, ?, ?, ?)
                """,
                (meta_key, json.dumps(value), value_type, time.time(), source),
            )
            await db.commit()

    async def set_app_metadata(self, meta_key: str, value: Any, *, value_type: str = "json", source: str = "runtime") -> None:
        await self._set_metadata(meta_key, value, value_type=value_type, source=source)

    async def _set_setting(self, category: str, key_name: str, value: Any, *, source: str = "system") -> None:
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO settings (category, key_name, value_text, value_type, updated_at, source)
                VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(category, key_name)
                DO UPDATE SET value_text = excluded.value_text,
                              value_type = excluded.value_type,
                              updated_at = excluded.updated_at,
                              source = excluded.source
                """,
                (category, key_name, json.dumps(value), "json", time.time(), source),
            )
            await db.commit()

    async def _get_setting(self, category: str, key_name: str) -> Any | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT value_text
                FROM settings
                WHERE category = ? AND key_name = ?
                LIMIT 1
                """,
                (category, key_name),
            )
            row = await cursor.fetchone()
        return json.loads(row[0]) if row else None

    async def set_setting(self, key_name: str, value: Any, *, source: str = "settings_api") -> None:
        await self._set_setting("settings", key_name, value, source=source)

    async def get_setting(self, key_name: str) -> Any | None:
        return await self._get_setting("settings", key_name)

    async def list_settings(self, *, category: str = "settings") -> dict[str, Any]:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT key_name, value_text
                FROM settings
                WHERE category = ?
                ORDER BY key_name ASC
                """,
                (category,),
            )
            rows = await cursor.fetchall()
        settings: dict[str, Any] = {}
        for row in rows:
            raw_value = row[1]
            try:
                settings[row[0]] = json.loads(raw_value) if raw_value is not None else None
            except json.JSONDecodeError:
                settings[row[0]] = raw_value
        return settings

    async def mark_clean_exit(self, clean: bool) -> None:
        await self._set_metadata("clean_exit", clean, value_type="boolean", source="runtime")

    async def set_runtime_state(self, runtime_key: str, value: dict[str, Any]) -> None:
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO runtime_housekeeping (runtime_key, value_json, updated_at)
                VALUES (?, ?, ?)
                ON CONFLICT(runtime_key)
                DO UPDATE SET value_json = excluded.value_json, updated_at = excluded.updated_at
                """,
                (runtime_key, json.dumps(value), time.time()),
            )
            await db.commit()

    async def get_runtime_state(self, runtime_key: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT value_json
                FROM runtime_housekeeping
                WHERE runtime_key = ?
                LIMIT 1
                """,
                (runtime_key,),
            )
            row = await cursor.fetchone()
        return json.loads(row[0]) if row else None

    async def set_app_state(self, state_key: str, value: dict[str, Any]) -> None:
        if state_key.startswith("settings."):
            await self._set_setting("settings", state_key.split(".", 1)[1], value, source="compat_set_app_state")
            return
        if state_key.startswith("runtime."):
            await self.set_runtime_state(state_key.split(".", 1)[1], value)
            return
        await self._set_metadata(
            self._compat_state_meta_key(state_key),
            value,
            value_type="json",
            source="compat_set_app_state",
        )

    async def get_app_state(self, state_key: str) -> dict[str, Any] | None:
        if state_key.startswith("settings."):
            payload = await self._get_setting("settings", state_key.split(".", 1)[1])
            if payload is not None:
                return payload if isinstance(payload, dict) else {"value": payload}
        if state_key.startswith("runtime."):
            payload = await self.get_runtime_state(state_key.split(".", 1)[1])
            if payload is not None:
                return payload
        async with self._connection() as db:
            cursor = await db.execute(
                "SELECT meta_value FROM app_metadata WHERE meta_key = ? LIMIT 1",
                (self._compat_state_meta_key(state_key),),
            )
            row = await cursor.fetchone()
        return json.loads(row[0]) if row else None

    async def sync_tweak_catalog(self, entries: list[dict[str, Any]]) -> None:
        if not entries:
            return
        now = time.time()
        async with self._connection() as db:
            for item in entries:
                await db.execute(
                    """
                    INSERT INTO tweak_catalog (
                        tweak_id, category, name, description, validity_tag, risk_level,
                        requires_admin, expert_only, supported_flag, hardware_scope, current_status_json, updated_at
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    ON CONFLICT(tweak_id)
                    DO UPDATE SET category = excluded.category,
                                  name = excluded.name,
                                  description = excluded.description,
                                  validity_tag = excluded.validity_tag,
                                  risk_level = excluded.risk_level,
                                  requires_admin = excluded.requires_admin,
                                  expert_only = excluded.expert_only,
                                  supported_flag = excluded.supported_flag,
                                  hardware_scope = excluded.hardware_scope,
                                  current_status_json = excluded.current_status_json,
                                  updated_at = excluded.updated_at
                    """,
                    (
                        item.get("id"),
                        item.get("category", "unknown"),
                        item.get("name", "Unnamed tweak"),
                        item.get("rationale", ""),
                        item.get("validity", "VALIDATED"),
                        "moderate" if item.get("requires_admin") else "safe",
                        int(bool(item.get("requires_admin"))),
                        int(bool(item.get("expert_only", False))),
                        int(bool(item.get("supported", True))),
                        json.dumps(item.get("hardware_requirements") or []),
                        json.dumps({"compatibility_note": item.get("compatibility_note"), "impact": item.get("impact")}),
                        now,
                    ),
                )
            await db.commit()

    async def append_audit_entry(self, entry: AuditEntry) -> None:
        async with self._connection() as db:
            await db.execute(
                """
                INSERT OR REPLACE INTO audit_entries (
                    id, timestamp, module, action, target, before_value, after_value,
                    rationale, validity_tag, triggered_by, reverted, revert_timestamp,
                    session_id, related_application_id, status
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    entry.id,
                    entry.timestamp,
                    entry.module,
                    entry.action,
                    entry.target,
                    json.dumps(entry.before_value) if entry.before_value is not None else None,
                    json.dumps(entry.after_value) if entry.after_value is not None else None,
                    entry.rationale,
                    entry.validity_tag,
                    entry.triggered_by,
                    int(entry.reverted),
                    entry.revert_timestamp,
                    entry.session_id,
                    None,
                    entry.status,
                ),
            )
            await db.commit()

    async def list_audit_entries(self, limit: int = 100) -> list[dict[str, Any]]:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT id, timestamp, module, action, target, before_value, after_value,
                       rationale, validity_tag, triggered_by, reverted, revert_timestamp,
                       session_id, related_application_id, status
                FROM audit_entries
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
                "module": row[2],
                "action": row[3],
                "target": row[4],
                "before_value": json.loads(row[5]) if row[5] else None,
                "after_value": json.loads(row[6]) if row[6] else None,
                "rationale": row[7],
                "validity_tag": row[8],
                "triggered_by": row[9],
                "reverted": bool(row[10]),
                "revert_timestamp": row[11],
                "session_id": row[12],
                "related_application_id": row[13],
                "status": row[14],
            }
            for row in rows
        ]

    async def get_audit_entry(self, audit_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT id, timestamp, module, action, target, before_value, after_value,
                       rationale, validity_tag, triggered_by, reverted, revert_timestamp,
                       session_id, related_application_id, status
                FROM audit_entries
                WHERE id = ?
                LIMIT 1
                """,
                (audit_id,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "id": row[0],
            "timestamp": row[1],
            "module": row[2],
            "action": row[3],
            "target": row[4],
            "before_value": json.loads(row[5]) if row[5] else None,
            "after_value": json.loads(row[6]) if row[6] else None,
            "rationale": row[7],
            "validity_tag": row[8],
            "triggered_by": row[9],
            "reverted": bool(row[10]),
            "revert_timestamp": row[11],
            "session_id": row[12],
            "related_application_id": row[13],
            "status": row[14],
        }

    async def insert_revert_snapshot(self, snapshot: RevertSnapshot) -> None:
        payload = {
            "target_type": snapshot.target_type,
            "target_id": snapshot.target_id,
            "before_value": snapshot.before_value,
            "metadata": snapshot.metadata,
            "restore_status": snapshot.restore_status,
        }
        async with self._connection() as db:
            await db.execute(
                """
                INSERT OR REPLACE INTO rollback_snapshots (
                    snapshot_id, created_at, scope, state_summary, restorable_flag, restored_at, related_session_id, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    snapshot.snapshot_id,
                    snapshot.created_at,
                    f"{snapshot.target_type}:{snapshot.target_id}",
                    json.dumps(snapshot.before_value),
                    int(not snapshot.restored),
                    snapshot.restored_at,
                    snapshot.session_id,
                    json.dumps(payload),
                ),
            )
            await db.commit()

    async def get_revert_snapshot(self, snapshot_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT snapshot_id, created_at, scope, state_summary, restorable_flag, restored_at, related_session_id, payload_json
                FROM rollback_snapshots
                WHERE snapshot_id = ?
                LIMIT 1
                """,
                (snapshot_id,),
            )
            row = await cursor.fetchone()
            if row:
                payload = json.loads(row[7]) if row[7] else {}
                return {
                    "snapshot_id": row[0],
                    "target_type": payload.get("target_type"),
                    "target_id": payload.get("target_id"),
                    "before_value": payload.get("before_value"),
                    "created_at": row[1],
                    "session_id": row[6],
                    "restored": not bool(row[4]),
                    "restored_at": row[5],
                    "restore_status": payload.get("restore_status"),
                    "metadata": payload.get("metadata") or {},
                }
        return None

    async def mark_revert_snapshot_restored(self, snapshot_id: str, *, status: str) -> None:
        now = time.time()
        async with self._connection() as db:
            cursor = await db.execute(
                "SELECT payload_json FROM rollback_snapshots WHERE snapshot_id = ? LIMIT 1",
                (snapshot_id,),
            )
            existing_row = await cursor.fetchone()
            payload = json.loads(existing_row[0]) if existing_row and existing_row[0] else {}
            payload["restore_status"] = status
            await db.execute(
                """
                UPDATE rollback_snapshots
                SET restorable_flag = 0, restored_at = ?, payload_json = ?
                WHERE snapshot_id = ?
                """,
                (now, json.dumps(payload), snapshot_id),
            )
            await db.commit()

    async def save_hardware_profile(self, profile: HardwareProfile) -> None:
        async with self._connection() as db:
            await db.execute(
                "INSERT INTO hardware_profiles (machine_id, machine_name, captured_at, payload_json) VALUES (?, ?, ?, ?)",
                (profile.machine_id, profile.machine_name, profile.captured_at, json.dumps(profile.to_dict())),
            )
            await db.commit()

    async def latest_hardware_profile(self) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute("SELECT payload_json FROM hardware_profiles ORDER BY captured_at DESC LIMIT 1")
            row = await cursor.fetchone()
        return json.loads(row[0]) if row else None

    async def save_capability_snapshot(self, snapshot: CapabilitySnapshot) -> None:
        async with self._connection() as db:
            await db.execute(
                "INSERT INTO capabilities_cache (machine_id, captured_at, payload_json) VALUES (?, ?, ?)",
                (snapshot.machine_id, snapshot.captured_at, json.dumps(snapshot.to_dict())),
            )
            await db.commit()

    async def latest_capability_snapshot(self) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute("SELECT payload_json FROM capabilities_cache ORDER BY captured_at DESC LIMIT 1")
            row = await cursor.fetchone()
        return json.loads(row[0]) if row else None

    async def insert_session(self, session: SessionRecord) -> None:
        payload = session.to_dict()
        async with self._connection() as db:
            await db.execute(
                """
                INSERT OR REPLACE INTO sessions (
                    session_id, game_name, workload_name, executable_path, started_at, ended_at,
                    session_type, status, summary, machine_profile_ref,
                    baseline_metrics_json, final_metrics_json, tweaks_applied_count,
                    adaptive_actions_count, stability_score, clean_exit
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    session.id,
                    session.game_name,
                    session.game_name,
                    session.executable_path,
                    session.started_at,
                    session.ended_at,
                    "game",
                    "active",
                    None,
                    None,
                    json.dumps(session.baseline_metrics_snapshot),
                    json.dumps(session.final_metrics_snapshot) if session.final_metrics_snapshot else None,
                    session.tweaks_applied_count,
                    session.adaptive_actions_count,
                    session.stability_score,
                    int(session.clean_exit),
                ),
            )
            await db.commit()

    async def complete_session(
        self,
        session_id: str,
        *,
        final_metrics_snapshot: dict[str, Any],
        stability_score: float | None,
        clean_exit: bool,
    ) -> None:
        async with self._connection() as db:
            await db.execute(
                """
                UPDATE sessions
                SET ended_at = ?, final_metrics_json = ?, stability_score = ?, clean_exit = ?, status = ?
                WHERE session_id = ?
                """,
                (time.time(), json.dumps(final_metrics_snapshot), stability_score, int(clean_exit), "completed", session_id),
            )
            await db.commit()

    async def list_sessions(self, limit: int = 20) -> list[dict[str, Any]]:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT session_id, game_name, executable_path, started_at, ended_at,
                       baseline_metrics_json, final_metrics_json, tweaks_applied_count,
                       adaptive_actions_count, stability_score, clean_exit
                FROM sessions
                ORDER BY started_at DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [
            {
                "id": row[0],
                "game_name": row[1],
                "executable_path": row[2],
                "started_at": row[3],
                "ended_at": row[4],
                "baseline_metrics_snapshot": json.loads(row[5]) if row[5] else {},
                "final_metrics_snapshot": json.loads(row[6]) if row[6] else {},
                "tweaks_applied_count": row[7],
                "adaptive_actions_count": row[8],
                "stability_score": row[9],
                "clean_exit": bool(row[10]),
            }
            for row in rows
        ]

    async def insert_health_history(
        self,
        *,
        score: float,
        cpu_load: float,
        ram_percent: float,
        gpu_temp: float | None,
        timestamp: float | None = None,
    ) -> None:
        record_ts = timestamp if timestamp is not None else time.time()
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO health_history (score, cpu_load, ram_percent, gpu_temp, timestamp)
                VALUES (?, ?, ?, ?, ?)
                """,
                (float(score), float(cpu_load), float(ram_percent), None if gpu_temp is None else float(gpu_temp), float(record_ts)),
            )
            await db.commit()

    async def list_health_history(self, *, days: int = 7, limit: int = 3000) -> list[dict[str, Any]]:
        window_seconds = max(1, int(days)) * 24 * 3600
        lower_bound = time.time() - window_seconds
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT id, score, cpu_load, ram_percent, gpu_temp, timestamp
                FROM health_history
                WHERE timestamp >= ?
                ORDER BY timestamp ASC
                LIMIT ?
                """,
                (float(lower_bound), int(limit)),
            )
            rows = await cursor.fetchall()
        return [
            {
                "id": row[0],
                "score": row[1],
                "cpu_load": row[2],
                "ram_percent": row[3],
                "gpu_temp": row[4],
                "timestamp": row[5],
            }
            for row in rows
        ]

    async def record_session_action(self, session_id: str, action_type: str, detail: dict[str, Any]) -> None:
        async with self._connection() as db:
            await db.execute(
                "INSERT INTO session_actions (session_id, timestamp, action_type, detail_json) VALUES (?, ?, ?, ?)",
                (session_id, time.time(), action_type, json.dumps(detail)),
            )
            await db.commit()

    async def create_tweak_application(
        self,
        *,
        tweak_id: str,
        session_id: str | None,
        triggered_by: str,
        temporary_flag: bool,
        apply_result: str,
        revert_snapshot_id: str | None,
        target: str | None,
        metadata: dict[str, Any] | None = None,
        before_value: dict[str, Any] | None = None,
        after_value: dict[str, Any] | None = None,
        error_text: str | None = None,
    ) -> str:
        application_id = str(uuid.uuid4())
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO tweak_applications (
                    application_id, tweak_id, session_id, applied_at, reverted_at, before_value, after_value,
                    apply_result, revert_result, triggered_by, temporary_flag, error_text, revert_snapshot_id, target, metadata_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    application_id,
                    tweak_id,
                    session_id,
                    time.time(),
                    None,
                    json.dumps(before_value) if before_value is not None else None,
                    json.dumps(after_value) if after_value is not None else None,
                    apply_result,
                    None,
                    triggered_by,
                    int(temporary_flag),
                    error_text,
                    revert_snapshot_id,
                    target,
                    json.dumps(metadata or {}),
                ),
            )
            await db.commit()
        return application_id

    async def list_active_tweak_applications(self) -> list[dict[str, Any]]:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT application_id, tweak_id, session_id, applied_at, revert_snapshot_id, target, metadata_json
                FROM tweak_applications
                WHERE temporary_flag = 1 AND reverted_at IS NULL
                ORDER BY applied_at ASC
                """
            )
            rows = await cursor.fetchall()
        return [
            {
                "application_id": row[0],
                "tweak_id": row[1],
                "session_id": row[2],
                "applied_at": row[3],
                "snapshot_id": row[4],
                "target": row[5],
                "metadata": json.loads(row[6]) if row[6] else {},
            }
            for row in rows
        ]

    async def mark_tweak_application_reverted(self, *, revert_snapshot_id: str, revert_result: str = "success") -> None:
        async with self._connection() as db:
            await db.execute(
                """
                UPDATE tweak_applications
                SET reverted_at = ?, revert_result = ?
                WHERE revert_snapshot_id = ? AND reverted_at IS NULL
                """,
                (time.time(), revert_result, revert_snapshot_id),
            )
            await db.commit()

    async def clear_active_tweak_applications(self) -> None:
        async with self._connection() as db:
            await db.execute(
                """
                UPDATE tweak_applications
                SET reverted_at = COALESCE(reverted_at, ?), revert_result = COALESCE(revert_result, 'cleared')
                WHERE temporary_flag = 1 AND reverted_at IS NULL
                """,
                (time.time(),),
            )
            await db.commit()

    async def insert_benchmark_result(self, result: BenchmarkResult) -> None:
        payload = normalize_benchmark_result_payload(result.to_dict())
        unsupported = payload.get("unsupported_reasons") or []
        async with self._connection() as db:
            await db.execute(
                """
                INSERT OR REPLACE INTO benchmark_runs (
                    benchmark_id, session_id, created_at, workload_name, machine_hash,
                    baseline_label, optimized_label, status, notes
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    result.benchmark_id,
                    result.session_id,
                    result.created_at,
                    result.workload_name,
                    payload.get("machine_hash"),
                    "baseline",
                    "optimized",
                    "completed",
                    result.notes,
                ),
            )
            await db.execute(
                """
                INSERT OR REPLACE INTO benchmark_results (
                    result_id, benchmark_id, average_fps, fps_1_low, frametime_variance,
                    cpu_delta, gpu_delta, ping_delta, jitter_delta, verdict, confidence_score,
                    unsupported_metrics_json, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    f"{result.benchmark_id}:result",
                    result.benchmark_id,
                    result.avg_fps_delta,
                    result.one_percent_low_delta,
                    result.frame_time_variance_delta,
                    result.cpu_delta,
                    result.gpu_delta,
                    result.ping_delta,
                    result.jitter_delta,
                    result.verdict,
                    payload.get("confidence_score"),
                    json.dumps(unsupported),
                    json.dumps(payload),
                ),
            )
            await db.commit()

    async def list_benchmark_results(self, limit: int = 25) -> list[dict[str, Any]]:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT r.payload_json
                FROM benchmark_results r
                JOIN benchmark_runs b ON b.benchmark_id = r.benchmark_id
                ORDER BY b.created_at DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [normalize_benchmark_result_payload(json.loads(row[0])) for row in rows if row[0]]

    async def get_benchmark_result(self, benchmark_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                "SELECT payload_json FROM benchmark_results WHERE benchmark_id = ? LIMIT 1",
                (benchmark_id,),
            )
            row = await cursor.fetchone()
        return normalize_benchmark_result_payload(json.loads(row[0])) if row and row[0] else None

    async def clear_benchmark_history(self) -> dict[str, int]:
        async with self._connection() as db:
            results_cursor = await db.execute("SELECT COUNT(1) FROM benchmark_results")
            runs_cursor = await db.execute("SELECT COUNT(1) FROM benchmark_runs")
            results_before = int((await results_cursor.fetchone())[0] or 0)
            runs_before = int((await runs_cursor.fetchone())[0] or 0)
            await db.execute("DELETE FROM benchmark_results")
            await db.execute("DELETE FROM benchmark_runs")
            await db.commit()
        return {
            "deleted_results": results_before,
            "deleted_runs": runs_before,
        }

    async def insert_adaptive_action(self, record: dict[str, Any]) -> None:
        success = bool(record.get("success"))
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO adaptive_actions (
                    timestamp, rule_id, rule_name, title, tweak_id, trigger_reason, action_name, cooldown_key,
                    rationale, session_id, success, outcome, reverted, notes, result_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    record["timestamp"],
                    record.get("rule_id"),
                    record.get("rule_name") or record.get("rule_id"),
                    record.get("title"),
                    record.get("tweak_id"),
                    record.get("trigger_reason") or "adaptive_engine",
                    record.get("action_name") or record.get("tweak_id"),
                    record.get("cooldown_key") or record.get("rule_id"),
                    record.get("rationale"),
                    record.get("session_id"),
                    int(success),
                    record.get("outcome") or ("success" if success else "failed"),
                    int(bool(record.get("reverted", False))),
                    record.get("notes") or record.get("rationale"),
                    json.dumps(record.get("result", {})),
                ),
            )
            await db.commit()

    async def list_adaptive_actions(self, limit: int = 10) -> list[dict[str, Any]]:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT timestamp, rule_id, rule_name, title, tweak_id, trigger_reason, action_name, cooldown_key,
                       rationale, session_id, success, outcome, reverted, notes, result_json
                FROM adaptive_actions
                ORDER BY timestamp DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [
            {
                "timestamp": row[0],
                "rule_id": row[1],
                "rule_name": row[2],
                "title": row[3],
                "tweak_id": row[4],
                "trigger_reason": row[5],
                "action_name": row[6],
                "cooldown_key": row[7],
                "rationale": row[8],
                "session_id": row[9],
                "success": bool(row[10]),
                "outcome": row[11],
                "reverted": bool(row[12]),
                "notes": row[13],
                "result": json.loads(row[14]) if row[14] else {},
            }
            for row in rows
        ]

    async def insert_network_diagnostics(self, payload: dict[str, Any]) -> None:
        snapshot_id = f"net-{uuid.uuid4()}"
        summary = payload.get("summary") or {}
        nic_capabilities = payload.get("nic_capabilities") or []
        nic_name = payload.get("nic_name") or (nic_capabilities[0].get("name") if nic_capabilities else None)
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO network_snapshots (
                    snapshot_id, session_id, recorded_at, latency, jitter, packet_loss, throughput_up, throughput_down,
                    protocol_profile, nic_name, qos_supported, capability_summary, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    snapshot_id,
                    payload.get("session_id"),
                    payload.get("timestamp", time.time()),
                    summary.get("latency_ms"),
                    summary.get("jitter_ms"),
                    summary.get("packet_loss_percent"),
                    payload.get("throughput_up"),
                    payload.get("throughput_down"),
                    json.dumps(payload.get("protocol_profile")) if payload.get("protocol_profile") is not None else None,
                    nic_name,
                    int(bool((payload.get("qos") or {}).get("supported"))),
                    json.dumps({"qos": payload.get("qos"), "targets": payload.get("targets")}),
                    json.dumps(payload),
                ),
            )
            await db.commit()

    async def latest_network_diagnostics(self) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                "SELECT payload_json FROM network_snapshots ORDER BY recorded_at DESC LIMIT 1"
            )
            row = await cursor.fetchone()
        return json.loads(row[0]) if row and row[0] else None

    async def insert_gpu_snapshot(self, payload: dict[str, Any], *, session_id: str | None = None) -> None:
        snapshot_id = f"gpu-{uuid.uuid4()}"
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO gpu_snapshots (
                    snapshot_id, session_id, recorded_at, vendor, model, utilization, vram_used, temperature,
                    clocks_json, telemetry_supported, advisory_summary, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    snapshot_id,
                    session_id,
                    payload.get("timestamp", time.time()),
                    payload.get("vendor"),
                    payload.get("model"),
                    payload.get("utilization_percent"),
                    payload.get("memory_used_mb"),
                    payload.get("temperature_c"),
                    json.dumps({"clock_mhz": payload.get("clock_mhz"), "power_watts": payload.get("power_watts")}),
                    int(bool(payload.get("telemetry_supported"))),
                    payload.get("reason"),
                    json.dumps(payload),
                ),
            )
            await db.commit()

    async def latest_gpu_snapshot(self) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT session_id, recorded_at, vendor, model, utilization, vram_used, temperature, clocks_json, telemetry_supported, advisory_summary, payload_json
                FROM gpu_snapshots
                ORDER BY recorded_at DESC
                LIMIT 1
                """
            )
            row = await cursor.fetchone()
        if not row:
            return None
        payload = json.loads(row[10]) if row[10] else {}
        payload.setdefault("session_id", row[0])
        payload.setdefault("timestamp", row[1])
        payload.setdefault("vendor", row[2])
        payload.setdefault("model", row[3])
        payload.setdefault("utilization_percent", row[4])
        payload.setdefault("memory_used_mb", row[5])
        payload.setdefault("temperature_c", row[6])
        payload.setdefault("clocks", json.loads(row[7]) if row[7] else {})
        payload.setdefault("telemetry_supported", bool(row[8]))
        payload.setdefault("reason", row[9])
        return payload

    async def upsert_game_profile(
        self,
        game_id: str,
        game_name: str,
        executable_path: str | None,
        payload: dict[str, Any],
    ) -> None:
        now = time.time()
        profile_id = f"profile-{game_id}"
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO game_profiles (
                    profile_id, game_id, game_name, display_name, executable_path, payload_json,
                    recommendations_json, recommendation_basis_json, stability_score, fingerprint_json,
                    last_benchmark_id, active_flag, created_at, updated_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(game_id)
                DO UPDATE SET game_name = excluded.game_name,
                              display_name = excluded.display_name,
                              executable_path = excluded.executable_path,
                              payload_json = excluded.payload_json,
                              recommendations_json = excluded.recommendations_json,
                              recommendation_basis_json = excluded.recommendation_basis_json,
                              stability_score = excluded.stability_score,
                              fingerprint_json = excluded.fingerprint_json,
                              last_benchmark_id = excluded.last_benchmark_id,
                              active_flag = excluded.active_flag,
                              updated_at = excluded.updated_at
                """,
                (
                    profile_id,
                    game_id,
                    game_name,
                    payload.get("display_name") or payload.get("game_name") or game_name,
                    executable_path,
                    json.dumps(payload),
                    json.dumps(payload.get("recommended_tweaks") or []),
                    json.dumps(payload.get("recommendation_basis") or {}),
                    payload.get("stability_score"),
                    json.dumps(payload.get("fingerprint") or {}),
                    payload.get("last_benchmark_id"),
                    int(bool(payload.get("active_flag", True))),
                    payload.get("created_at", now),
                    now,
                ),
            )
            await db.commit()

    async def get_game_profile(self, game_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT game_name, executable_path, payload_json, updated_at
                FROM game_profiles
                WHERE game_id = ?
                LIMIT 1
                """,
                (game_id,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        payload = json.loads(row[2]) if row[2] else {}
        payload.setdefault("game_id", game_id)
        payload.setdefault("game_name", row[0])
        payload.setdefault("executable_path", row[1])
        payload.setdefault("updated_at", row[3])
        return payload

    async def list_game_profiles(self) -> list[dict[str, Any]]:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT game_id, game_name, executable_path, payload_json, updated_at
                FROM game_profiles
                ORDER BY updated_at DESC
                """
            )
            rows = await cursor.fetchall()
        result: list[dict[str, Any]] = []
        for row in rows:
            payload = json.loads(row[3]) if row[3] else {}
            payload.setdefault("game_id", row[0])
            payload.setdefault("game_name", row[1])
            payload.setdefault("executable_path", row[2])
            payload.setdefault("updated_at", row[4])
            result.append(payload)
        return result

    async def insert_trust_center_state(self, payload: dict[str, Any]) -> None:
        state_id = f"trust-{uuid.uuid4()}"
        rollback_ready = bool(((payload.get("rollback_readiness") or {}).get("ready")))
        protected = payload.get("protected_process_rules") or []
        unsupported = payload.get("unsupported_capabilities") or []
        safeguards = payload.get("safeguard_summary") or []
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO trust_center_state (
                    state_id, recorded_at, rollback_ready, recovery_state, protected_process_summary,
                    unsupported_capabilities_json, dangerous_features_visible, expert_mode_enabled, safeguard_summary, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    state_id,
                    time.time(),
                    int(rollback_ready),
                    json.dumps(payload.get("crash_recovery_status") or {}),
                    json.dumps(protected),
                    json.dumps(unsupported),
                    int(not bool(payload.get("dangerous_tweaks_disabled", True))),
                    int(bool(payload.get("expert_mode_state", False))),
                    json.dumps(safeguards),
                    json.dumps(payload),
                ),
            )
            await db.commit()

    async def count_pending_revert_snapshots(self) -> int:
        async with self._connection() as db:
            cursor = await db.execute(
                "SELECT COUNT(1) FROM rollback_snapshots WHERE restorable_flag = 1"
            )
            row = await cursor.fetchone()
        return int(row[0] or 0)

    async def upsert_account_identity(self, payload: dict[str, Any]) -> None:
        now = time.time()
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO account_identities (account_id, email, display_name, avatar_url, created_at, updated_at)
                VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(account_id)
                DO UPDATE SET email = excluded.email,
                              display_name = excluded.display_name,
                              avatar_url = excluded.avatar_url,
                              created_at = COALESCE(account_identities.created_at, excluded.created_at),
                              updated_at = excluded.updated_at
                """,
                (
                    payload["account_id"],
                    payload["email"],
                    payload["display_name"],
                    payload.get("avatar_url"),
                    payload.get("created_at"),
                    now,
                ),
            )
            await db.execute(
                """
                INSERT INTO account_cache (account_id, email, display_name, avatar_url, last_synced_at)
                VALUES (?, ?, ?, ?, ?)
                ON CONFLICT(account_id)
                DO UPDATE SET email = excluded.email,
                              display_name = excluded.display_name,
                              avatar_url = excluded.avatar_url,
                              last_synced_at = excluded.last_synced_at
                """,
                (
                    payload["account_id"],
                    payload["email"],
                    payload["display_name"],
                    payload.get("avatar_url"),
                    now,
                ),
            )
            await db.commit()

    async def get_account_identity(self, account_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT account_id, email, display_name, avatar_url
                FROM account_cache
                WHERE account_id = ?
                LIMIT 1
                """,
                (account_id,),
            )
            row = await cursor.fetchone()
            if not row:
                cursor = await db.execute(
                    """
                    SELECT account_id, email, display_name, avatar_url
                    FROM account_identities
                    WHERE account_id = ?
                    LIMIT 1
                    """,
                    (account_id,),
                )
                row = await cursor.fetchone()
        if not row:
            return None
        return {
            "account_id": row[0],
            "email": row[1],
            "display_name": row[2],
            "avatar_url": row[3],
        }

    async def upsert_auth_session(self, payload: dict[str, Any]) -> None:
        now = time.time()
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO auth_sessions (account_id, expires_at, last_verified_at, signed_in, source, activation_id, created_at, updated_at)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(account_id)
                DO UPDATE SET expires_at = excluded.expires_at,
                              last_verified_at = excluded.last_verified_at,
                              signed_in = excluded.signed_in,
                              source = excluded.source,
                              activation_id = excluded.activation_id,
                              updated_at = excluded.updated_at
                """,
                (
                    payload["account_id"],
                    payload["expires_at"],
                    payload.get("last_verified_at"),
                    int(bool(payload.get("signed_in", True))),
                    payload.get("source", "local"),
                    payload.get("activation_id"),
                    payload.get("created_at", now),
                    now,
                ),
            )
            await db.commit()

    async def get_active_auth_session(self) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT account_id, expires_at, last_verified_at, signed_in, source, activation_id, created_at, updated_at
                FROM auth_sessions
                WHERE signed_in = 1
                ORDER BY updated_at DESC
                LIMIT 1
                """
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "account_id": row[0],
            "expires_at": row[1],
            "last_verified_at": row[2],
            "signed_in": bool(row[3]),
            "source": row[4],
            "activation_id": row[5],
            "created_at": row[6],
            "updated_at": row[7],
        }

    async def get_auth_session(self, account_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT account_id, expires_at, last_verified_at, signed_in, source, activation_id, created_at, updated_at
                FROM auth_sessions
                WHERE account_id = ?
                LIMIT 1
                """,
                (account_id,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "account_id": row[0],
            "expires_at": row[1],
            "last_verified_at": row[2],
            "signed_in": bool(row[3]),
            "source": row[4],
            "activation_id": row[5],
            "created_at": row[6],
            "updated_at": row[7],
        }

    async def upsert_auth_tokens(self, account_id: str, access_token_blob: bytes, refresh_token_blob: bytes) -> None:
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO auth_session_tokens (account_id, access_token_blob, refresh_token_blob, updated_at)
                VALUES (?, ?, ?, ?)
                ON CONFLICT(account_id)
                DO UPDATE SET access_token_blob = excluded.access_token_blob,
                              refresh_token_blob = excluded.refresh_token_blob,
                              updated_at = excluded.updated_at
                """,
                (account_id, access_token_blob, refresh_token_blob, time.time()),
            )
            await db.commit()

    async def get_auth_tokens(self, account_id: str) -> dict[str, bytes] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT access_token_blob, refresh_token_blob
                FROM auth_session_tokens
                WHERE account_id = ?
                LIMIT 1
                """,
                (account_id,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "access_token_blob": bytes(row[0]),
            "refresh_token_blob": bytes(row[1]),
        }

    async def clear_auth_session(self, account_id: str | None = None) -> None:
        async with self._connection() as db:
            if account_id:
                await db.execute("DELETE FROM auth_session_tokens WHERE account_id = ?", (account_id,))
                await db.execute("DELETE FROM auth_sessions WHERE account_id = ?", (account_id,))
            else:
                await db.execute("DELETE FROM auth_session_tokens")
                await db.execute("DELETE FROM auth_sessions")
            await db.commit()

    async def upsert_account_plan(self, payload: dict[str, Any]) -> None:
        now = time.time()
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO account_plans (account_id, plan_tier, status, renewal_at, trial_ends_at, updated_at)
                VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(account_id)
                DO UPDATE SET plan_tier = excluded.plan_tier,
                              status = excluded.status,
                              renewal_at = excluded.renewal_at,
                              trial_ends_at = excluded.trial_ends_at,
                              updated_at = excluded.updated_at
                """,
                (
                    payload["account_id"],
                    payload["plan_tier"],
                    payload["status"],
                    payload.get("renewal_at"),
                    payload.get("trial_ends_at"),
                    now,
                ),
            )
            await db.execute(
                """
                UPDATE account_cache
                SET current_plan = ?, status = ?, last_synced_at = ?
                WHERE account_id = ?
                """,
                (payload["plan_tier"], payload["status"], now, payload["account_id"]),
            )
            await db.commit()

    async def get_account_plan(self, account_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT account_id, plan_tier, status, renewal_at, trial_ends_at
                FROM account_plans
                WHERE account_id = ?
                LIMIT 1
                """,
                (account_id,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "account_id": row[0],
            "plan_tier": row[1],
            "status": row[2],
            "renewal_at": row[3],
            "trial_ends_at": row[4],
        }

    async def upsert_entitlement_snapshot(self, payload: dict[str, Any]) -> None:
        account_id = payload.get("account_id") or "__signed_out__"
        now = time.time()
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO entitlement_snapshots (account_id, plan_tier, payload_json, generated_at, source, updated_at)
                VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(account_id)
                DO UPDATE SET plan_tier = excluded.plan_tier,
                              payload_json = excluded.payload_json,
                              generated_at = excluded.generated_at,
                              source = excluded.source,
                              updated_at = excluded.updated_at
                """,
                (
                    account_id,
                    payload["plan_tier"],
                    json.dumps(payload),
                    payload["generated_at"],
                    payload["source"],
                    now,
                ),
            )
            snapshot_id = str(uuid.uuid4())
            await db.execute(
                """
                INSERT INTO entitlement_snapshot_history (
                    snapshot_id, account_id, generated_at, source, plan_tier, features_json, verification_status, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    snapshot_id,
                    None if account_id == "__signed_out__" else account_id,
                    payload["generated_at"],
                    payload["source"],
                    payload["plan_tier"],
                    json.dumps(payload.get("features") or {}),
                    payload.get("verification_status") or "cached",
                    json.dumps(payload),
                ),
            )
            await db.commit()

    async def get_entitlement_snapshot(self, account_id: str | None) -> dict[str, Any] | None:
        lookup_id = account_id or "__signed_out__"
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT payload_json
                FROM entitlement_snapshots
                WHERE account_id = ?
                LIMIT 1
                """,
                (lookup_id,),
            )
            row = await cursor.fetchone()
        return json.loads(row[0]) if row and row[0] else None

    async def upsert_device_activation(self, payload: dict[str, Any]) -> None:
        async with self._connection() as db:
            await db.execute(
                """
                INSERT INTO device_activations (
                    activation_id, account_id, machine_hash, device_name, app_version,
                    activated_at, last_seen_at, revoked, source, updated_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(activation_id)
                DO UPDATE SET account_id = excluded.account_id,
                              machine_hash = excluded.machine_hash,
                              device_name = excluded.device_name,
                              app_version = excluded.app_version,
                              activated_at = excluded.activated_at,
                              last_seen_at = excluded.last_seen_at,
                              revoked = excluded.revoked,
                              source = excluded.source,
                              updated_at = excluded.updated_at
                """,
                (
                    payload["activation_id"],
                    payload["account_id"],
                    payload["machine_hash"],
                    payload["device_name"],
                    payload["app_version"],
                    payload["activated_at"],
                    payload["last_seen_at"],
                    int(bool(payload.get("revoked", False))),
                    payload.get("source", "local"),
                    time.time(),
                ),
            )
            await db.commit()

    async def get_device_activation(self, account_id: str) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT activation_id, account_id, machine_hash, device_name, app_version,
                       activated_at, last_seen_at, revoked, source
                FROM device_activations
                WHERE account_id = ?
                ORDER BY updated_at DESC
                LIMIT 1
                """,
                (account_id,),
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "activation_id": row[0],
            "account_id": row[1],
            "machine_hash": row[2],
            "device_name": row[3],
            "app_version": row[4],
            "activated_at": row[5],
            "last_seen_at": row[6],
            "revoked": bool(row[7]),
            "source": row[8],
        }

    async def latest_device_activation(self) -> dict[str, Any] | None:
        async with self._connection() as db:
            cursor = await db.execute(
                """
                SELECT activation_id, account_id, machine_hash, device_name, app_version,
                       activated_at, last_seen_at, revoked, source
                FROM device_activations
                ORDER BY updated_at DESC
                LIMIT 1
                """
            )
            row = await cursor.fetchone()
        if not row:
            return None
        return {
            "activation_id": row[0],
            "account_id": row[1],
            "machine_hash": row[2],
            "device_name": row[3],
            "app_version": row[4],
            "activated_at": row[5],
            "last_seen_at": row[6],
            "revoked": bool(row[7]),
            "source": row[8],
        }

