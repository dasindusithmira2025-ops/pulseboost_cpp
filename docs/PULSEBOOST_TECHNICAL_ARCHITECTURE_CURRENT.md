# PulseBoost Technical Architecture (Current)

This document describes the architecture implemented in the current repository state.

## 1. Runtime Topology

PulseBoost runs as a desktop-local stack:
- Desktop shell: Electron (preferred) or PyWebView (fallback)
- Backend API: FastAPI + Uvicorn, localhost-only binding
- Persistence: SQLite + migration engine
- Frontend: React/Vite static build served by backend
- Realtime channel: WebSocket (`/ws`) and SSE (`/api/metrics/live`)

## 2. Startup and Launch Path

### Primary scripts
- `start_app_and_wait.ps1`
- `start_desktop.ps1`
- `run_app.ps1`

### Startup sequence
1. Launcher verifies bundled Node/Python runtimes.
2. Frontend build is produced (`npm run build` under `pulseboost/ui`).
3. Launcher selects desktop runtime:
   - Electron if available
   - else PyWebView fallback
4. Desktop runtime ensures backend is reachable and serving SPA shell.
5. UI opens against `http://127.0.0.1:<port>`.

### Backend port strategy
- Default: `18400`
- Electron fallback probe range: `18400-18420`
- Health endpoint used for readiness: `/healthz`

## 3. Electron Architecture

Files:
- `pulseboost/electron/main.cjs`
- `pulseboost/electron/preload.cjs`

### Main process responsibilities
- Spawn/monitor local backend process when needed
- Probe backend readiness and SPA availability
- Manage window lifecycle and desktop controls
- Register global shortcuts
- Expose IPC handlers:
  - metadata
  - open logs/data folders
  - open external links
  - restart backend
  - window control actions

### Preload bridge
`window.pulseboostDesktop` methods are exposed to renderer with `contextIsolation=true` and `nodeIntegration=false`.

## 4. PyWebView Fallback Architecture

File:
- `pulseboost/desktop_app.py`

Responsibilities:
- Start/stop backend process
- Inject browser bridge API equivalent to desktop methods
- Manage fallback desktop window
- Preserve consistent metadata and runtime behavior contract

## 5. FastAPI Application Composition

Files:
- `pulseboost/api/main.py`
- `pulseboost/api/routes.py`
- `serve_app.py`

### Lifespan composition in `api/main.py`
Service graph created at startup:
- `DatabaseService`
- `EventBus`
- `AuditLog`
- `SessionRecovery`
- `AuthService` (with DPAPI cipher)
- `PlatformInfoService` + `CapabilityService`
- `RevertManager`
- `SafetyGuard`
- `SystemOptimizer`
- `SessionManager`
- `Orchestrator`
- `NetworkOptimizer`
- `GPUController`
- `BenchmarkEngine`
- `AdaptiveEngine`
- `GameProfileService`
- `TrustCenterService`

### Backend behavior highlights
- Startup capability snapshot persisted and audited
- Temporary tweak cleanup attempted on startup recovery and shutdown
- Session recovery lifecycle tracked
- Response caching for selected expensive routes
- WebSocket broadcasts full orchestrator state and event updates

### Frontend hosting
`serve_app.py` mounts built Vite assets from `pulseboost/ui/dist` and serves SPA fallback routes.

## 6. Frontend Architecture

Root files:
- `pulseboost/ui/src/App.jsx`
- `pulseboost/ui/src/main.jsx`
- `pulseboost/ui/src/api/client.js`
- `pulseboost/ui/src/store/useSystemStore.js`
- `pulseboost/ui/src/hooks/useWebSocket.js`

### UI shell model
- Single React shell with `activePage` state
- Sidebar-driven navigation
- Page modules under `ui/src/pages`
- Shared components under `ui/src/components`

### Data flow model
- Poll-based refresh for page datasets by page type
- WebSocket for full state push and chat streaming
- Local state store for health, metrics, auth, feature access, and page data
- API client centralizes HTTP contract and error normalization

## 7. Backend Service Layering

### Core boundaries
- `core/db/*`: migration and SQLite service
- `core/optimizer.py`: tweak apply/revert orchestration
- `core/revert_manager.py`: snapshot lifecycle
- `core/audit_log.py`: event journaling
- `core/session_*`: session lifecycle and recovery
- `core/benchmark_engine.py`: before/after capture and verdict
- `core/adaptive_engine.py`: adaptive action state
- `core/network_optimizer.py`: diagnostics and qos surface
- `core/gpu_controller.py`: telemetry, settings surface, bios advisory
- `core/performance_diagnostics.py`: frame-time ingestion and live bottleneck classification
- `core/game_*`: game detection/library/profile services
- `core/auth_service.py` + `core/entitlements.py`: account and feature access context
- `core/agents/*` + `core/cognition/*`: continuous orchestration and optimization intelligence

### API route design
- All primary routes are centralized in `api/routes.py`
- Thin route handlers delegate to service modules
- Entitlement checks are route-level where required
- HTTP errors used for blocked/unsupported/locked states

## 8. Persistence Architecture

### Migration ownership
- `core/db/migrations.py`
- current code schema version: `7`

### Runtime DB access
- `core/db/service.py` via `DatabaseService`
- optional connection pooling (`DBPool`) with semaphore and busy timeout controls

### Data domains
- runtime/settings metadata
- tweaks and application lifecycle
- audits and rollback snapshots
- sessions and benchmark evidence
- adaptive/network/gpu/game/trust snapshots
- auth/account/entitlement/activation records
- cognition telemetry and historical baselines

### Compatibility posture
- migration layer imports and projects legacy tables where present
- fresh installs write to normalized tables only

## 9. Secure Storage and Secrets Boundary

File:
- `core/secure_storage.py`

### Security model
- Uses Windows DPAPI (`CryptProtectData` / `CryptUnprotectData`) for token encryption/decryption
- `AuthService` persists encrypted token blobs in `auth_session_tokens`
- Non-sensitive account/session/plan/entitlement metadata remains in SQLite

### Sensitive data handling
- Access token: encrypted at rest
- Refresh token: encrypted at rest
- No password storage implementation present

## 10. Auth and Licensing Architecture (Current)

Files:
- `core/auth_service.py`
- `core/entitlements.py`
- `api/routes.py` auth/account routes

### Current model
- Local cached account state and entitlement snapshot
- Feature access map derived from entitlement snapshot
- Plan feature rules for `free/pro/team/enterprise`
- Device activation tracked locally

### Future authority hooks
- `POST /api/auth/token-exchange` is explicit placeholder
- Website links and authority metadata are exposed in auth status payload
- Cloud entitlement authority not yet active

## 11. Capability Gating and Safety Controls

### Capability source
- `core/capabilities.py` with hardware/runtime/admin checks and notes

### Safety source
- `core/safety_guard.py` protected process rules and action checks
- `core/compatibility.py` action compatibility checks
- `core/revert_manager.py` snapshot capture/restore

### Practical outcomes
- Unsupported controls return explicit unsupported responses
- Risky/advanced controls require expert mode or explicit confirmation paths
- Some high-impact controls are intentionally dry-run until production-safe writers are added

## 12. Benchmark and Evidence Pipeline

File:
- `core/benchmark_engine.py`

### Current capture model
- Baseline and optimized windows sampled over fixed duration
- Captures CPU and optionally network/gpu where available
- Reuses `FrameTimeCapture` against a trusted PresentMon-compatible CSV source to persist benchmark FPS, 1% low FPS, average frame-time, p95 frame-time, and frame-time variance evidence per window
- Calculates deltas and verdict (`HELPED`, `REGRESSION`, `NO_MEASURABLE_IMPACT`, `UNSTABLE`)
- Persists both run and result payloads

### Known evidence boundary
- Live frame-time ingestion is supported through a configured PresentMon-compatible CSV source (`PRESENTMON_CSV_PATH`).
- Without that trusted source, frame-time/FPS fields remain explicit unavailable values.
- Benchmark persistence now consumes that same live frame-time ingestion path when configured.
- Without the source, persisted benchmark results keep the frame-time evidence fields unavailable and store the explicit `FrameTimeCapture` reason.

## 12.1 Live Bottleneck Diagnostics

File:
- `core/performance_diagnostics.py`

### Current model
- The orchestrator enriches each live snapshot with foreground app, optional GPU telemetry, optional PresentMon CSV frame-time telemetry, and bottleneck classification.
- API surfaces:
  - `GET /api/metrics`
  - `GET /api/metrics/bottleneck`
  - `GET /api/metrics/live`
- Output labels are constrained to:
  - `CPU_BOUND`
  - `GPU_BOUND`
  - `RAM_BOUND`
  - `DISK_BOUND`
  - `NETWORK_BOUND`
  - `THERMAL_BOUND`
  - `BACKGROUND_PROCESS_BOUND`

## 13. Network and GPU Control Posture

### Network
File: `core/network_optimizer.py`
- Diagnostics are real and persisted
- Public target probe via TCP-connect timing
- Router/game-server probe currently unavailable unless endpoint discovery exists
- QoS apply path returns dry-run or unsupported with audit event

### GPU
File: `core/gpu_controller.py`
- Telemetry attempts live vendor reads (NVIDIA via `nvidia-smi`)
- Vendor and capability-aware settings catalog
- Apply path is dry-run only with audit event
- BIOS checklist is advisory-only (no firmware writes)

## 14. Game Discovery and Detection Pipeline

Files:
- `core/game_library.py`
- `core/game_detection.py`
- `core/game_profile_service.py`
- `core/steam_library.py`

### Discovery sources
- Steam
- Epic manifests
- GOG registry and roots
- Xbox registry and roots
- Manual/common install root scan with executable heuristics

### Detection strategy
- Preferred: path match between running executable and discovered install directory
- Fallback: top process heuristic hints
- Launchers are explicitly filtered

## 15. UI Page to Backend Dependency Mapping (High Level)

- Dashboard: status/state/trust/metrics
- PulseCore: tweaks + actions + optimizations + audit + chat
- Network: network diagnostics + qos
- GPU: gpu stats/settings + bios checklist
- Audit: audit entries + audit export + revert
- Benchmark: benchmark run/results/export
- Profiles: games catalog + profile CRUD/import/export
- Trust: trust status + rollback/undo + expert mode
- Settings: settings/preferences/expert/adaptive/data actions
- Account: auth/account/entitlements/activation + website links

## 16. Toolchain and Operations

### Python dependencies
- `pulseboost/requirements.txt`
- FastAPI, Uvicorn, psutil, aiosqlite, structlog, APScheduler, etc.

### Frontend dependencies
- `pulseboost/ui/package.json`
- React 18, Vite 5, Tailwind 3, Electron 35, Zustand, Recharts

### Standard checks in repo docs
- Python compileall
- unittest discover
- frontend `npm run build`
- desktop runtime checks `npm run desktop:check`

## 17. Deployment and Distribution State

### Current state
- Local desktop app launch is operational via scripts
- Electron runtime is integrated and preferred
- PyWebView fallback remains available

### Missing for production distribution
- Installer/uninstaller implementation is not present in repo
- Packaging pipeline is not fully implemented for end-user distribution

## 18. Architectural Risks and Drift Notes

- Doc drift: schema version note in persistence doc is outdated vs current migration code (`7`).
- Doc drift: known limitations doc still describes game detection as session/history-only despite installed game scanner implementation.
- Feature messaging risk: dry-run control surfaces must remain clearly labeled in sales/demo contexts.

## 19. Current Technical Readiness Summary

PulseBoost has a solid, modular, safety-aware architecture with clear service boundaries and transparent behavior under unsupported conditions. The remaining architectural work before full commercial release is mostly on authority integration, real hardware write paths, distribution packaging, and broader automation coverage, not on foundational runtime integrity.
