import asyncio
import json
import sqlite3
import tempfile
import time
import unittest
from contextlib import closing
from pathlib import Path

from core.database import DatabaseService
from core.db.migrations import SCHEMA_VERSION, ensure_database_schema
from core.models import BenchmarkResult, RevertSnapshot


class Phase2DatabaseMigrationTests(unittest.TestCase):
    def test_schema_versioning_and_idempotent_migrations(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = Path(temp_dir) / "phase2.db"
            first = ensure_database_schema(db_path)
            second = ensure_database_schema(db_path)

            self.assertGreaterEqual(len(first), 1)
            self.assertEqual(second, [])

            with closing(sqlite3.connect(db_path)) as connection:
                version_row = connection.execute(
                    "SELECT MAX(version) FROM schema_migrations"
                ).fetchone()
                meta_row = connection.execute(
                    "SELECT meta_value FROM app_metadata WHERE meta_key = 'schema_version'"
                ).fetchone()
                table_row = connection.execute(
                    "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'benchmark_results'"
                ).fetchone()
                legacy_table_rows = connection.execute(
                    """
                    SELECT name
                    FROM sqlite_master
                    WHERE type = 'table'
                      AND name IN ('app_state', 'revert_snapshots', 'benchmarks', 'network_diagnostics')
                    """
                ).fetchall()

            self.assertEqual(int(version_row[0]), SCHEMA_VERSION)
            self.assertEqual(int(json.loads(meta_row[0])), SCHEMA_VERSION)
            self.assertIsNotNone(table_row)
            self.assertEqual(len(legacy_table_rows), 0)

    def test_migrates_legacy_state_and_payload_tables(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = Path(temp_dir) / "phase2_legacy.db"
            with closing(sqlite3.connect(db_path)) as connection:
                connection.executescript(
                    """
                    CREATE TABLE app_state (
                        state_key TEXT PRIMARY KEY,
                        value_json TEXT NOT NULL,
                        updated_at REAL NOT NULL
                    );
                    CREATE TABLE revert_snapshots (
                        snapshot_id TEXT PRIMARY KEY,
                        target_type TEXT NOT NULL,
                        target_id TEXT NOT NULL,
                        before_value TEXT NOT NULL,
                        created_at REAL NOT NULL,
                        session_id TEXT,
                        restored INTEGER NOT NULL DEFAULT 0,
                        restored_at REAL,
                        restore_status TEXT,
                        metadata_json TEXT
                    );
                    CREATE TABLE benchmarks (
                        benchmark_id TEXT PRIMARY KEY,
                        workload_name TEXT NOT NULL,
                        created_at REAL NOT NULL,
                        duration_seconds INTEGER NOT NULL,
                        session_id TEXT,
                        verdict TEXT NOT NULL,
                        tweak_set_json TEXT NOT NULL,
                        payload_json TEXT NOT NULL
                    );
                    CREATE TABLE network_diagnostics (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp REAL NOT NULL,
                        payload_json TEXT NOT NULL
                    );
                    """
                )
                now = time.time()
                connection.execute(
                    "INSERT INTO app_state (state_key, value_json, updated_at) VALUES (?, ?, ?)",
                    ("settings.expert_mode", json.dumps({"enabled": True}), now),
                )
                connection.execute(
                    "INSERT INTO app_state (state_key, value_json, updated_at) VALUES (?, ?, ?)",
                    ("runtime.current", json.dumps({"session_id": "s-1", "active": True}), now),
                )
                connection.execute(
                    "INSERT INTO app_state (state_key, value_json, updated_at) VALUES (?, ?, ?)",
                    (
                        "optimizer.active_temporary_tweaks",
                        json.dumps(
                            {
                                "items": [
                                    {
                                        "snapshot_id": "snap-123",
                                        "tweak_id": "search_indexer_priority",
                                        "target": "SearchIndexer.exe",
                                        "applied_at": now,
                                    }
                                ]
                            }
                        ),
                        now,
                    ),
                )
                connection.execute(
                    """
                    INSERT INTO revert_snapshots (
                        snapshot_id, target_type, target_id, before_value, created_at, session_id, restored, restore_status, metadata_json
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        "snap-legacy",
                        "service_start_type",
                        "WSearch",
                        json.dumps({"start_type": "auto"}),
                        now,
                        "session-legacy",
                        0,
                        "pending",
                        json.dumps({"source": "legacy-test"}),
                    ),
                )
                benchmark_payload = {
                    "session_id": "session-legacy",
                    "workload_name": "Valorant",
                    "avg_fps_delta": 12.5,
                    "one_percent_low_delta": 8.0,
                    "frame_time_variance_delta": -1.2,
                    "cpu_delta": -4.0,
                    "gpu_delta": 3.0,
                    "ping_delta": -2.0,
                    "jitter_delta": -1.0,
                    "verdict": "improved",
                    "unsupported_reasons": [],
                }
                connection.execute(
                    """
                    INSERT INTO benchmarks (
                        benchmark_id, workload_name, created_at, duration_seconds, session_id, verdict, tweak_set_json, payload_json
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        "bench-legacy",
                        "Valorant",
                        now,
                        45,
                        "session-legacy",
                        "improved",
                        json.dumps(["search_indexer_priority"]),
                        json.dumps(benchmark_payload),
                    ),
                )
                network_payload = {
                    "timestamp": now,
                    "session_id": "session-legacy",
                    "summary": {"latency_ms": 22.0, "jitter_ms": 1.2, "packet_loss_percent": 0.0},
                    "qos": {"supported": True},
                }
                connection.execute(
                    "INSERT INTO network_diagnostics (timestamp, payload_json) VALUES (?, ?)",
                    (now, json.dumps(network_payload)),
                )
                connection.commit()

            reports = ensure_database_schema(db_path)
            self.assertTrue(any("Migrated" in line for line in reports))

            with closing(sqlite3.connect(db_path)) as connection:
                settings_row = connection.execute(
                    "SELECT value_text FROM settings WHERE category = 'settings' AND key_name = 'expert_mode'"
                ).fetchone()
                runtime_row = connection.execute(
                    "SELECT value_json FROM runtime_housekeeping WHERE runtime_key = 'current'"
                ).fetchone()
                rollback_row = connection.execute(
                    "SELECT 1 FROM rollback_snapshots WHERE snapshot_id = 'snap-legacy'"
                ).fetchone()
                bench_run = connection.execute(
                    "SELECT 1 FROM benchmark_runs WHERE benchmark_id = 'bench-legacy'"
                ).fetchone()
                bench_result = connection.execute(
                    "SELECT 1 FROM benchmark_results WHERE benchmark_id = 'bench-legacy'"
                ).fetchone()
                network_row = connection.execute(
                    "SELECT 1 FROM network_snapshots WHERE session_id = 'session-legacy'"
                ).fetchone()
                tweak_row = connection.execute(
                    "SELECT 1 FROM tweak_applications WHERE revert_snapshot_id = 'snap-123'"
                ).fetchone()

            self.assertEqual(json.loads(settings_row[0])["enabled"], True)
            self.assertEqual(json.loads(runtime_row[0])["session_id"], "s-1")
            self.assertIsNotNone(rollback_row)
            self.assertIsNotNone(bench_run)
            self.assertIsNotNone(bench_result)
            self.assertIsNotNone(network_row)
            self.assertIsNotNone(tweak_row)

    def test_database_service_persists_new_domain_tables(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = Path(temp_dir) / "phase2_service.db"
            database = DatabaseService(db_path)
            asyncio.run(database.initialize())

            asyncio.run(database.set_setting("expert_mode", {"enabled": True}, source="test"))
            self.assertEqual(asyncio.run(database.get_setting("expert_mode"))["enabled"], True)

            asyncio.run(database.set_runtime_state("current", {"session_id": "s-2", "active": True}))
            self.assertEqual(asyncio.run(database.get_runtime_state("current"))["session_id"], "s-2")

            application_id = asyncio.run(
                database.create_tweak_application(
                    tweak_id="search_indexer_priority",
                    session_id="s-2",
                    triggered_by="test",
                    temporary_flag=True,
                    apply_result="success",
                    revert_snapshot_id="snap-phase2",
                    target="SearchIndexer.exe",
                    metadata={"from": "test"},
                )
            )
            self.assertTrue(application_id)
            active = asyncio.run(database.list_active_tweak_applications())
            self.assertEqual(active[0]["snapshot_id"], "snap-phase2")

            asyncio.run(
                database.insert_gpu_snapshot(
                    {
                        "timestamp": time.time(),
                        "vendor": "nvidia",
                        "model": "RTX Test",
                        "utilization_percent": 55.0,
                        "memory_used_mb": 2048,
                        "temperature_c": 62.0,
                        "clock_mhz": 1800,
                        "telemetry_supported": True,
                    },
                    session_id="s-2",
                )
            )
            gpu_latest = asyncio.run(database.latest_gpu_snapshot())
            self.assertEqual(gpu_latest["vendor"], "nvidia")
            self.assertEqual(gpu_latest["session_id"], "s-2")

            asyncio.run(
                database.insert_trust_center_state(
                    {
                        "rollback_readiness": {"ready": True},
                        "crash_recovery_status": {"status": "clear"},
                        "protected_process_rules": ["lsass.exe"],
                        "dangerous_tweaks_disabled": True,
                        "expert_mode_state": False,
                        "unsupported_capabilities": [],
                        "safeguard_summary": ["ok"],
                    }
                )
            )
            with closing(sqlite3.connect(db_path)) as connection:
                trust_row = connection.execute(
                    "SELECT 1 FROM trust_center_state LIMIT 1"
                ).fetchone()
            self.assertIsNotNone(trust_row)

    def test_service_no_longer_writes_legacy_mirror_tables(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            db_path = Path(temp_dir) / "phase2_cleanup.db"
            database = DatabaseService(db_path)
            asyncio.run(database.initialize())

            with closing(sqlite3.connect(db_path)) as connection:
                connection.executescript(
                    """
                    CREATE TABLE IF NOT EXISTS app_state (
                        state_key TEXT PRIMARY KEY,
                        value_json TEXT NOT NULL,
                        updated_at REAL NOT NULL
                    );
                    CREATE TABLE IF NOT EXISTS revert_snapshots (
                        snapshot_id TEXT PRIMARY KEY,
                        target_type TEXT NOT NULL,
                        target_id TEXT NOT NULL,
                        before_value TEXT NOT NULL,
                        created_at REAL NOT NULL,
                        session_id TEXT,
                        restored INTEGER NOT NULL DEFAULT 0,
                        restored_at REAL,
                        restore_status TEXT,
                        metadata_json TEXT
                    );
                    CREATE TABLE IF NOT EXISTS benchmarks (
                        benchmark_id TEXT PRIMARY KEY,
                        workload_name TEXT NOT NULL,
                        created_at REAL NOT NULL,
                        duration_seconds INTEGER NOT NULL,
                        session_id TEXT,
                        verdict TEXT NOT NULL,
                        tweak_set_json TEXT NOT NULL,
                        payload_json TEXT NOT NULL
                    );
                    CREATE TABLE IF NOT EXISTS network_diagnostics (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp REAL NOT NULL,
                        payload_json TEXT NOT NULL
                    );
                    """
                )
                connection.commit()

            asyncio.run(database.set_app_state("test.key", {"value": 7}))
            self.assertEqual(asyncio.run(database.get_app_state("test.key"))["value"], 7)

            asyncio.run(
                database.insert_revert_snapshot(
                    RevertSnapshot(
                        snapshot_id="phase3-snap",
                        target_type="service_start_type",
                        target_id="WSearch",
                        before_value={"start_type": "auto"},
                        created_at=time.time(),
                    )
                )
            )
            asyncio.run(
                database.insert_benchmark_result(
                    BenchmarkResult(
                        benchmark_id="phase3-bench",
                        workload_name="Valorant",
                        created_at=time.time(),
                        duration_seconds=30,
                        tweak_set=["search_indexer_priority"],
                        verdict="improved",
                    )
                )
            )
            asyncio.run(
                database.insert_network_diagnostics(
                    {
                        "timestamp": time.time(),
                        "session_id": "phase3-session",
                        "summary": {"latency_ms": 20.0, "jitter_ms": 1.0, "packet_loss_percent": 0.0},
                        "qos": {"supported": True},
                    }
                )
            )

            with closing(sqlite3.connect(db_path)) as connection:
                app_state_count = connection.execute("SELECT COUNT(1) FROM app_state").fetchone()[0]
                revert_count = connection.execute("SELECT COUNT(1) FROM revert_snapshots").fetchone()[0]
                bench_count = connection.execute("SELECT COUNT(1) FROM benchmarks").fetchone()[0]
                network_count = connection.execute("SELECT COUNT(1) FROM network_diagnostics").fetchone()[0]
                rollback_count = connection.execute(
                    "SELECT COUNT(1) FROM rollback_snapshots WHERE snapshot_id = 'phase3-snap'"
                ).fetchone()[0]
                benchmark_count = connection.execute(
                    "SELECT COUNT(1) FROM benchmark_results WHERE benchmark_id = 'phase3-bench'"
                ).fetchone()[0]
                snapshot_count = connection.execute(
                    "SELECT COUNT(1) FROM network_snapshots WHERE session_id = 'phase3-session'"
                ).fetchone()[0]

            self.assertEqual(app_state_count, 0)
            self.assertEqual(revert_count, 0)
            self.assertEqual(bench_count, 0)
            self.assertEqual(network_count, 0)
            self.assertEqual(rollback_count, 1)
            self.assertEqual(benchmark_count, 1)
            self.assertEqual(snapshot_count, 1)


if __name__ == "__main__":
    unittest.main()
