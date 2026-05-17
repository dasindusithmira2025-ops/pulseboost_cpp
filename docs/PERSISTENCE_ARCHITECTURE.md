# PulseBoost Persistence Architecture

## Scope
PulseBoost now uses one migration-owned SQLite persistence layer for application state, session history, optimization evidence, trust posture, account metadata, and cognition telemetry.  
Primary DB path remains `pulseboost/data/memory.db` (configurable via `DB_PATH`).

## Single Database Owner
- Schema/migration owner: `pulseboost/core/db/migrations.py`
- Runtime data access owner: `pulseboost/core/db/service.py`
- Compatibility import path: `pulseboost/core/database.py` now re-exports the unified service only.
- Cognition storage (`MemorySystem`) no longer creates its own schema and now delegates DB bootstrap to the migration engine.

## Migration Strategy
- Schema version table: `schema_migrations`
- Current schema version: `6`
- Migrations are ordered and idempotent and run through `ensure_database_schema(...)`.
- Migration metadata is recorded in `app_metadata`:
  - `schema_version`
  - `last_migrated_at`
  - `legacy_artifact_last_checked`
- Legacy artifact import path is explicitly handled for stale secondary DB artifacts (`../data/memory.db` relative to project DB root).

## Core Normalized Tables
- `app_metadata`
- `settings`
- `runtime_housekeeping`
- `sessions`
- `session_actions`
- `tweak_catalog`
- `tweak_applications`
- `audit_entries`
- `rollback_snapshots`
- `benchmark_runs`
- `benchmark_results`
- `adaptive_actions`
- `network_snapshots`
- `gpu_snapshots`
- `game_profiles` (+ unique `game_id` enforcement)
- `trust_center_state`
- `account_cache`
- `account_identities`
- `auth_sessions`
- `account_plans`
- `entitlement_snapshots`
- `entitlement_snapshot_history`
- `device_activations`
- `ui_notifications` (reserved for persisted UI event usage)

Legacy compatibility tables are retired from active feature writes.  
Fresh installs do not create `app_state`, `revert_snapshots`, `benchmarks`, or `network_diagnostics`; migration code still imports from them when they exist on upgraded installs.

## Data Migration Coverage
Implemented migration projection/backfill from legacy storage into normalized tables:
- `app_state.settings.*` -> `settings`
- `app_state.runtime.*` -> `runtime_housekeeping`
- `app_state.optimizer.active_temporary_tweaks` -> `tweak_applications`
- `revert_snapshots` -> `rollback_snapshots`
- `benchmarks` -> `benchmark_runs` + `benchmark_results`
- `network_diagnostics` -> `network_snapshots`
- `adaptive_actions` -> normalized action fields
- `game_profiles.payload_json` -> normalized profile columns
- `account_identities` + `account_plans` -> `account_cache`
- `entitlement_snapshots` -> `entitlement_snapshot_history`
- cognition tables can be imported from legacy DB artifact when target tables are empty.

## Secure Storage Boundary
- **Not stored in plaintext DB**:
  - access tokens
  - refresh tokens
- Tokens are encrypted by `SecretCipher` / DPAPI-backed cipher before persistence.
- DB stores non-sensitive identity/session/plan/entitlement/activation metadata plus encrypted token blobs.

## Feature Wiring (Current)
- Settings read/write: normalized `settings` rows
- Runtime housekeeping: `runtime_housekeeping`
- Tweak apply/revert lifecycle: `tweak_applications`
- Audit and rollback: `audit_entries`, `rollback_snapshots`
- Benchmark history/results: `benchmark_runs`, `benchmark_results`
- Adaptive action history: `adaptive_actions`
- Network diagnostics snapshots: `network_snapshots`
- GPU telemetry snapshots: `gpu_snapshots`
- Game profiles: `game_profiles`
- Trust center snapshots: `trust_center_state`
- Account/auth/entitlement/activation metadata: dedicated account/auth tables listed above

## Future Website Authority / Sync Preparation
- Account/auth/entitlement/device state is split into discrete tables to support future upstream authority reconciliation.
- `app_metadata`, `runtime_housekeeping`, and timestamped history tables are structured for eventual outbound sync and conflict resolution.
- Compatibility tables remain only as transitional surfaces; new writes should continue to use dedicated domain tables.
