from __future__ import annotations

import json
import logging
import sqlite3
import threading
import time
import uuid
from contextlib import closing
from pathlib import Path
from typing import Callable


LOGGER = logging.getLogger(__name__)
SCHEMA_VERSION = 7
_MIGRATION_LOCK = threading.Lock()


MigrationFn = Callable[[sqlite3.Connection, Path, Path | None, list[str]], None]


def ensure_database_schema(db_path: str | Path, *, legacy_artifact_path: str | Path | None = None) -> list[str]:
    target_path = Path(db_path)
    target_path.parent.mkdir(parents=True, exist_ok=True)
    resolved_legacy = Path(legacy_artifact_path) if legacy_artifact_path else _default_legacy_path(target_path)

    reports: list[str] = []
    with _MIGRATION_LOCK:
        with closing(sqlite3.connect(target_path)) as connection:
            connection.row_factory = sqlite3.Row
            connection.execute("PRAGMA journal_mode = WAL;")
            connection.execute("PRAGMA foreign_keys = ON;")
            _ensure_schema_migrations_table(connection)
            applied_versions = {
                int(row[0])
                for row in connection.execute("SELECT version FROM schema_migrations").fetchall()
            }
            for version, name, migration in _MIGRATIONS:
                if version in applied_versions:
                    continue
                started = time.time()
                migration(connection, target_path, resolved_legacy, reports)
                duration_ms = round((time.time() - started) * 1000.0, 2)
                connection.execute(
                    """
                    INSERT INTO schema_migrations (version, name, applied_at, duration_ms)
                    VALUES (?, ?, ?, ?)
                    """,
                    (version, name, time.time(), duration_ms),
                )
                reports.append(f"Applied migration v{version}: {name} ({duration_ms} ms)")
            _upsert_metadata(connection, "schema_version", SCHEMA_VERSION, value_type="integer", source="migration")
            _upsert_metadata(connection, "last_migrated_at", time.time(), value_type="real", source="migration")
            connection.commit()

    for line in reports:
        LOGGER.info("[DB Migration] %s", line)
    return reports


def _default_legacy_path(db_path: Path) -> Path:
    resolved = db_path.expanduser().resolve()
    # Legacy cognition artifact historically lived in the workspace-level data directory.
    if len(resolved.parents) >= 3:
        workspace_candidate = resolved.parents[2] / "data" / resolved.name
        if workspace_candidate != resolved:
            return workspace_candidate
    if len(resolved.parents) >= 2:
        project_candidate = resolved.parents[1] / "data" / resolved.name
        if project_candidate != resolved:
            return project_candidate
    return resolved.parent / f"legacy-{resolved.name}"


def _ensure_schema_migrations_table(connection: sqlite3.Connection) -> None:
    connection.execute(
        """
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            applied_at REAL NOT NULL,
            duration_ms REAL NOT NULL
        )
        """
    )


def _table_exists(connection: sqlite3.Connection, table_name: str, *, schema: str = "main") -> bool:
    row = connection.execute(
        f"SELECT 1 FROM {schema}.sqlite_master WHERE type = 'table' AND name = ? LIMIT 1",
        (table_name,),
    ).fetchone()
    return row is not None


def _column_exists(connection: sqlite3.Connection, table_name: str, column_name: str) -> bool:
    rows = connection.execute(f"PRAGMA table_info({table_name})").fetchall()
    return any(str(row[1]) == column_name for row in rows)


def _add_column_if_missing(
    connection: sqlite3.Connection,
    table_name: str,
    column_name: str,
    column_sql: str,
    reports: list[str],
) -> None:
    if _column_exists(connection, table_name, column_name):
        return
    connection.execute(f"ALTER TABLE {table_name} ADD COLUMN {column_name} {column_sql}")
    reports.append(f"Added column {table_name}.{column_name}")


def _upsert_metadata(
    connection: sqlite3.Connection,
    meta_key: str,
    value: object,
    *,
    value_type: str,
    source: str,
) -> None:
    connection.execute(
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


def _migration_1_create_base_schema(
    connection: sqlite3.Connection,
    _db_path: Path,
    _legacy_artifact_path: Path | None,
    _reports: list[str],
) -> None:
    connection.executescript(
        """
        CREATE TABLE IF NOT EXISTS app_metadata (
            meta_key TEXT PRIMARY KEY,
            meta_value TEXT NOT NULL,
            value_type TEXT NOT NULL,
            updated_at REAL NOT NULL,
            source TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS settings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category TEXT NOT NULL,
            key_name TEXT NOT NULL,
            value_text TEXT NOT NULL,
            value_type TEXT NOT NULL,
            updated_at REAL NOT NULL,
            source TEXT NOT NULL,
            UNIQUE(category, key_name)
        );

        CREATE TABLE IF NOT EXISTS runtime_housekeeping (
            runtime_key TEXT PRIMARY KEY,
            value_json TEXT NOT NULL,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS tweak_catalog (
            tweak_id TEXT PRIMARY KEY,
            category TEXT NOT NULL,
            name TEXT NOT NULL,
            description TEXT NOT NULL,
            validity_tag TEXT NOT NULL,
            risk_level TEXT NOT NULL,
            requires_admin INTEGER NOT NULL DEFAULT 0,
            expert_only INTEGER NOT NULL DEFAULT 0,
            supported_flag INTEGER NOT NULL DEFAULT 1,
            hardware_scope TEXT,
            current_status_json TEXT,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS tweak_applications (
            application_id TEXT PRIMARY KEY,
            tweak_id TEXT NOT NULL,
            session_id TEXT,
            applied_at REAL NOT NULL,
            reverted_at REAL,
            before_value TEXT,
            after_value TEXT,
            apply_result TEXT NOT NULL,
            revert_result TEXT,
            triggered_by TEXT NOT NULL,
            temporary_flag INTEGER NOT NULL DEFAULT 1,
            error_text TEXT,
            revert_snapshot_id TEXT,
            target TEXT,
            metadata_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_tweak_apps_tweak ON tweak_applications(tweak_id, applied_at DESC);
        CREATE INDEX IF NOT EXISTS idx_tweak_apps_active ON tweak_applications(temporary_flag, reverted_at);

        CREATE TABLE IF NOT EXISTS audit_entries (
            id TEXT PRIMARY KEY,
            timestamp REAL NOT NULL,
            module TEXT NOT NULL,
            action TEXT NOT NULL,
            target TEXT NOT NULL,
            before_value TEXT,
            after_value TEXT,
            rationale TEXT NOT NULL,
            validity_tag TEXT NOT NULL,
            triggered_by TEXT NOT NULL,
            reverted INTEGER NOT NULL DEFAULT 0,
            revert_timestamp REAL,
            session_id TEXT,
            related_application_id TEXT,
            status TEXT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_audit_entries_timestamp ON audit_entries(timestamp);
        CREATE INDEX IF NOT EXISTS idx_audit_entries_session ON audit_entries(session_id);

        CREATE TABLE IF NOT EXISTS rollback_snapshots (
            snapshot_id TEXT PRIMARY KEY,
            created_at REAL NOT NULL,
            scope TEXT NOT NULL,
            state_summary TEXT,
            restorable_flag INTEGER NOT NULL DEFAULT 1,
            restored_at REAL,
            related_session_id TEXT,
            payload_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_rollback_snapshots_scope ON rollback_snapshots(scope, created_at DESC);

        CREATE TABLE IF NOT EXISTS hardware_profiles (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            machine_id TEXT NOT NULL,
            machine_name TEXT NOT NULL,
            captured_at REAL NOT NULL,
            payload_json TEXT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_hardware_profiles_machine ON hardware_profiles(machine_id, captured_at DESC);

        CREATE TABLE IF NOT EXISTS capabilities_cache (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            machine_id TEXT NOT NULL,
            captured_at REAL NOT NULL,
            payload_json TEXT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_capabilities_machine ON capabilities_cache(machine_id, captured_at DESC);

        CREATE TABLE IF NOT EXISTS sessions (
            session_id TEXT PRIMARY KEY,
            game_name TEXT,
            workload_name TEXT,
            executable_path TEXT,
            started_at REAL NOT NULL,
            ended_at REAL,
            session_type TEXT NOT NULL DEFAULT 'game',
            status TEXT NOT NULL DEFAULT 'active',
            summary TEXT,
            machine_profile_ref TEXT,
            baseline_metrics_json TEXT NOT NULL,
            final_metrics_json TEXT,
            tweaks_applied_count INTEGER NOT NULL DEFAULT 0,
            adaptive_actions_count INTEGER NOT NULL DEFAULT 0,
            stability_score REAL,
            clean_exit INTEGER NOT NULL DEFAULT 1
        );
        CREATE INDEX IF NOT EXISTS idx_sessions_started_at ON sessions(started_at DESC);

        CREATE TABLE IF NOT EXISTS session_actions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT NOT NULL,
            timestamp REAL NOT NULL,
            action_type TEXT NOT NULL,
            detail_json TEXT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_session_actions_session ON session_actions(session_id, timestamp DESC);

        CREATE TABLE IF NOT EXISTS benchmark_runs (
            benchmark_id TEXT PRIMARY KEY,
            session_id TEXT,
            created_at REAL NOT NULL,
            workload_name TEXT NOT NULL,
            machine_hash TEXT,
            baseline_label TEXT,
            optimized_label TEXT,
            status TEXT NOT NULL,
            notes TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_benchmark_runs_created ON benchmark_runs(created_at DESC);

        CREATE TABLE IF NOT EXISTS benchmark_results (
            result_id TEXT PRIMARY KEY,
            benchmark_id TEXT NOT NULL,
            average_fps REAL,
            fps_1_low REAL,
            frametime_variance REAL,
            cpu_delta REAL,
            gpu_delta REAL,
            ping_delta REAL,
            jitter_delta REAL,
            verdict TEXT NOT NULL,
            confidence_score REAL,
            unsupported_metrics_json TEXT,
            payload_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_benchmark_results_benchmark ON benchmark_results(benchmark_id);
        
        CREATE TABLE IF NOT EXISTS adaptive_actions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL NOT NULL,
            rule_id TEXT,
            rule_name TEXT,
            title TEXT,
            tweak_id TEXT,
            trigger_reason TEXT,
            action_name TEXT,
            cooldown_key TEXT,
            rationale TEXT,
            session_id TEXT,
            success INTEGER NOT NULL DEFAULT 0,
            outcome TEXT,
            reverted INTEGER NOT NULL DEFAULT 0,
            notes TEXT,
            result_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_adaptive_actions_timestamp ON adaptive_actions(timestamp DESC);
        CREATE INDEX IF NOT EXISTS idx_adaptive_actions_session ON adaptive_actions(session_id, timestamp DESC);

        CREATE TABLE IF NOT EXISTS network_snapshots (
            snapshot_id TEXT PRIMARY KEY,
            session_id TEXT,
            recorded_at REAL NOT NULL,
            latency REAL,
            jitter REAL,
            packet_loss REAL,
            throughput_up REAL,
            throughput_down REAL,
            protocol_profile TEXT,
            nic_name TEXT,
            qos_supported INTEGER NOT NULL DEFAULT 0,
            capability_summary TEXT,
            payload_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_network_snapshots_recorded ON network_snapshots(recorded_at DESC);

        CREATE TABLE IF NOT EXISTS gpu_snapshots (
            snapshot_id TEXT PRIMARY KEY,
            session_id TEXT,
            recorded_at REAL NOT NULL,
            vendor TEXT,
            model TEXT,
            utilization REAL,
            vram_used REAL,
            temperature REAL,
            clocks_json TEXT,
            telemetry_supported INTEGER NOT NULL DEFAULT 0,
            advisory_summary TEXT,
            payload_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_gpu_snapshots_recorded ON gpu_snapshots(recorded_at DESC);

        CREATE TABLE IF NOT EXISTS game_profiles (
            profile_id TEXT PRIMARY KEY,
            game_id TEXT NOT NULL,
            game_name TEXT NOT NULL,
            display_name TEXT,
            executable_path TEXT,
            payload_json TEXT NOT NULL,
            recommendations_json TEXT,
            recommendation_basis_json TEXT,
            stability_score REAL,
            fingerprint_json TEXT,
            last_benchmark_id TEXT,
            active_flag INTEGER NOT NULL DEFAULT 1,
            created_at REAL NOT NULL,
            updated_at REAL NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_game_profiles_game ON game_profiles(game_id, updated_at DESC);

        CREATE TABLE IF NOT EXISTS trust_center_state (
            state_id TEXT PRIMARY KEY,
            recorded_at REAL NOT NULL,
            rollback_ready INTEGER NOT NULL DEFAULT 0,
            recovery_state TEXT,
            protected_process_summary TEXT,
            unsupported_capabilities_json TEXT,
            dangerous_features_visible INTEGER NOT NULL DEFAULT 0,
            expert_mode_enabled INTEGER NOT NULL DEFAULT 0,
            safeguard_summary TEXT,
            payload_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_trust_center_state_recorded ON trust_center_state(recorded_at DESC);

        CREATE TABLE IF NOT EXISTS account_cache (
            account_id TEXT PRIMARY KEY,
            email TEXT NOT NULL,
            display_name TEXT NOT NULL,
            avatar_url TEXT,
            current_plan TEXT,
            status TEXT,
            last_synced_at REAL
        );

        CREATE TABLE IF NOT EXISTS account_identities (
            account_id TEXT PRIMARY KEY,
            email TEXT NOT NULL,
            display_name TEXT NOT NULL,
            avatar_url TEXT,
            created_at REAL,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS auth_sessions (
            account_id TEXT PRIMARY KEY,
            expires_at REAL NOT NULL,
            last_verified_at REAL,
            signed_in INTEGER NOT NULL,
            source TEXT NOT NULL,
            activation_id TEXT,
            created_at REAL NOT NULL,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS auth_session_tokens (
            account_id TEXT PRIMARY KEY,
            access_token_blob BLOB NOT NULL,
            refresh_token_blob BLOB NOT NULL,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS account_plans (
            account_id TEXT PRIMARY KEY,
            plan_tier TEXT NOT NULL,
            status TEXT NOT NULL,
            renewal_at REAL,
            trial_ends_at REAL,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS entitlement_snapshots (
            account_id TEXT PRIMARY KEY,
            plan_tier TEXT NOT NULL,
            payload_json TEXT NOT NULL,
            generated_at REAL NOT NULL,
            source TEXT NOT NULL,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS entitlement_snapshot_history (
            snapshot_id TEXT PRIMARY KEY,
            account_id TEXT,
            generated_at REAL NOT NULL,
            source TEXT NOT NULL,
            plan_tier TEXT NOT NULL,
            features_json TEXT NOT NULL,
            verification_status TEXT,
            payload_json TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_entitlement_snapshot_history_account ON entitlement_snapshot_history(account_id, generated_at DESC);

        CREATE TABLE IF NOT EXISTS device_activations (
            activation_id TEXT PRIMARY KEY,
            account_id TEXT NOT NULL,
            machine_hash TEXT NOT NULL,
            device_name TEXT NOT NULL,
            app_version TEXT NOT NULL,
            activated_at REAL NOT NULL,
            last_seen_at REAL NOT NULL,
            revoked INTEGER NOT NULL DEFAULT 0,
            source TEXT NOT NULL,
            updated_at REAL NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_device_activations_account ON device_activations(account_id, updated_at DESC);

        CREATE TABLE IF NOT EXISTS ui_notifications (
            notification_id TEXT PRIMARY KEY,
            created_at REAL NOT NULL,
            severity TEXT NOT NULL,
            title TEXT NOT NULL,
            message TEXT NOT NULL,
            read_flag INTEGER NOT NULL DEFAULT 0,
            source TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_ui_notifications_created ON ui_notifications(created_at DESC);

        -- Cognition/telemetry tables remain in the unified DB but are now migration-managed.
        CREATE TABLE IF NOT EXISTS metrics (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL NOT NULL,
            bucket_start REAL,
            cpu_total REAL NOT NULL,
            cpu_per_core TEXT NOT NULL,
            ram_used REAL NOT NULL,
            ram_total REAL NOT NULL,
            ram_percent REAL NOT NULL,
            swap_used REAL NOT NULL,
            disk_read REAL NOT NULL,
            disk_write REAL NOT NULL,
            disk_percent REAL NOT NULL,
            net_sent REAL NOT NULL,
            net_recv REAL NOT NULL,
            temperature REAL,
            health_score REAL NOT NULL,
            session_mode TEXT NOT NULL DEFAULT 'normal',
            sample_period INTEGER NOT NULL DEFAULT 3
        );
        CREATE INDEX IF NOT EXISTS idx_metrics_timestamp ON metrics(timestamp DESC);
        CREATE INDEX IF NOT EXISTS idx_metrics_bucket ON metrics(bucket_start, sample_period);

        CREATE TABLE IF NOT EXISTS anomalies (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL NOT NULL,
            metric TEXT NOT NULL,
            current_value REAL NOT NULL,
            expected_mean REAL NOT NULL,
            expected_std REAL NOT NULL,
            deviation_score REAL NOT NULL,
            severity TEXT NOT NULL,
            resolved INTEGER DEFAULT 0,
            resolved_at REAL,
            action_taken TEXT
        );

        CREATE TABLE IF NOT EXISTS actions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL NOT NULL,
            action_type TEXT NOT NULL,
            action_detail TEXT NOT NULL,
            trigger_reason TEXT NOT NULL,
            ai_reasoning TEXT NOT NULL,
            health_before REAL NOT NULL,
            health_after REAL,
            score_delta REAL,
            success INTEGER,
            error_message TEXT
        );

        CREATE TABLE IF NOT EXISTS baselines (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            metric TEXT NOT NULL,
            hour_of_day INTEGER NOT NULL,
            session_mode TEXT NOT NULL,
            mean REAL NOT NULL,
            std_dev REAL NOT NULL,
            sample_count INTEGER NOT NULL,
            last_updated REAL NOT NULL,
            UNIQUE(metric, hour_of_day, session_mode)
        );

        CREATE TABLE IF NOT EXISTS learning (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            action_type TEXT NOT NULL,
            context_hash TEXT NOT NULL,
            avg_score_delta REAL NOT NULL,
            success_count INTEGER DEFAULT 0,
            failure_count INTEGER DEFAULT 0,
            last_used REAL NOT NULL,
            confidence REAL NOT NULL DEFAULT 0.5,
            UNIQUE(action_type, context_hash)
        );

        CREATE TABLE IF NOT EXISTS usage_counters (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            counter_date TEXT NOT NULL,
            counter_key TEXT NOT NULL,
            value INTEGER NOT NULL DEFAULT 0,
            UNIQUE(counter_date, counter_key)
        );

        CREATE TABLE IF NOT EXISTS optimizations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            external_id TEXT,
            timestamp REAL NOT NULL,
            pillar TEXT NOT NULL,
            title TEXT NOT NULL,
            finding TEXT NOT NULL,
            impact TEXT NOT NULL,
            action TEXT NOT NULL,
            params TEXT NOT NULL,
            risk TEXT NOT NULL,
            auto_executed INTEGER NOT NULL,
            user_approved INTEGER,
            status TEXT NOT NULL,
            estimated_gain TEXT,
            actual_gain TEXT,
            efficiency_before REAL,
            efficiency_after REAL
        );
        CREATE UNIQUE INDEX IF NOT EXISTS idx_optimizations_external_id ON optimizations(external_id);

        CREATE TABLE IF NOT EXISTS process_intelligence (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL NOT NULL,
            pid INTEGER NOT NULL,
            name TEXT NOT NULL,
            cpu_percent REAL NOT NULL,
            ram_bytes INTEGER NOT NULL,
            waste_score REAL NOT NULL,
            anomaly_level TEXT,
            is_leak_candidate INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS scheduled_tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            created_at REAL NOT NULL,
            task_type TEXT NOT NULL,
            task_detail TEXT NOT NULL,
            scheduled_for TEXT NOT NULL,
            status TEXT DEFAULT 'queued',
            completed_at REAL
        );

        CREATE TABLE IF NOT EXISTS efficiency_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL NOT NULL,
            overall_score REAL NOT NULL,
            cpu_score REAL NOT NULL,
            ram_score REAL NOT NULL,
            disk_score REAL NOT NULL,
            network_score REAL NOT NULL,
            thermal_score REAL NOT NULL
        );
        """
    )

    _upsert_metadata(connection, "created_at", time.time(), value_type="real", source="migration")
    _upsert_metadata(connection, "clean_exit", True, value_type="boolean", source="migration")


def _migration_2_expand_existing_schema(
    connection: sqlite3.Connection,
    _db_path: Path,
    _legacy_artifact_path: Path | None,
    reports: list[str],
) -> None:
    _add_column_if_missing(connection, "audit_entries", "related_application_id", "TEXT", reports)
    _add_column_if_missing(connection, "sessions", "workload_name", "TEXT", reports)
    _add_column_if_missing(connection, "sessions", "session_type", "TEXT NOT NULL DEFAULT 'game'", reports)
    _add_column_if_missing(connection, "sessions", "status", "TEXT NOT NULL DEFAULT 'active'", reports)
    _add_column_if_missing(connection, "sessions", "summary", "TEXT", reports)
    _add_column_if_missing(connection, "sessions", "machine_profile_ref", "TEXT", reports)
    _add_column_if_missing(connection, "adaptive_actions", "rule_name", "TEXT", reports)
    _add_column_if_missing(connection, "adaptive_actions", "trigger_reason", "TEXT", reports)
    _add_column_if_missing(connection, "adaptive_actions", "action_name", "TEXT", reports)
    _add_column_if_missing(connection, "adaptive_actions", "cooldown_key", "TEXT", reports)
    _add_column_if_missing(connection, "adaptive_actions", "outcome", "TEXT", reports)
    _add_column_if_missing(connection, "adaptive_actions", "reverted", "INTEGER NOT NULL DEFAULT 0", reports)
    _add_column_if_missing(connection, "adaptive_actions", "notes", "TEXT", reports)
    _add_column_if_missing(connection, "game_profiles", "profile_id", "TEXT", reports)
    _add_column_if_missing(connection, "game_profiles", "display_name", "TEXT", reports)
    _add_column_if_missing(connection, "game_profiles", "recommendations_json", "TEXT", reports)
    _add_column_if_missing(connection, "game_profiles", "recommendation_basis_json", "TEXT", reports)
    _add_column_if_missing(connection, "game_profiles", "stability_score", "REAL", reports)
    _add_column_if_missing(connection, "game_profiles", "fingerprint_json", "TEXT", reports)
    _add_column_if_missing(connection, "game_profiles", "last_benchmark_id", "TEXT", reports)
    _add_column_if_missing(connection, "game_profiles", "active_flag", "INTEGER NOT NULL DEFAULT 1", reports)
    _add_column_if_missing(connection, "game_profiles", "created_at", "REAL", reports)


def _migration_3_migrate_settings_and_runtime(
    connection: sqlite3.Connection,
    _db_path: Path,
    _legacy_artifact_path: Path | None,
    reports: list[str],
) -> None:
    if not _table_exists(connection, "app_state"):
        return
    rows = connection.execute("SELECT state_key, value_json, updated_at FROM app_state").fetchall()
    migrated_settings = 0
    migrated_runtime = 0
    for row in rows:
        state_key = str(row["state_key"])
        payload_json = str(row["value_json"])
        updated_at = float(row["updated_at"] or time.time())
        if state_key.startswith("settings."):
            key_name = state_key.split(".", 1)[1]
            connection.execute(
                """
                INSERT INTO settings (category, key_name, value_text, value_type, updated_at, source)
                VALUES ('settings', ?, ?, 'json', ?, 'legacy_app_state')
                ON CONFLICT(category, key_name)
                DO UPDATE SET value_text = excluded.value_text, updated_at = excluded.updated_at, source = excluded.source
                """,
                (key_name, payload_json, updated_at),
            )
            migrated_settings += 1
            continue
        if state_key.startswith("runtime."):
            runtime_key = state_key.split(".", 1)[1]
            connection.execute(
                """
                INSERT INTO runtime_housekeeping (runtime_key, value_json, updated_at)
                VALUES (?, ?, ?)
                ON CONFLICT(runtime_key)
                DO UPDATE SET value_json = excluded.value_json, updated_at = excluded.updated_at
                """,
                (runtime_key, payload_json, updated_at),
            )
            migrated_runtime += 1
    reports.append(
        f"Migrated app_state runtime/settings keys (settings={migrated_settings}, runtime={migrated_runtime})"
    )


def _migration_4_migrate_domain_tables(
    connection: sqlite3.Connection,
    _db_path: Path,
    _legacy_artifact_path: Path | None,
    reports: list[str],
) -> None:
    if _table_exists(connection, "revert_snapshots"):
        legacy_rows = connection.execute(
            """
            SELECT snapshot_id, target_type, target_id, before_value, created_at, session_id, restored, restored_at, restore_status, metadata_json
            FROM revert_snapshots
            """
        ).fetchall()
        copied = 0
        for row in legacy_rows:
            scope = f"{row['target_type']}:{row['target_id']}"
            payload = {
                "target_type": row["target_type"],
                "target_id": row["target_id"],
                "before_value": json.loads(row["before_value"]) if row["before_value"] else None,
                "restore_status": row["restore_status"],
                "metadata": json.loads(row["metadata_json"]) if row["metadata_json"] else {},
            }
            connection.execute(
                """
                INSERT OR IGNORE INTO rollback_snapshots (
                    snapshot_id, created_at, scope, state_summary, restorable_flag, restored_at, related_session_id, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    row["snapshot_id"],
                    row["created_at"],
                    scope,
                    row["before_value"],
                    0 if row["restored"] else 1,
                    row["restored_at"],
                    row["session_id"],
                    json.dumps(payload),
                ),
            )
            copied += 1
        reports.append(f"Migrated rollback snapshots from legacy table ({copied} rows)")

    if _table_exists(connection, "benchmarks"):
        benchmark_rows = connection.execute(
            """
            SELECT benchmark_id, workload_name, created_at, duration_seconds, session_id, verdict, payload_json
            FROM benchmarks
            """
        ).fetchall()
        migrated = 0
        for row in benchmark_rows:
            payload = json.loads(row["payload_json"]) if row["payload_json"] else {}
            benchmark_id = str(row["benchmark_id"])
            connection.execute(
                """
                INSERT OR IGNORE INTO benchmark_runs (
                    benchmark_id, session_id, created_at, workload_name, machine_hash,
                    baseline_label, optimized_label, status, notes
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    benchmark_id,
                    payload.get("session_id") or row["session_id"],
                    payload.get("created_at") or row["created_at"],
                    payload.get("workload_name") or row["workload_name"],
                    payload.get("machine_hash"),
                    payload.get("baseline_label") or "baseline",
                    payload.get("optimized_label") or "optimized",
                    "completed",
                    payload.get("notes") or "",
                ),
            )
            connection.execute(
                """
                INSERT OR IGNORE INTO benchmark_results (
                    result_id, benchmark_id, average_fps, fps_1_low, frametime_variance,
                    cpu_delta, gpu_delta, ping_delta, jitter_delta, verdict, confidence_score,
                    unsupported_metrics_json, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    f"{benchmark_id}:result",
                    benchmark_id,
                    payload.get("avg_fps_delta"),
                    payload.get("one_percent_low_delta"),
                    payload.get("frame_time_variance_delta"),
                    payload.get("cpu_delta"),
                    payload.get("gpu_delta"),
                    payload.get("ping_delta"),
                    payload.get("jitter_delta"),
                    payload.get("verdict") or row["verdict"],
                    payload.get("confidence_score"),
                    json.dumps(payload.get("unsupported_reasons") or []),
                    row["payload_json"],
                ),
            )
            migrated += 1
        reports.append(f"Migrated benchmark payloads ({migrated} rows)")

    if _table_exists(connection, "network_diagnostics"):
        network_rows = connection.execute(
            "SELECT id, timestamp, payload_json FROM network_diagnostics ORDER BY id"
        ).fetchall()
        migrated = 0
        for row in network_rows:
            payload = json.loads(row["payload_json"]) if row["payload_json"] else {}
            summary = payload.get("summary") or {}
            public_target = (payload.get("targets") or {}).get("public") or {}
            nic_rows = payload.get("nic_capabilities") or []
            primary_nic = nic_rows[0].get("name") if nic_rows else None
            qos_supported = bool((payload.get("qos") or {}).get("supported"))
            connection.execute(
                """
                INSERT OR IGNORE INTO network_snapshots (
                    snapshot_id, session_id, recorded_at, latency, jitter, packet_loss,
                    throughput_up, throughput_down, protocol_profile, nic_name, qos_supported, capability_summary, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    f"legacy-network-{row['id']}",
                    payload.get("session_id"),
                    payload.get("timestamp") or row["timestamp"],
                    summary.get("latency_ms"),
                    summary.get("jitter_ms"),
                    summary.get("packet_loss_percent") or public_target.get("packet_loss_percent"),
                    payload.get("throughput_up"),
                    payload.get("throughput_down"),
                    json.dumps(payload.get("protocol_profile")) if payload.get("protocol_profile") is not None else None,
                    payload.get("nic_name") or primary_nic,
                    int(qos_supported),
                    json.dumps({"qos": payload.get("qos"), "targets": payload.get("targets")}),
                    row["payload_json"],
                ),
            )
            migrated += 1
        reports.append(f"Migrated network diagnostics ({migrated} rows)")

    if _table_exists(connection, "game_profiles"):
        game_rows = connection.execute(
            """
            SELECT game_id, game_name, executable_path, payload_json, updated_at, profile_id, display_name,
                   recommendations_json, recommendation_basis_json, stability_score, fingerprint_json, last_benchmark_id,
                   active_flag, created_at
            FROM game_profiles
            """
        ).fetchall()
        for row in game_rows:
            payload = json.loads(row["payload_json"]) if row["payload_json"] else {}
            profile_id = row["profile_id"] or f"profile-{row['game_id']}"
            connection.execute(
                """
                UPDATE game_profiles
                SET profile_id = ?,
                    display_name = COALESCE(display_name, ?),
                    recommendations_json = COALESCE(recommendations_json, ?),
                    recommendation_basis_json = COALESCE(recommendation_basis_json, ?),
                    stability_score = COALESCE(stability_score, ?),
                    fingerprint_json = COALESCE(fingerprint_json, ?),
                    last_benchmark_id = COALESCE(last_benchmark_id, ?),
                    active_flag = COALESCE(active_flag, 1),
                    created_at = COALESCE(created_at, updated_at)
                WHERE game_id = ?
                """,
                (
                    profile_id,
                    payload.get("display_name") or payload.get("game_name") or row["game_name"],
                    json.dumps(payload.get("recommended_tweaks") or []),
                    json.dumps(payload.get("recommendation_basis") or {}),
                    payload.get("stability_score"),
                    json.dumps(payload.get("fingerprint") or {}),
                    payload.get("last_benchmark_id"),
                    row["game_id"],
                ),
            )

    if _table_exists(connection, "app_state"):
        row = connection.execute(
            "SELECT value_json FROM app_state WHERE state_key = 'optimizer.active_temporary_tweaks' LIMIT 1"
        ).fetchone()
        if row and row["value_json"]:
            payload = json.loads(row["value_json"])
            items = payload.get("items") if isinstance(payload, dict) else []
            migrated = 0
            for item in items or []:
                snapshot_id = item.get("snapshot_id")
                if not snapshot_id:
                    continue
                exists = connection.execute(
                    "SELECT 1 FROM tweak_applications WHERE revert_snapshot_id = ? LIMIT 1",
                    (snapshot_id,),
                ).fetchone()
                if exists:
                    continue
                connection.execute(
                    """
                    INSERT INTO tweak_applications (
                        application_id, tweak_id, session_id, applied_at, before_value, after_value,
                        apply_result, triggered_by, temporary_flag, revert_snapshot_id, target, metadata_json
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        str(uuid.uuid4()),
                        item.get("tweak_id") or "unknown_tweak",
                        item.get("session_id"),
                        float(item.get("applied_at") or time.time()),
                        None,
                        None,
                        "success",
                        "legacy_migration",
                        1,
                        snapshot_id,
                        item.get("target"),
                        json.dumps(item),
                    ),
                )
                migrated += 1
            if migrated:
                reports.append(f"Migrated temporary tweak lifecycle rows ({migrated} rows)")

    if _table_exists(connection, "account_identities"):
        identity_rows = connection.execute(
            "SELECT account_id, email, display_name, avatar_url, updated_at FROM account_identities"
        ).fetchall()
        migrated = 0
        for row in identity_rows:
            plan_row = connection.execute(
                "SELECT plan_tier, status FROM account_plans WHERE account_id = ? LIMIT 1",
                (row["account_id"],),
            ).fetchone()
            connection.execute(
                """
                INSERT OR REPLACE INTO account_cache (
                    account_id, email, display_name, avatar_url, current_plan, status, last_synced_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    row["account_id"],
                    row["email"],
                    row["display_name"],
                    row["avatar_url"],
                    plan_row["plan_tier"] if plan_row else None,
                    plan_row["status"] if plan_row else None,
                    row["updated_at"],
                ),
            )
            migrated += 1
        if migrated:
            reports.append(f"Projected account cache rows ({migrated} rows)")

    if _table_exists(connection, "adaptive_actions"):
        connection.execute(
            """
            UPDATE adaptive_actions
            SET rule_name = COALESCE(rule_name, rule_id, title),
                trigger_reason = COALESCE(trigger_reason, 'adaptive_engine'),
                action_name = COALESCE(action_name, tweak_id),
                cooldown_key = COALESCE(cooldown_key, rule_id, tweak_id),
                outcome = COALESCE(outcome, CASE WHEN success = 1 THEN 'success' ELSE 'failed' END),
                notes = COALESCE(notes, rationale)
            """
        )

    if _table_exists(connection, "entitlement_snapshots"):
        ent_rows = connection.execute(
            "SELECT account_id, plan_tier, payload_json, generated_at, source FROM entitlement_snapshots"
        ).fetchall()
        for row in ent_rows:
            payload = json.loads(row["payload_json"]) if row["payload_json"] else {}
            features = payload.get("features") or {}
            verification_status = payload.get("verification_status") or ("signed_out" if row["account_id"] == "__signed_out__" else "cached")
            snapshot_id = f"{row['account_id']}:{int(float(row['generated_at']))}"
            connection.execute(
                """
                INSERT OR IGNORE INTO entitlement_snapshot_history (
                    snapshot_id, account_id, generated_at, source, plan_tier, features_json, verification_status, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    snapshot_id,
                    None if row["account_id"] == "__signed_out__" else row["account_id"],
                    row["generated_at"],
                    row["source"],
                    row["plan_tier"],
                    json.dumps(features),
                    verification_status,
                    row["payload_json"],
                ),
            )


def _migration_5_import_legacy_artifact_if_needed(
    connection: sqlite3.Connection,
    db_path: Path,
    legacy_artifact_path: Path | None,
    reports: list[str],
) -> None:
    if not legacy_artifact_path:
        return
    if legacy_artifact_path.resolve() == db_path.resolve():
        return
    if not legacy_artifact_path.exists():
        reports.append(f"No legacy DB artifact found at {legacy_artifact_path}")
        return

    connection.execute("ATTACH DATABASE ? AS legacy_db", (str(legacy_artifact_path),))
    try:
        copied_tables = 0
        for table_name in (
            "metrics",
            "anomalies",
            "actions",
            "baselines",
            "learning",
            "usage_counters",
            "optimizations",
            "process_intelligence",
            "scheduled_tasks",
            "efficiency_history",
        ):
            if not _table_exists(connection, table_name):
                continue
            if not _table_exists(connection, table_name, schema="legacy_db"):
                continue
            target_count = int(connection.execute(f"SELECT COUNT(1) FROM {table_name}").fetchone()[0] or 0)
            source_count = int(connection.execute(f"SELECT COUNT(1) FROM legacy_db.{table_name}").fetchone()[0] or 0)
            if source_count <= 0 or target_count > 0:
                continue
            column_rows = connection.execute(f"PRAGMA table_info({table_name})").fetchall()
            ordered_columns = [str(item[1]) for item in column_rows]
            csv_columns = ", ".join(ordered_columns)
            connection.execute(
                f"INSERT INTO {table_name} ({csv_columns}) SELECT {csv_columns} FROM legacy_db.{table_name}"
            )
            copied_tables += 1
            reports.append(
                f"Imported legacy artifact table {table_name} from {legacy_artifact_path} ({source_count} rows)"
            )
        _upsert_metadata(
            connection,
            "legacy_artifact_last_checked",
            {"path": str(legacy_artifact_path), "checked_at": time.time(), "copied_tables": copied_tables},
            value_type="json",
            source="migration",
        )
    finally:
        # DETACH can fail with "database is locked" when the legacy DB is in WAL mode and a
        # WAL checkpoint cannot complete on that attached connection.  Catch and log instead of
        # propagating an exception that crashes FastAPI startup – the import data was already
        # written to the main DB before reaching this point.
        try:
            connection.execute("DETACH DATABASE legacy_db")
        except sqlite3.OperationalError as _detach_err:
            LOGGER.warning(
                "[DB Migration] Could not DETACH legacy_db (%s). The legacy import may still have "
                "succeeded. Run 'PRAGMA wal_checkpoint(FULL)' on %s to clear WAL state if needed.",
                _detach_err,
                legacy_artifact_path,
            )


def _migration_6_enforce_game_profile_uniqueness(
    connection: sqlite3.Connection,
    _db_path: Path,
    _legacy_artifact_path: Path | None,
    reports: list[str],
) -> None:
    if not _table_exists(connection, "game_profiles"):
        return

    # Backfill nullable profile IDs from earlier schema revisions.
    connection.execute(
        """
        UPDATE game_profiles
        SET profile_id = 'profile-' || game_id || '-' || rowid
        WHERE profile_id IS NULL OR TRIM(profile_id) = ''
        """
    )

    duplicate_rows = connection.execute(
        """
        SELECT game_id, COUNT(1) AS duplicate_count
        FROM game_profiles
        GROUP BY game_id
        HAVING COUNT(1) > 1
        """
    ).fetchall()
    removed = 0
    for row in duplicate_rows:
        game_id = str(row["game_id"])
        candidates = connection.execute(
            """
            SELECT rowid
            FROM game_profiles
            WHERE game_id = ?
            ORDER BY COALESCE(updated_at, 0) DESC, rowid DESC
            """,
            (game_id,),
        ).fetchall()
        # Keep newest row and delete older duplicates.
        for candidate in candidates[1:]:
            connection.execute("DELETE FROM game_profiles WHERE rowid = ?", (candidate["rowid"],))
            removed += 1

    connection.execute(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_game_profiles_game_unique ON game_profiles(game_id)"
    )
    reports.append(
        f"Enforced game_profiles uniqueness on game_id (removed_duplicates={removed})"
    )


def _migration_7_add_health_history(
    connection: sqlite3.Connection,
    _db_path: Path,
    _legacy_artifact_path: Path | None,
    reports: list[str],
) -> None:
    connection.executescript(
        """
        CREATE TABLE IF NOT EXISTS health_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            score REAL NOT NULL,
            cpu_load REAL NOT NULL,
            ram_percent REAL NOT NULL,
            gpu_temp REAL,
            timestamp REAL NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_health_history_timestamp ON health_history(timestamp DESC);
        """
    )
    reports.append("Ensured health_history table and index for long-window health snapshots.")


_MIGRATIONS: tuple[tuple[int, str, MigrationFn], ...] = (
    (1, "create_base_schema", _migration_1_create_base_schema),
    (2, "expand_existing_schema", _migration_2_expand_existing_schema),
    (3, "migrate_settings_and_runtime", _migration_3_migrate_settings_and_runtime),
    (4, "migrate_domain_tables", _migration_4_migrate_domain_tables),
    (5, "import_legacy_artifact_if_needed", _migration_5_import_legacy_artifact_if_needed),
    (6, "enforce_game_profile_uniqueness", _migration_6_enforce_game_profile_uniqueness),
    (7, "add_health_history", _migration_7_add_health_history),
)
