# PulseBoost API Reference (Current)

Source of truth:
- `pulseboost/api/routes.py`
- `pulseboost/api/schemas.py`
- `pulseboost/api/main.py` (`/healthz`, `/ws`)

Base path: `/api` unless otherwise noted.

Status labels in this document:
- `Working`
- `Working (capability-limited)`
- `Dry-run`
- `Placeholder`
- `Future website-dependent`

## 1. Health and Realtime

### GET `/healthz` (no `/api` prefix)
- Purpose: backend liveness check.
- Request: none.
- Response:
  - `{ "ok": true }`
- Status: `Working`

### GET `/api/metrics/live`
- Purpose: SSE metrics stream.
- Request: none.
- Response: `text/event-stream` from `metrics_service.stream(orchestrator)`.
- Status: `Working`

### WS `/ws` (no `/api` prefix)
- Purpose: realtime full-state broadcast and chat/decision commands.
- Client messages:
  - `{ "type": "chat", "message": string, "history": [...] }`
  - `{ "type": "state_request" }`
  - `{ "type": "optimization_decision", "optimization_id": string, "decision": "approve|dismiss" }`
- Server messages include:
  - `full_state`
  - `chat_token`
  - `chat_done`
  - `optimization_decision_result`
  - `system_error`
  - `game_closed`
- Status: `Working (capability-limited)` (chat quality depends on API key and plan limits)

## 2. Core State and Capabilities

### GET `/api/status`
- Purpose: aggregated runtime/foundation status.
- Request: none.
- Response keys include:
  - `ok`, `runtime`, `machine`, `plan`, `plan_info`, `executor`, `recovery`, `capabilities`, `hardware_profile`
  - `active_temporary_tweaks`, `active_temporary_tweak_items`
  - `benchmark_running`, `adaptive_engine`, `network_diagnostics`, `expert_mode`
  - `auth`, `feature_access`, `entitlement_snapshot`
- Status: `Working`

### GET `/api/capabilities`
- Purpose: capability and hardware snapshot payload.
- Request: none.
- Response:
  - `{ "capability_snapshot": {...}, "hardware_profile": {...} }`
- Notes: response cache TTL ~60s.
- Status: `Working`

### GET `/api/state`
- Purpose: orchestrator state payload for current plan context.
- Request: none.
- Response: full orchestrator state object (metrics, health, anomalies, predictions, optimizations, plan/auth/access context).
- Status: `Working`

### GET `/api/metrics`
- Purpose: lightweight state metrics read.
- Request: none.
- Response:
  - `{ "metrics": {...}, "health_score": number, "session_mode": string, "active_session": {...} }`
- Status: `Working`

## 3. Tweaks

### GET `/api/tweaks`
- Purpose: optimization catalog with active/apply metadata.
- Request: none.
- Response: array of tweak objects with augmented fields:
  - `applied`, `active_snapshot_id`, `active_target`
- Status: `Working`

### POST `/api/tweaks/{tweak_id}/apply`
- Purpose: apply one tweak.
- Request body (`TweakApplyRequest`):
  - `{ "params": object }`
- Response: optimizer apply result object.
- Errors:
  - `400` invalid tweak/params or apply failure.
- Status: `Working`

### POST `/api/tweaks/{tweak_id}/revert`
- Purpose: revert one tweak by id and optional snapshot.
- Request body (`TweakRevertRequest`):
  - `{ "snapshot_id": string|null }`
- Response: optimizer revert result object.
- Errors:
  - `400` invalid request or revert failure.
- Status: `Working`

## 4. Settings

### GET `/api/settings`
- Purpose: settings + plan + auth payload used by settings/account UI.
- Request: none.
- Response keys include:
  - `plan`, `plan_info`, `features`, `feature_access`
  - `auth_enabled`, `auth_dev_mode`, `stripe_public_key`
  - `machine`, `executor`, `expert_mode`, `preferences`, `auth`
- Status: `Working`

### POST `/api/settings/expert-mode`
- Purpose: enable/disable expert mode gate.
- Request body (`ExpertModeRequest`):
  - `{ "enabled": boolean }`
- Response:
  - `{ "enabled": boolean }`
- Status: `Working`

### POST `/api/settings/preferences`
- Purpose: persist settings preferences map.
- Request body (`SettingsPreferencesRequest`):
  - `{ "preferences": object }`
- Response:
  - `{ "preferences": object }` (merged with defaults)
- Status: `Working`

### POST `/api/settings/data-action`
### POST `/api/settings/data-actions`
- Purpose: perform one of supported settings data operations.
- Request body (`SettingsDataActionRequest`):
  - `{ "action": "export_all_data|import_settings|reset_all_settings|export_report|clear_benchmark_history", "payload": object }`
- Response: action-specific payload, always includes `success` and `action` on success.
- Action behavior:
  - `export_all_data`: returns export payload snapshot
  - `import_settings`: imports and persists normalized settings
  - `clear_benchmark_history`: deletes benchmark history rows
  - `reset_all_settings`: resets to safe defaults
  - `export_report`: writes markdown report and returns path
- Errors:
  - `400` invalid import payload
- Status: `Working`

## 5. Auth, Account, Licensing

### GET `/api/auth/status`
- Purpose: full auth/session/plan/entitlement/activation status.
- Request: none.
- Response keys include:
  - `signed_in`, `identity`, `session`, `plan`, `entitlement_snapshot`, `feature_access`, `activation`, `authority`, `links`
- Status: `Working`

### POST `/api/auth/local-session`
- Purpose: local placeholder sign-in for dev mode.
- Request body (`LocalPlaceholderSignInRequest`):
  - `{ "email": string, "display_name": string, "plan_tier": string }`
- Response: auth status payload for created session.
- Errors:
  - `403` when `AUTH_DEV_MODE` disabled
  - `400` invalid email or payload
- Status: `Working (capability-limited)`

### POST `/api/auth/token-exchange`
- Purpose: website token exchange hook.
- Request body (`TokenExchangeRequest`):
  - `{ "provider": string, "authorization_code": string }`
- Response: placeholder exchange payload from auth service.
- Status: `Placeholder` / `Future website-dependent`

### POST `/api/auth/refresh-entitlements`
- Purpose: refresh entitlement snapshot.
- Request: none.
- Response: refreshed auth/entitlement context payload.
- Status: `Working (future website-dependent for full authority)`

### POST `/api/auth/sign-out`
- Purpose: clear session.
- Request: none.
- Response: sign-out result payload.
- Status: `Working`

### GET `/api/account/identity`
- Purpose: current identity summary.
- Response:
  - `{ "signed_in": boolean, "identity": object|null }`
- Status: `Working`

### GET `/api/account/plan`
- Purpose: current plan summary.
- Response:
  - `{ "signed_in": boolean, "plan": object }`
- Status: `Working`

### GET `/api/account/entitlements`
- Purpose: entitlement snapshot + feature access map.
- Response:
  - `{ "signed_in": boolean, "entitlement_snapshot": object, "feature_access": object }`
- Status: `Working`

### GET `/api/account/activation`
- Purpose: device activation summary.
- Response:
  - `{ "signed_in": boolean, "activation": object|null }`
- Status: `Working`

## 6. Audit and Action History

### GET `/api/audit`
- Purpose: recent audit rows.
- Query params:
  - `limit` default `100`, max `500`
- Response: array of audit entries.
- Status: `Working`

### POST `/api/audit/{audit_id}/revert`
- Purpose: revert using audit-linked snapshot.
- Request: none.
- Response: revert result object.
- Errors:
  - `404` audit row missing
  - `409` row has no revert snapshot
  - `400` revert failure
- Status: `Working (capability-limited)`

### GET `/api/actions`
- Purpose: recent action rows.
- Query params:
  - `limit` default `25`, max `200`
- Response: array of action rows.
- Status: `Working (plan-feature-limited; empty if audit feature disabled)`

### GET `/api/actions/export`
- Purpose: export actions CSV.
- Query params:
  - `limit` default `200`, max `1000`
- Response: CSV text (`text/plain`).
- Errors:
  - `403` if `audit_export` entitlement disabled
- Status: `Working (capability-limited)`

## 7. Timeline, Health, Anomalies, Optimizations

### GET `/api/timeline`
- Purpose: historical timeline points.
- Query params:
  - `limit` default `300`, max `1000`
  - `hours` optional (`1..720`), clamped by plan window
- Response: timeline array.
- Status: `Working`

### GET `/api/health/history`
- Purpose: health history rows.
- Query params:
  - `days` default `7`, range `1..30`
- Response: array of health history points.
- Notes: response cache TTL ~30s.
- Status: `Working`

### GET `/api/timeline/snapshot`
- Purpose: nearest timeline snapshot lookup.
- Query params:
  - `timestamp` required (float)
- Response: snapshot object.
- Errors:
  - `404` no snapshot found
- Status: `Working`

### GET `/api/anomalies`
- Purpose: recent anomalies.
- Query params:
  - `limit` default `10`, max `100`
  - `unresolved_only` boolean
- Response: anomalies array.
- Status: `Working`

### GET `/api/suggestions`
- Purpose: smart suggestion feed.
- Response: array of suggestion items.
- Status: `Working`

### GET `/api/optimizations`
- Purpose: recent optimizations feed.
- Query params:
  - `limit` default `25`, max `200`
- Response: optimizations array.
- Status: `Working`

### POST `/api/actions/preview`
- Purpose: dry preview reasoning for arbitrary action.
- Request body (`ActionRequest`):
  - `{ "action_type": string, "params": object }`
- Response: reasoner output with proceed/confirmation guidance.
- Status: `Working`

### POST `/api/actions/execute`
- Purpose: execute manual action through reasoner/executor.
- Request body (`ExecuteActionRequest`):
  - `{ "action_type": string, "params": object, "confirmed": boolean }`
- Response: execution result payload.
- Errors:
  - `409` action blocked by reasoner/safety gate
- Status: `Working (capability-limited; may run dry-run depending on config/action)`

### POST `/api/optimizations/decision`
- Purpose: approve/dismiss an optimization recommendation.
- Request body (`OptimizationDecisionRequest`):
  - `{ "optimization_id": string, "decision": "approve|dismiss" }`
- Response: decision result.
- Errors:
  - `409` blocked decision
  - `404` optimization not found/failed
- Status: `Working`

## 8. Adaptive Engine

### GET `/api/adaptive/status`
- Purpose: adaptive state and recent actions.
- Response: adaptive status object.
- Status: `Working`

### POST `/api/adaptive/toggle`
- Purpose: enable/disable adaptive engine.
- Request body (`AdaptiveToggleRequest`):
  - `{ "enabled": boolean }`
- Response: toggle result/state.
- Status: `Working`

## 9. Network

### GET `/api/network/diagnostics`
- Purpose: collect and return diagnostics payload.
- Response includes:
  - `protocol_profile`, `nic_capabilities`, `targets` (`router`, `public`, `game_server`), `summary`, `qos`
- Status: `Working (capability-limited)`

### POST `/api/network/qos`
- Purpose: apply QoS profile request.
- Request body (`NetworkQosRequest`):
  - `{ "profile_name": string, "protocol": string }`
- Response: qos apply result (currently dry-run/unsupported semantics).
- Errors:
  - `403` when `advanced_network_controls` entitlement disabled
- Status: `Dry-run` (or unsupported depending runtime/capabilities)

## 10. GPU and BIOS

### GET `/api/gpu/stats`
- Purpose: GPU telemetry and settings surface.
- Response includes:
  - `vendor`, `model`, `driver_version`, clocks/temp/utilization/memory/power fields, `telemetry_supported`, `reason`, `settings`
- Status: `Working (capability-limited)`

### POST `/api/gpu/settings`
- Purpose: apply GPU setting request.
- Request body (`GpuSettingRequest`):
  - `{ "setting_id": string, "value": any, "confirm_risky": boolean }`
- Response: apply result (currently dry-run when supported).
- Errors:
  - `403` when `advanced_gpu_controls` entitlement disabled
- Status: `Dry-run`

### GET `/api/bios/checklist`
- Purpose: advisory BIOS checklist.
- Response: vendor/model + advisory items array.
- Status: `Working`

## 11. Games and Profiles

### GET `/api/games`
- Purpose: list game catalog merged from discovered installs, sessions, benchmarks, and saved profiles.
- Response: game summary array.
- Status: `Working (capability-limited)`

### GET `/api/games/profiles`
- Purpose: cached games/profiles list for profile views.
- Response: same style as games list.
- Notes: response cache TTL ~10s.
- Status: `Working`

### GET `/api/games/{game_id}/profile`
- Purpose: fetch profile for a game id (or generated default).
- Response: profile object with recommendations/history/basis.
- Status: `Working`

### POST `/api/games/{game_id}/profile`
- Purpose: save/update game profile.
- Request body (`GameProfileRequest`):
  - `game_name`, `executable_path`, `recommended_tweaks[]`, `notes`, `history`, `recommendation_basis`
- Response: saved profile object.
- Status: `Working`

### GET `/api/games/{game_id}/export`
- Purpose: export profile payload.
- Response: profile JSON text (`text/plain`) with `format: pbprofile.v1`.
- Status: `Working`

### POST `/api/games/import`
- Purpose: import profile payload.
- Request body (`GameProfileRequest`) payload.
- Response: saved profile object.
- Status: `Working`

## 12. Trust Center

### GET `/api/trust-center/status`
- Purpose: trust status payload for UI.
- Response includes:
  - `admin_status`, `last_clean_exit`, `crash_recovery_status`, `rollback_readiness`, `protected_process_rules`, `dangerous_tweaks_disabled`, `expert_mode_state`, `unsupported_capabilities`, `touch_matrix`, `permission_audit`, `safeguard_summary`
- Status: `Working`

### POST `/api/trust-center/rollback-all`
- Purpose: restore all active temporary tweaks.
- Response:
  - `{ success, restored_count, results[] }`
- Status: `Working`

### POST `/api/trust-center/undo/{category}`
- Purpose: category-level undo request.
- Path param:
  - `category` in `{registry, network}`
- Response:
  - `{ success, category, restored_count, results[] }`
- Errors:
  - `400` unsupported category
- Status: `Working (capability-limited)`

## 13. Benchmark

### POST `/api/benchmark/run`
- Purpose: run benchmark capture.
- Request body (`BenchmarkRunRequest`):
  - `workload_name: string`
  - `tweak_set: string[]`
  - `duration_seconds: 2..30` (default 6)
  - `notes: string`
  - `revert_after: boolean`
- Response:
  - `{ success: true, result: benchmark_result_object }`
- Errors:
  - `409` benchmark failed/already running style errors
- Status: `Working`

### GET `/api/benchmark/results`
- Purpose: list benchmark history.
- Query params:
  - `limit` default `25`, max `100`
- Response: array of benchmark result summaries.
- Errors:
  - `403` when `benchmark_history` entitlement disabled
- Status: `Working (capability-limited)`

### GET `/api/benchmark/results/{benchmark_id}`
- Purpose: fetch one benchmark result.
- Response: benchmark result object.
- Errors:
  - `403` entitlement lock
  - `404` result not found
- Status: `Working (capability-limited)`

### GET `/api/benchmark/results/{benchmark_id}/export`
- Purpose: export benchmark markdown report.
- Response: markdown text.
- Errors:
  - `403` entitlement lock
  - `404` result not found
- Status: `Working (capability-limited)`

## 14. Request Schema Reference

Defined in `api/schemas.py`:
- `ActionRequest`: `action_type`, `params`
- `ExecuteActionRequest`: `action_type`, `params`, `confirmed`
- `OptimizationDecisionRequest`: `optimization_id`, `decision`
- `TweakApplyRequest`: `params`
- `TweakRevertRequest`: `snapshot_id`
- `BenchmarkRunRequest`: `workload_name`, `tweak_set`, `duration_seconds`, `notes`, `revert_after`
- `AdaptiveToggleRequest`: `enabled`
- `NetworkQosRequest`: `profile_name`, `protocol`
- `GpuSettingRequest`: `setting_id`, `value`, `confirm_risky`
- `GameProfileRequest`: `game_name`, `executable_path`, `recommended_tweaks`, `notes`, `history`, `recommendation_basis`
- `ExpertModeRequest`: `enabled`
- `SettingsPreferencesRequest`: `preferences`
- `SettingsDataActionRequest`: `action`, `payload`
- `LocalPlaceholderSignInRequest`: `email`, `display_name`, `plan_tier`
- `TokenExchangeRequest`: `provider`, `authorization_code`

## 15. API Reality Notes for Stakeholders

- Several premium routes are intentionally entitlement-gated at backend level with HTTP 403.
- QoS and GPU control writes are exposed but currently dry-run, by design.
- Auth token exchange route is an explicit placeholder hook for future website authority integration.
- Benchmark payloads include explicit unsupported-metric reasons rather than synthetic values.
