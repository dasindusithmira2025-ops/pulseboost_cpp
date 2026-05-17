# PulseBoost Product Dossier (Current Codebase)

Generated from direct inspection of the current repository implementation, runtime scripts, API routes, core modules, UI pages, tests, and existing documentation.

Scope constraints respected:
- No redesign
- No feature additions
- No refactor for behavior changes
- Documentation and product intelligence only

## 1. Product Overview

### What PulseBoost is
PulseBoost is a Windows desktop optimization product built as a local-first control plane for gaming and performance sessions. It combines:
- real-time system telemetry,
- safe tweak application and rollback,
- adaptive optimization suggestions,
- benchmark evidence capture,
- trust and audit transparency,
- entitlement-aware feature gating.

The shipped runtime is desktop-first and currently prefers Electron with a PyWebView fallback path.

### Target users
- Performance-focused PC gamers
- Enthusiast and power-user Windows operators
- Creators or developers who need machine observability with reversible tuning
- Early technical buyers evaluating trust and safety posture before broad deployment

### Core value proposition
- Safety-first optimization with explicit rollback and audit metadata
- Honest capability reporting (supported, dry-run, unsupported) instead of fake success states
- Measurable before/after benchmark pipeline with persisted evidence
- Session-aware, hardware-aware, entitlement-aware behavior
- Premium command-center UI over local backend intelligence

### Major differentiators in current build
- Trust Center is not cosmetic: it reflects rollback state, permission audit, unsupported capabilities, and expert-mode gates from backend state.
- Audit log is append-only in behavior and stores before/after rationale for changes and dry-run blocks.
- Temporary tweak lifecycle includes startup/shutdown recovery routines and unclean-exit cleanup attempts.
- Multi-source game discovery exists (Steam, Epic, GOG, Xbox, manual roots), not Steam-only.
- Premium gating is wired through entitlement snapshots and enforced in backend routes, not only in UI.

### Premium/commercial positioning
Current product posture is "technical premium desktop software" with clear upsell surfaces:
- locked premium features visible in Account and feature surfaces,
- entitlement-backed backend route guards,
- website-link hooks for sign-in/subscription management.

### Current maturity level
Maturity is strong for a local desktop foundation and internal/closed-beta demos, but not yet production-commercial complete due to known missing components:
- real website authority integration,
- real GPU/QoS writers (currently dry-run),
- frame-hook metrics for FPS/1% low/frame-time,
- installer/uninstaller implementation,
- browser/E2E automation coverage.

## 2. Feature Inventory (Implemented Surfaces)

Status legend used below:
- Working
- Working (capability-limited)
- Dry-run
- Placeholder
- Unsupported (runtime/hardware-gated)
- Future website-dependent

### Dashboard
- What it does:
  - Presents machine health summary, trust state, quick actions, and optimization recommendations.
- How it works:
  - Reads orchestrator state, trust center status, metrics snapshots, and recommendation feed.
- Backend/core:
  - `core/agents/orchestrator.py`, `core/trust_center.py`, `core/metrics.py`
- API:
  - `GET /api/status`, `GET /api/state`, `GET /api/metrics`, `GET /api/trust-center/status`
- Frontend:
  - `ui/src/pages/DashboardPage.jsx`
- Status:
  - Working (capability-limited where runtime/hardware limits apply)

### Optimizations / PulseCore
- What it does:
  - Shows tweak catalog by category, allows apply/revert, provides one-click "Boost Now", displays tweak timeline and AI panel.
- How it works:
  - Uses optimizer catalog + temporary tweak state, audit rows, suggestions feed, and orchestrator decisions.
- Backend/core:
  - `core/optimizer.py`, `core/temporary_tweaks.py`, `core/revert_manager.py`, `core/safety_guard.py`, `core/agents/orchestrator.py`
- API:
  - `GET /api/tweaks`, `POST /api/tweaks/{id}/apply`, `POST /api/tweaks/{id}/revert`, optimization decision endpoints, actions preview/execute
- Frontend:
  - `ui/src/pages/PulseCorePage.jsx`
- Status:
  - Working for safe/local controls; some actions can be blocked by safety or compatibility gates

### Benchmark / Proof
- What it does:
  - Captures baseline/optimized windows, computes verdict, stores history, exports markdown report.
- How it works:
  - Benchmark engine captures CPU and optional network/GPU data, records unsupported metrics explicitly.
- Backend/core:
  - `core/benchmark_engine.py`, `core/report_generator.py`
- API:
  - `POST /api/benchmark/run`, `GET /api/benchmark/results`, `GET /api/benchmark/results/{id}`, `GET /api/benchmark/results/{id}/export`
- Frontend:
  - `ui/src/pages/BenchmarkPage.jsx`
- Status:
  - Working (capability-limited): FPS/1% low/frame-time not implemented without frame hook

### Adaptive Engine
- What it does:
  - Maintains adaptive on/off state and records adaptive actions.
- How it works:
  - Rule-based adaptive cycle from orchestrator; cooldown and action logging through DB and audit.
- Backend/core:
  - `core/adaptive_engine.py`, orchestrator integration
- API:
  - `GET /api/adaptive/status`, `POST /api/adaptive/toggle`
- Frontend:
  - surfaced in Settings and Status views
- Status:
  - Working (capability-limited by safe-action envelopes)

### Network Diagnostics and QoS
- What it does:
  - Provides NIC and probe diagnostics, protocol profile, and QoS action surface.
- How it works:
  - Diagnostic probes run against public target; router/game server probes can remain unavailable; QoS apply is gated by entitlement and capability.
- Backend/core:
  - `core/network_optimizer.py`
- API:
  - `GET /api/network/diagnostics`, `POST /api/network/qos`
- Frontend:
  - `ui/src/pages/NetworkPage.jsx`
- Status:
  - Diagnostics: Working (capability-limited)
  - QoS apply: Dry-run when supported; unsupported otherwise

### GPU and BIOS Advisory
- What it does:
  - Shows telemetry/settings surface and BIOS advisory checklist.
- How it works:
  - Vendor/runtime detection; telemetry from `nvidia-smi` when available; settings writes are dry-run only.
- Backend/core:
  - `core/gpu_controller.py`
- API:
  - `GET /api/gpu/stats`, `POST /api/gpu/settings`, `GET /api/bios/checklist`
- Frontend:
  - `ui/src/pages/GpuPage.jsx`
- Status:
  - Telemetry: Working (runtime-dependent)
  - Settings apply: Dry-run
  - BIOS checklist: Working advisory-only

### Audit Log and Revert
- What it does:
  - Shows event timeline/list with before/after payloads and supports revert where snapshot link exists.
- How it works:
  - Reads append-only-style audit entries; per-entry revert delegates to optimizer snapshot restore.
- Backend/core:
  - `core/audit_log.py`, `core/revert_manager.py`, optimizer integration
- API:
  - `GET /api/audit`, `POST /api/audit/{id}/revert`, `GET /api/actions/export`
- Frontend:
  - `ui/src/pages/AuditPage.jsx`
- Status:
  - Working
  - Export endpoint is entitlement-gated

### Game Profiles and Discovery
- What it does:
  - Lists discovered games, allows profile create/edit/save/import/export, provides recommendation basis.
- How it works:
  - Aggregates discovered libraries + session history + benchmark history + saved profiles.
- Backend/core:
  - `core/game_library.py`, `core/game_detection.py`, `core/game_profile_service.py`, `core/steam_library.py`
- API:
  - `GET /api/games`, `GET /api/games/profiles`, `GET/POST /api/games/{id}/profile`, `GET /api/games/{id}/export`, `POST /api/games/import`
- Frontend:
  - `ui/src/pages/ProfilesPage.jsx`
- Status:
  - Working (with capability variance by install layout and process visibility)
  - Community/cloud surfaces are still website-dependent

### Trust Center
- What it does:
  - Displays rollback readiness, protected-process posture, capability touch matrix, permission audit, expert mode.
- How it works:
  - Reads trust snapshot + audit-derived permission rows + active temporary tweak counts.
- Backend/core:
  - `core/trust_center.py`, `core/safety_guard.py`, optimizer integration
- API:
  - `GET /api/trust-center/status`, `POST /api/trust-center/rollback-all`, `POST /api/trust-center/undo/{category}`
- Frontend:
  - `ui/src/pages/TrustPage.jsx`
- Status:
  - Working
  - Category undo currently supports `registry` and `network` categories only

### Settings
- What it does:
  - Persists user preferences, expert mode, adaptive toggles, and data actions (export/import/reset/clear benchmark/report export).
- How it works:
  - Uses settings table and audited data-action routes.
- Backend/core:
  - `core/db/service.py`, audit integration, report generator
- API:
  - `GET /api/settings`, `POST /api/settings/expert-mode`, `POST /api/settings/preferences`, `POST /api/settings/data-action`
- Frontend:
  - `ui/src/pages/SettingsPage.jsx`
- Status:
  - Working

### Account / Auth / Licensing
- What it does:
  - Exposes local auth/session/plan/entitlement/activation state; plan feature matrix; website links where configured.
- How it works:
  - Auth service writes encrypted token blobs and cached entitlement snapshots.
- Backend/core:
  - `core/auth_service.py`, `core/entitlements.py`, `core/secure_storage.py`
- API:
  - `/api/auth/*`, `/api/account/*`
- Frontend:
  - `ui/src/pages/AccountPage.jsx`
- Status:
  - Local foundation: Working
  - Website authority integration: Placeholder/Future website-dependent

### Notifications / Dialogs / Onboarding
- What it does:
  - First-run onboarding overlay, shortcut-based UI events, explicit confirm prompts before risky actions.
- How it works:
  - LocalStorage onboarding flag + desktop shortcut events + `window.confirm` action gates.
- Backend/core:
  - game-close watchdog event via websocket
- Frontend:
  - `OnboardingOverlay`, page-level confirm calls, toast message on game close
- Status:
  - Working (UX-level hardening opportunity remains: replace native confirms with app dialog system)

## 3. Technical Architecture Summary

### Desktop runtime architecture
- Entry scripts:
  - `start_app_and_wait.ps1` (builds UI, launches desktop shell, waits for backend + window)
  - `start_desktop.ps1` and `run_app.ps1` proxy to `start_app_and_wait.ps1`
- Preferred shell:
  - Electron (`pulseboost/electron/main.cjs`, preload bridge in `pulseboost/electron/preload.cjs`)
- Fallback shell:
  - PyWebView (`pulseboost/desktop_app.py`)
- Backend service:
  - FastAPI app (`serve_app.py` importing `api.main:app`) served via uvicorn
- Transport:
  - Localhost HTTP for REST
  - Local websocket (`/ws`) for state streaming + AI chat token streaming

### Frontend architecture
- React + Vite + Tailwind + Zustand
- Single shell app with local `activePage` state (no router package)
- API layer in `ui/src/api/client.js`
- State store in `ui/src/store/useSystemStore.js`
- WebSocket hook in `ui/src/hooks/useWebSocket.js`

### Backend architecture
- Lifespan composes services in `api/main.py`:
  - DB service and migrations
  - capability snapshot
  - auth service and entitlement context
  - optimizer, adaptive, benchmark, network, GPU, game profile, trust center services
  - orchestrator for periodic cognition/optimization cycles
- Routes are centralized in `api/routes.py`

### API architecture
- Single `APIRouter(prefix="/api")`
- Mix of REST + SSE (`/api/metrics/live`) + websocket (`/ws`)
- Entitlement checks and HTTP 403/409 status for locked/blocked paths
- Response cache for selected expensive endpoints

### Persistence/database architecture
- Unified SQLite schema with migration ownership in `core/db/migrations.py`
- Runtime access through `core/db/service.py`
- Note: code schema version is `7`, while `docs/PERSISTENCE_ARCHITECTURE.md` still states `6`

### Secure storage boundary
- Sensitive tokens: encrypted via Windows DPAPI (`core/secure_storage.py`)
- SQLite stores encrypted token blobs plus non-sensitive identity/session/plan/entitlement metadata

### Auth/licensing model
- Local-first cached auth posture with entitlement snapshot and feature access map
- Local placeholder sign-in available only when `AUTH_DEV_MODE=true`
- Website token exchange endpoint exists as explicit placeholder

### Build/test toolchain
- Python: `requirements.txt`, unittest suite in `pulseboost/tests`
- Frontend: `npm run build`, `npm run desktop:check`
- Launcher uses bundled Node/Python tool paths in repository

## 4. Database and Persistence (Current)

### Core schema and versioning
- Migration owner: `core/db/migrations.py`
- `SCHEMA_VERSION = 7` in code
- Migration journal: `schema_migrations`
- Metadata keys include migration and runtime state

### Data domains in SQLite
- System/runtime/settings:
  - `app_metadata`, `settings`, `runtime_housekeeping`
- Tweak lifecycle and safety:
  - `tweak_catalog`, `tweak_applications`, `rollback_snapshots`, `audit_entries`
- Session and proof:
  - `sessions`, `session_actions`, `benchmark_runs`, `benchmark_results`
- Adaptive/network/gpu/game/trust:
  - `adaptive_actions`, `network_snapshots`, `gpu_snapshots`, `game_profiles`, `trust_center_state`
- Auth/account/licensing:
  - `account_cache`, `account_identities`, `auth_sessions`, `auth_session_tokens`, `account_plans`, `entitlement_snapshots`, `entitlement_snapshot_history`, `device_activations`
- Cognition telemetry/history:
  - `metrics`, `anomalies`, `actions`, `baselines`, `learning`, `usage_counters`, `optimizations`, `process_intelligence`, `scheduled_tasks`, `efficiency_history`, `health_history`

### What uses DB vs secure storage
- DB:
  - preferences, sessions, audits, snapshots, benchmark records, profile data, trust states, entitlement snapshots, activation metadata
- Secure storage boundary:
  - DPAPI-encrypted token blobs in `auth_session_tokens`

### Migration/legacy posture
- Migrations include legacy projection/import flows for older tables/artifacts.
- Cognition/history tables are migration-managed in the unified DB.

## 5. Capability and Limitation Snapshot

### Fully supported foundations
- Core backend startup/lifespan composition
- SQLite migration and runtime persistence
- Session/audit/revert core flows
- Tweak apply/revert for implemented safe controls
- Trust Center status and rollback pathways
- Game profile CRUD + import/export
- Entitlement-aware route gating

### Supported but capability-limited
- GPU telemetry (vendor/runtime-dependent)
- Network diagnostics (router/game-server probes unavailable in many runtimes)
- Game detection precision (depends on process path visibility and launcher layouts)

### Dry-run by design
- Network QoS write path (`apply_qos_profile`)
- GPU settings write path (`apply_setting`)
- Some executor actions when `EXECUTOR_DRY_RUN=true` (default)

### Placeholders / future-dependent
- Auth token exchange to website authority
- Cloud profile sync and true multi-device licensing authority
- Subscription/billing authority integration

### Unsupported on current runtime/hardware (varies)
- FPS/1% low/frame-time benchmark capture (no frame hook)
- GPU runtime integration when required vendor tools are absent
- Non-Windows or non-admin paths for registry/service/qos operations

## 6. Validation and Quality Status

### Automated coverage present
- 28 Python unittest files under `pulseboost/tests`
- Coverage includes:
  - DB migrations and schema evolution
  - optimizer safety/apply/revert
  - benchmark/adaptive/network/gpu/trust/profile flows
  - auth/licensing foundation
  - game discovery/detection (including Steam + expanded scanners)
  - desktop shell wiring and startup route behavior

### Build checks documented and used
- Python compile checks (`compileall`)
- Python unittest discovery
- Frontend production build
- Desktop shell parse checks (`desktop:check`)

### Not yet covered by automation
- Browser/E2E automation for UI interactions
- End-to-end installer lifecycle tests
- Real hardware write-path validation for QoS/GPU controls (because those paths are intentionally dry-run)

### Current release-readiness level
- Strong technical demo/closed-beta readiness
- Not production-commercial complete yet

## 7. Commercial and Sale Readiness (Executive)

### Demo-ready now
- Premium desktop shell look/flow
- Live telemetry and command deck pages
- Safe tweak apply/revert with audit evidence
- Trust Center transparency and rollback flows
- Benchmark verdict capture with explicit unsupported-metric honesty
- Entitlement lock/unlock posture in UI and backend

### Ready for closed beta (technical users)
- Yes, for local-machine-first users aware of current dry-run/hardware constraints

### Blocking real paid launch
- Missing website authority integration for true auth/licensing/billing
- Missing production-safe QoS and GPU write implementations
- Missing frame-hook benchmark metrics expected by gaming buyers
- Missing installer/uninstaller implementation in repository

### Blocking enterprise credibility
- No policy management backend
- No cloud sync/central admin authority
- No E2E automation suite for UI and packaged app scenarios

## 8. Security and Trust Posture

### Strengths
- Explicit rollback architecture and persisted snapshots
- Audit trail with rationale/before-after metadata
- Protected process safety guardrails
- DPAPI token encryption for local session secrets
- Honest unsupported/dry-run messaging paths

### Buyer concerns still likely
- Local placeholder auth flow exists for dev mode and must stay disabled in production config
- Website authority not yet authoritative in runtime
- Some pages still use native `window.confirm` for critical actions (functional but not premium UX trust pattern)
- Dry-run control surfaces may be misread as full control if not framed clearly during sales demos

## 9. Deployment and Shipping Status

### Current shipping mechanics
- Desktop launch scripts build frontend and launch Electron or fallback PyWebView
- Electron can spawn and own local backend on available port in defined range
- FastAPI serves built frontend and API from same local backend process

### Packaging/installer status
- Installer/uninstaller architecture is documented, but implementation is not present in this repository
- Product is runnable locally and demoable, but distribution packaging is incomplete

## 10. Key Findings for Founder/Buyer Conversations

### Strongest product truths
- This is a real, integrated local product, not a UI mock.
- Safety and transparency infrastructure is substantive and persisted.
- Entitlement gating is already wired through backend decisions.
- Game discovery and profile posture now extends beyond Steam in code.

### Most important caveats to disclose
- Advanced write controls (GPU/QoS) are intentionally dry-run in current build.
- Benchmark proof is real but does not yet include frame-hook FPS evidence.
- Commercial authority integration (website/billing/license enforcement) is incomplete.
- Distribution installer flow is not yet implemented.

## 11. Source-of-Truth Notes and Drift Detected

Code-level findings that differ from some docs:
- `docs/PERSISTENCE_ARCHITECTURE.md` says schema version `6`; code uses `SCHEMA_VERSION = 7`.
- `docs/KNOWN_LIMITATIONS.md` still says game detection is sourced from observed sessions/bench history only; code now includes broader installed-game scanning (Steam/Epic/GOG/Xbox/manual roots).

These should be treated as documentation drift, not runtime behavior.

## 12. Overall Assessment

PulseBoost currently stands as a high-quality, safety-first local desktop optimization platform with credible engineering foundations and strong demo value. It is close to sale-ready for technical pilot users, but not yet ready for broad paid commercial launch until authority integration, real hardware write paths, frame-hook benchmark metrics, and installer/distribution implementation are completed.
