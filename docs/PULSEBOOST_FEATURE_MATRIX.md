# PulseBoost Feature Matrix (Current)

Status classes used:
- `Working`
- `Working (capability-limited)`
- `Dry-run`
- `Placeholder`
- `Unsupported (runtime/hardware)`
- `Future website-dependent`

## Matrix

| Feature Area | Capability | Status | Backend/Core Evidence | API Surface | UI Surface | Notes |
|---|---|---|---|---|---|---|
| Dashboard | Live machine/session summary | Working | `core/agents/orchestrator.py`, `core/metrics.py` | `GET /api/status`, `GET /api/state`, `GET /api/metrics` | `DashboardPage.jsx` | Pulls real live state and trust posture. |
| Dashboard | Live bottleneck diagnosis | Working (capability-limited) | `core/performance_diagnostics.py`, orchestrator metrics enrichment | `GET /api/metrics`, `GET /api/metrics/bottleneck`, `/api/metrics/live` | `DashboardPage.jsx` | Emits `CPU_BOUND`, `GPU_BOUND`, `RAM_BOUND`, `DISK_BOUND`, `NETWORK_BOUND`, `THERMAL_BOUND`, or `BACKGROUND_PROCESS_BOUND` with input evidence. |
| Dashboard | Quick jump actions (Boost/Chat/Audit) | Working | App orchestration + optimization decision path | uses existing routes | `DashboardPage.jsx` | Launches existing flows, not placeholder buttons. |
| PulseCore | Tweak catalog display | Working | `core/optimizer.py` | `GET /api/tweaks` | `PulseCorePage.jsx` | Includes applied state and active snapshot metadata. |
| PulseCore | Apply tweak | Working | `core/optimizer.py`, `core/revert_manager.py` | `POST /api/tweaks/{id}/apply` | `PulseCorePage.jsx` | Audited and rollback-aware. |
| PulseCore | Revert tweak | Working | optimizer revert path | `POST /api/tweaks/{id}/revert` | `PulseCorePage.jsx` | Supports snapshot-directed revert. |
| PulseCore | Boost Now flow | Working (capability-limited) | optimization decision + short benchmark estimate | `POST /api/optimizations/decision`, benchmark route | `PulseCorePage.jsx` | Gain estimate depends on available metrics. |
| PulseAI | Chat stream | Working (capability-limited) | `api/main.py` websocket + optional Anthropic client | `/ws` message type `chat` | `AIPanel.jsx` | Falls back to local response logic without API key. |
| Audit | Timeline/list view | Working | `core/audit_log.py`, DB audit tables | `GET /api/audit` | `AuditPage.jsx` | Shows before/after payload details. |
| Audit | Revert from audit row | Working (capability-limited) | snapshot-linked revert via optimizer | `POST /api/audit/{id}/revert` | `AuditPage.jsx` | Only rows with revert snapshot can be reverted. |
| Audit | Export audit CSV | Working (capability-limited) | orchestrator memory export | `GET /api/actions/export` | `AuditPage.jsx` | Entitlement-gated (`audit_export`). |
| Benchmark | Run benchmark | Working | `core/benchmark_engine.py` | `POST /api/benchmark/run` | `BenchmarkPage.jsx` | Captures baseline/optimized windows and verdict. |
| Benchmark | View benchmark history | Working (capability-limited) | DB benchmark tables | `GET /api/benchmark/results` | `BenchmarkPage.jsx` | Entitlement-gated (`benchmark_history`). |
| Benchmark | Export benchmark markdown | Working (capability-limited) | route-generated markdown from result | `GET /api/benchmark/results/{id}/export` | `BenchmarkPage.jsx` | Entitlement-gated and id-dependent. |
| Metrics | Live frame-time ingestion | Working (capability-limited) | `core/performance_diagnostics.py` | `GET /api/metrics`, `/api/metrics/live` | Dashboard summary | Requires `PRESENTMON_CSV_PATH` pointing to a live PresentMon-compatible CSV; otherwise explicit unavailable. |
| Benchmark | FPS/1% low/frame-time evidence | Unsupported (runtime/hardware) | explicit unsupported reasons in benchmark engine | returned in benchmark payload | Benchmark verdict/details | Benchmark engine still needs to consume the frame-time ingestion path for persisted FPS evidence. |
| Adaptive Engine | Status and toggle | Working | `core/adaptive_engine.py` | `GET /api/adaptive/status`, `POST /api/adaptive/toggle` | Settings + status areas | Real state persisted and exposed. |
| Network | Diagnostics overview | Working (capability-limited) | `core/network_optimizer.py` | `GET /api/network/diagnostics` | `NetworkPage.jsx` | Router and game-server probes may be unavailable. |
| Network | QoS apply profile | Dry-run | network optimizer apply path | `POST /api/network/qos` | `NetworkPage.jsx` | Returns dry-run success when supported; no real policy writes yet. |
| Network | Advanced controls lock | Working | entitlement access map checks | 403 from qos route | Network QoS tab | Proper lock behavior for free plan. |
| GPU | Telemetry stats | Working (capability-limited) | `core/gpu_controller.py` | `GET /api/gpu/stats` | `GpuPage.jsx` | Depends on vendor runtime (`nvidia-smi`, etc.). |
| GPU | Apply GPU setting | Dry-run | gpu controller apply path | `POST /api/gpu/settings` | `GpuPage.jsx` | Writes remain dry-run by design. |
| GPU | BIOS checklist | Working | advisory-only generator | `GET /api/bios/checklist` | `GpuPage.jsx` | Explicitly advisory-only, no firmware writes. |
| Game Discovery | Steam detection | Working | `core/steam_library.py`, game library scanner | `GET /api/games` | `ProfilesPage.jsx` | Includes app IDs and install metadata where found. |
| Game Discovery | Epic/GOG/Xbox/manual discovery | Working (capability-limited) | `core/game_library.py` | `GET /api/games` | `ProfilesPage.jsx` | Best-effort registry/path heuristics. |
| Game Detection | Running game mapping | Working (capability-limited) | `core/game_detection.py` | reflected in orchestrator/session state | Dashboard/PulseCore/Profiles | Precision depends on executable visibility and launcher behavior. |
| Profiles | Profile CRUD | Working | `core/game_profile_service.py` | `GET/POST /api/games/{id}/profile` | `ProfilesPage.jsx` | Saved in SQLite with recommendation basis/history fields. |
| Profiles | Profile import/export | Working | game profile service JSON handling | `GET /api/games/{id}/export`, `POST /api/games/import` | `ProfilesPage.jsx` | Uses `.pbprofile` JSON payloads. |
| Profiles | Community profiles | Placeholder / Future website-dependent | UI and entitlement hooks only | none dedicated | `ProfilesPage.jsx` community tab | Explicitly shown as unavailable in desktop build. |
| Trust Center | Rollback readiness and matrix | Working | `core/trust_center.py` | `GET /api/trust-center/status` | `TrustPage.jsx` | Uses real capability and audit-derived rows. |
| Trust Center | Rollback all active temporary tweaks | Working | optimizer restore path | `POST /api/trust-center/rollback-all` | `TrustPage.jsx` | Audited and counted results returned. |
| Trust Center | Undo category | Working (capability-limited) | undo path with category whitelist | `POST /api/trust-center/undo/{category}` | `TrustPage.jsx` | Only `registry` and `network` supported categories. |
| Settings | Preference persistence | Working | `DatabaseService` settings rows | `POST /api/settings/preferences` | `SettingsPage.jsx` | Merged with defaults and expert-mode state. |
| Settings | Expert mode toggle | Working | settings + audit event | `POST /api/settings/expert-mode` | Settings + Trust | Gate used for high-risk surfaces. |
| Settings | Data actions (export/import/reset/clear/report) | Working | settings data-action route + report generator | `POST /api/settings/data-action` | `SettingsPage.jsx`, `AccountPage.jsx` | Performs real DB operations and audits. |
| Account/Auth | Auth status and account reads | Working | `core/auth_service.py` | `GET /api/auth/status`, `/api/account/*` | `AccountPage.jsx` | Local cached auth posture is functional. |
| Account/Auth | Local placeholder sign-in | Working (dev-gated) | `AuthService.sign_in_local_placeholder` | `POST /api/auth/local-session` | `AccountPage.jsx` | Disabled unless `AUTH_DEV_MODE=true`. |
| Account/Auth | Token exchange | Placeholder | token exchange placeholder method | `POST /api/auth/token-exchange` | Account-linked flows | Explicit placeholder for future website authority. |
| Licensing | Entitlement feature gating | Working | `core/entitlements.py` + route checks | multiple routes (403 guards) | Account + locked controls | Backend and UI are both entitlement-aware. |
| Licensing | Website-managed subscription flow | Future website-dependent | link fields and authority metadata only | surfaced in auth status links | Account buttons | Requires configured website URLs and backend authority. |
| Desktop Runtime | Electron shell | Working | `electron/main.cjs`, preload bridge | local IPC handlers | full app shell | Preferred runtime when installed. |
| Desktop Runtime | PyWebView shell fallback | Working | `desktop_app.py` | bridge API parity | full app shell | Used if Electron not available. |
| Packaging | Installer/uninstaller | Placeholder | docs only (`INSTALLER_ARCHITECTURE` guidance) | none | none | Not implemented in repository. |
| QA Automation | Backend/unit tests | Working | `pulseboost/tests/*.py` | n/a | n/a | Broad module and API coverage exists. |
| QA Automation | Browser/E2E UI automation | Placeholder | documented gap | n/a | n/a | Frontend validation currently build-based. |

## Summary by Status Class

### Working
- Core dashboard/state stack
- Tweak apply/revert lifecycle
- Audit and rollback primitives
- Trust center visibility and rollback operations
- Game profile CRUD/import/export
- Settings persistence and data actions
- Desktop runtime launch paths

### Working (capability-limited)
- Benchmark history/details (entitlement and metric-source limits)
- Network diagnostics depth
- GPU telemetry availability
- Game detection precision in launcher edge-cases

### Dry-run
- Network QoS writes
- GPU settings writes
- Executor actions when global dry-run is enabled

### Placeholder
- Token exchange to website authority
- Installer/uninstaller packaging workflow
- Community profile cloud flow in UI

### Future website-dependent
- Production sign-in/subscription authority
- Cloud profile sync
- Multi-device licensing enforcement beyond local cache

### Unsupported (runtime/hardware)
- FPS/1% low/frame-time benchmark metrics without a configured trusted frame-time source
- Vendor telemetry/writer paths when required runtime tools are absent
