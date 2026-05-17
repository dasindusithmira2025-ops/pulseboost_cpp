# PulseBoost Full A-to-Z Review

Date: 2026-04-14  
Reviewer: Codex (repo audit + runtime verification)

## 1) Scope and Method

This review covered first-party runtime code, frontend code, desktop shells, docs, and tests:

- Backend/API: `pulseboost/api`, `pulseboost/core`, `pulseboost/config.py`, `serve_app.py`
- Desktop shell: `pulseboost/desktop_app.py`, `pulseboost/electron/*`, `start_desktop.ps1`, `pulseboost/ui/scripts/launch-desktop.cjs`
- Frontend: `pulseboost/ui/src/*`
- Tests: `pulseboost/tests/*`
- Product docs: `docs/PULSEBOOST_SPEC.md`, `PLANS.md`, `implement.md`, `PROGRESS.md`, plus architecture/limitations docs

Verification commands run:

- `tools\python\python.exe -m unittest discover -s tests` (from `pulseboost`) -> PASS (`66` tests), with repeated `anyio` `ResourceWarning` notices for unclosed memory streams
- `tools\python\python.exe -m compileall api core desktop_app.py` -> PASS
- `npm run build` (from `pulseboost/ui`) -> PASS

Codebase size reviewed (first-party app/test/UI scope):

- Python files: `78`
- JS/JSX/CJS files: `29`
- CSS files: `1`
- Approx code lines scanned in scoped areas:
  - Python: `12615`
  - UI/Electron JS/JSX/CJS: `5187`

## 2) Top Findings (Prioritized)

## Critical

1. Local auth input errors can crash route handling as server errors
- Evidence:
  - `pulseboost/api/routes.py:194` (`/auth/local-session`) directly calls service without converting `ValueError`
  - `pulseboost/core/auth_service.py:200` raises `ValueError` for invalid email
- Impact:
  - Invalid user input can surface as 500/internal exception instead of a clean 4xx client error.
  - Reproduced with invalid email payload to `/api/auth/local-session`.
- Recommendation:
  - Catch `ValueError` in route and return `HTTPException(status_code=400, detail=...)`.
  - Add route-level test for invalid-email behavior.

## High

2. Chat path does blocking work inside async request flow
- Evidence:
  - `pulseboost/api/main.py:297` reads prompt file synchronously on each chat call
  - `pulseboost/api/main.py:307` calls synchronous `client.messages.create(...)` in async function
- Impact:
  - Can block the event loop under load, degrading websocket/API responsiveness.
- Recommendation:
  - Cache prompt text at startup.
  - Move model call to thread executor or async client path.
  - Add timeout/circuit-breaker behavior for provider stalls.

3. Duplicate game detection implementations are already drifting
- Evidence:
  - `pulseboost/core/game_detector.py` and `pulseboost/core/game_detection_hooks.py` implement near-identical detection logic with different constants and return payload shapes.
  - Example drift: launcher/game hint sets differ (`game_detector` includes `cs2`, `apex`, `riotclientservices.exe`; hooks version does not).
- Impact:
  - Inconsistent active-game detection across subsystems and future maintenance risk.
- Recommendation:
  - Consolidate to one detector service and one shared hint/launcher config.

## Medium

4. Desktop launcher log file handles are opened but not explicitly closed
- Evidence:
  - PyWebView path: `pulseboost/desktop_app.py:53-56`
  - Electron path: `pulseboost/electron/main.cjs:80-83`
- Impact:
  - Potential descriptor leak over repeated restart cycles and hard-to-debug file lock behavior on Windows.
- Recommendation:
  - Keep handles and close them on backend stop/exit, or route logs via inherited stdio + process-level logger.

5. Broad `except Exception` hides important transport failures
- Evidence:
  - `pulseboost/api/main.py:62` in websocket broadcast drops errors silently.
- Impact:
  - Connection failures are absorbed without diagnostics; weakens observability for real runtime issues.
- Recommendation:
  - Log exception context (socket/client id, operation type) before disconnecting.

6. Security defaults are still dev-leaning for a shipped desktop product
- Evidence:
  - `pulseboost/config.py:25` -> `AUTH_DEV_MODE` default `True`
  - `pulseboost/api/main.py:234` -> CORS falls back to `["*"]` if origin list empty
- Impact:
  - Safer for development, but risky if packaged defaults are not environment-hardened.
- Recommendation:
  - Production profile should force explicit CORS origins and disable local auth-dev mode by default.

## Low

7. Non-product artifact files exist in Electron folder
- Evidence:
  - `pulseboost/electron/sample.txt`, `pulseboost/electron/sample2.txt`, `pulseboost/electron/foo.txt`
- Impact:
  - Noise in production tree and release hygiene concerns.
- Recommendation:
  - Remove or relocate non-runtime artifacts.

## 3) A-to-Z Subsystem Review

## A) API (`pulseboost/api/*`)
- Coverage:
  - Route surface is broad and coherent (`/status`, `/state`, `/tweaks`, `/settings`, `/auth`, `/account`, `/audit`, `/adaptive`, `/network`, `/gpu`, `/games`, `/trust-center`, `/benchmark`).
- Strengths:
  - Good domain grouping and explicit 4xx usage in many flows.
  - Entitlement gating is represented in route behavior.
- Gaps:
  - Local auth invalid input error mapping bug (critical finding #1).

## B) Benchmark Engine (`core/benchmark_engine.py`)
- Strengths:
  - Honest unsupported-metric reasoning and persisted verdicts.
  - Integrates optimizer + collector + network/gpu context.
- Gaps:
  - FPS-grade evidence still limited by known runtime boundaries (already documented in repo docs).

## C) Capabilities and Compatibility (`core/capabilities.py`, `core/compatibility.py`)
- Strengths:
  - Capability snapshots persisted and consumed for safe gating.
  - Hardware/runtime awareness is integrated into decision paths.
- Gaps:
  - Defensive exception handling in capability probing is pragmatic but could benefit from richer diagnostics in logs.

## D) Data Layer and Migrations (`core/db/service.py`, `core/db/migrations.py`)
- Strengths:
  - Strong schema breadth: audit, revert snapshots, sessions, benchmarks, adaptive/network/gpu/profile/trust/auth tables.
  - Migration model and compatibility shims are in place.
- Gaps:
  - High connection churn (`aiosqlite.connect(...)` in many methods) can increase lock sensitivity on Windows.

## E) Eventing and Auditability (`core/event_bus.py`, `core/audit_log.py`)
- Strengths:
  - Append-only audit model is consistent.
  - Before/after/rationale metadata is widely used.
- Gaps:
  - Event-stream observability could be improved with stronger structured error logging in transport paths.

## F) Frontend Shell (`ui/src/App.jsx`, pages/components)
- Strengths:
  - Clear multi-page product shell, status badges, trust and profile surfaces, and action confirmations.
  - Good mapping to backend domains.
- Gaps:
  - No browser automation tests yet; validation is build-centric.

## G) Game Detection and Profiles (`core/game_detector.py`, `core/game_detection_hooks.py`, `core/game_profile_service.py`)
- Strengths:
  - Profile persistence/export/import is implemented and test-covered.
- Gaps:
  - Duplicate detector logic (high finding #3) risks inconsistent behavior.

## H) Hardening and Safety (`core/safety_guard.py`, `core/process_priority_manager.py`, `core/service_wrapper.py`, `core/registry_wrapper.py`)
- Strengths:
  - Protected-process rules and safer wrappers are present.
  - Dry-run and unsupported states are generally honest.
- Gaps:
  - Continue expanding explicit blocked-state telemetry in UI for every privileged operation.

## I) IPC and Desktop Runtime (`desktop_app.py`, `electron/main.cjs`, `electron/preload.cjs`)
- Strengths:
  - Electron preferred + PyWebView fallback is implemented.
  - Preload bridge is scoped and context-isolated.
- Gaps:
  - Log-handle lifecycle issue (medium finding #4).

## J) Job Loop / Orchestration (`core/agents/orchestrator.py`, `core/agents/executor.py`)
- Strengths:
  - Centralized cycle architecture with state publication, suggestions, adaptive hooks.
  - Good integration with memory and session manager.
- Gaps:
  - Some broad catch blocks reduce diagnosability during execution failures.

## K) Knowledge/Memory System (`core/cognition/memory.py`)
- Strengths:
  - Rich persistence of snapshots/actions/optimizations/usage/retention.
  - Useful timeline and learning utilities.
- Gaps:
  - Very large class surface; consider modular split for maintainability.

## L) Logging and Diagnostics
- Strengths:
  - Audit trail is strong.
- Gaps:
  - Runtime exception logging in certain websocket/bridge paths is too sparse.

## M) Metrics (`core/metrics.py`, collector path)
- Strengths:
  - Live metrics + state streaming integrated.
- Gaps:
  - Add tighter transport-level cleanup to reduce stream warning noise in tests.

## N) Network Optimizer (`core/network_optimizer.py`)
- Strengths:
  - Transparent diagnostics and bounded probe logic.
  - QoS remains honest dry-run in unsupported contexts.
- Gaps:
  - Production-safe QoS writer remains roadmap (documented limitation).

## O) Optimizer Core (`core/optimizer.py`, wrappers, revert/temp tweaks)
- Strengths:
  - Catalog, apply, revert, and temporary restoration loop are present and test-backed.
  - Process targeting includes validation paths.
- Gaps:
  - Additional test cases for race/error edges are still valuable under real Windows process churn.

## P) Plans, Entitlements, and Account (`core/entitlements.py`, `core/auth_service.py`, `core/cognition/plans.py`)
- Strengths:
  - Plan normalization and feature gating are coherent.
  - Local secure token persistence architecture exists.
- Gaps:
  - Placeholder auth mode still front-and-center; production authority integration remains pending.

## Q) Quality and Tests (`pulseboost/tests/*`)
- Strengths:
  - Coverage spans phases and product areas; 66 tests currently pass.
- Gaps:
  - No browser/E2E automation.
  - Repeated `ResourceWarning` from anyio streams during suite run should be cleaned up.
  - Startup-route cleanup should continue to be monitored for file-lock flakiness on Windows.

## R) Recovery and Revert (`core/revert_manager.py`, `core/session_recovery.py`)
- Strengths:
  - Recovery decisioning and temporary tweak restoration are wired into startup/shutdown.
  - Revert metadata flow is audited.
- Gaps:
  - Ensure all new tweak categories always include reversible snapshot semantics.

## S) Security Posture
- Strengths:
  - Safer-by-default behavior in many dangerous surfaces (dry-run/unsupported).
  - Context isolation in Electron preload.
- Gaps:
  - Dev-auth/CORS defaults should be production-hardened (medium finding #6).

## T) Trust Center (`core/trust_center.py`)
- Strengths:
  - Trust center state reflects runtime posture, rollback, and safeguards.
- Gaps:
  - Add explicit per-capability provenance and last-verification timestamps in UI if not already visible.

## U) Unsupported-State Honesty
- Strengths:
  - Strong overall adherence: unsupported features are surfaced as unavailable/dry-run rather than silently applied.
- Gaps:
  - Keep this contract strict in future high-risk writers (GPU/QoS real writers).

## V) Validation and Release Readiness
- Current status:
  - Unit tests pass (with warnings)
  - Frontend production build passes
  - Backend compile checks pass
- Remaining documented ship blockers:
  - Browser automation
  - Production-safe QoS writer
  - Production-safe GPU writer
  - Real frame-hook benchmark integration
  - Installer packaging

## W) Windows Isolation
- Strengths:
  - Windows-specific behavior is largely wrapped behind service/registry/process abstractions.
- Gaps:
  - Continue enforcing wrapper-only policy as more features are added.

## X) UX and Product Polish
- Strengths:
  - Product surfaces feel cohesive and “tool-grade” rather than toy-grade.
  - Good transparency messaging in account/settings/trust contexts.
- Gaps:
  - Continue tightening copy around placeholder auth mode and lock states for non-technical users.

## Y) Yield / Performance Opportunities
- High-value opportunities:
  - Make chat provider path non-blocking.
  - Reduce repeated DB connection churn via pooled or managed connection strategy.
  - Add instrumentation around orchestrator cycle durations and API tail latency.

## Z) Zero-Blocker Decision
- If shipping internal/dev preview: acceptable with known limitations documented.
- If shipping broad public build: fix critical/high items first, then harden security defaults and warning hygiene.

## 4) Feature Coverage Snapshot

Implemented and integrated:

- Foundation platform (DB bootstrap, capabilities, safety, audit, revert, recovery)
- Optimizer core with validated tweak catalog and rollback path
- Live runtime orchestration, session tracking, timeline/history
- Benchmark mode with persisted before/after evidence and explicit unsupported metrics
- Adaptive engine (rule + cooldown + logging)
- Network diagnostics with honest QoS dry-run posture
- GPU telemetry/settings with vendor-aware gating and explicit unsupported reasons
- Game profiles + trust center + settings data actions
- Desktop runtime support: Electron preferred, PyWebView fallback

Partially complete / intentionally limited:

- Website-owned auth/billing integration (placeholder local flow remains)
- Real FPS/frame-hook benchmark telemetry
- Production-safe network QoS writer
- Production-safe GPU setting writer
- Browser automation / desktop E2E
- Installer packaging

## 5) Test Surface by Product Area

- Foundation + DB + recovery: covered
- Optimizer + revert + safety: covered
- Adaptive/network/gpu benchmark phases: covered
- Profiles/trust/account foundations: covered
- UI/backend contract checks: covered
- Missing:
  - Browser-level page interaction tests
  - End-to-end desktop shell automation
  - Negative-path API tests for all validation errors (notably local auth invalid payload)

## 6) Recommended Next Execution Order

1. Fix `/api/auth/local-session` validation mapping to 400 and add tests.
2. Refactor chat path to non-blocking provider call + cached prompt.
3. Consolidate game detection into one shared module.
4. Close desktop log handles explicitly in both Electron and PyWebView launchers.
5. Harden production defaults (`AUTH_DEV_MODE`, CORS fallback profile).
6. Add test cleanup/warning hygiene for anyio stream warnings and monitor startup file-lock edge cases.

## 7) Appendix: Reviewed First-Party File Inventory

### Root and docs
- `docs/PULSEBOOST_SPEC.md`
- `implement.md`
- `PLANS.md`
- `PROGRESS.md`
- `serve_app.py`
- `start_desktop.ps1`

### Backend/API
- `pulseboost/api/__init__.py`
- `pulseboost/api/main.py`
- `pulseboost/api/routes.py`
- `pulseboost/api/schemas.py`
- `pulseboost/config.py`

### Core runtime
- `pulseboost/core/__init__.py`
- `pulseboost/core/adaptive_engine.py`
- `pulseboost/core/affinity_manager.py`
- `pulseboost/core/audit_log.py`
- `pulseboost/core/auth_service.py`
- `pulseboost/core/benchmark_engine.py`
- `pulseboost/core/capabilities.py`
- `pulseboost/core/compatibility.py`
- `pulseboost/core/database.py`
- `pulseboost/core/entitlements.py`
- `pulseboost/core/event_bus.py`
- `pulseboost/core/game_detection_hooks.py`
- `pulseboost/core/game_detector.py`
- `pulseboost/core/game_profile_service.py`
- `pulseboost/core/gpu_controller.py`
- `pulseboost/core/metrics.py`
- `pulseboost/core/models.py`
- `pulseboost/core/network_optimizer.py`
- `pulseboost/core/optimizer.py`
- `pulseboost/core/platform_info.py`
- `pulseboost/core/process_priority_manager.py`
- `pulseboost/core/registry_wrapper.py`
- `pulseboost/core/revert_manager.py`
- `pulseboost/core/safety_guard.py`
- `pulseboost/core/secure_storage.py`
- `pulseboost/core/service_wrapper.py`
- `pulseboost/core/session_manager.py`
- `pulseboost/core/session_recovery.py`
- `pulseboost/core/temporary_tweaks.py`
- `pulseboost/core/trust_center.py`

### Core subpackages
- `pulseboost/core/agents/__init__.py`
- `pulseboost/core/agents/executor.py`
- `pulseboost/core/agents/orchestrator.py`
- `pulseboost/core/cognition/__init__.py`
- `pulseboost/core/cognition/anomaly.py`
- `pulseboost/core/cognition/efficiency_scorer.py`
- `pulseboost/core/cognition/leak_analyzer.py`
- `pulseboost/core/cognition/learning.py`
- `pulseboost/core/cognition/memory.py`
- `pulseboost/core/cognition/planner.py`
- `pulseboost/core/cognition/plans.py`
- `pulseboost/core/cognition/predictor.py`
- `pulseboost/core/cognition/reasoner.py`
- `pulseboost/core/cognition/scheduler.py`
- `pulseboost/core/cognition/scorer.py`
- `pulseboost/core/cognition/thermal.py`
- `pulseboost/core/cognition/waste_hunter.py`
- `pulseboost/core/db/__init__.py`
- `pulseboost/core/db/migrations.py`
- `pulseboost/core/db/service.py`
- `pulseboost/core/tools/__init__.py`
- `pulseboost/core/tools/collector.py`
- `pulseboost/core/tools/session_detector.py`
- `pulseboost/core/tools/system_tools.py`

### Desktop runtime
- `pulseboost/desktop_app.py`
- `pulseboost/electron/main.cjs`
- `pulseboost/electron/preload.cjs`
- `pulseboost/electron/foo.txt`
- `pulseboost/electron/sample.txt`
- `pulseboost/electron/sample2.txt`
- `pulseboost/ui/scripts/launch-desktop.cjs`

### Frontend (`pulseboost/ui/src`)
- `pulseboost/ui/src/App.jsx`
- `pulseboost/ui/src/main.jsx`
- `pulseboost/ui/src/api/client.js`
- `pulseboost/ui/src/hooks/useWebSocket.js`
- `pulseboost/ui/src/store/useSystemStore.js`
- `pulseboost/ui/src/styles/globals.css`
- `pulseboost/ui/src/utils/formatters.js`
- `pulseboost/ui/src/components/AIPanel.jsx`
- `pulseboost/ui/src/components/AlertBanner.jsx`
- `pulseboost/ui/src/components/AuditLog.jsx`
- `pulseboost/ui/src/components/Card.jsx`
- `pulseboost/ui/src/components/ConfirmDialog.jsx`
- `pulseboost/ui/src/components/HealthRing.jsx`
- `pulseboost/ui/src/components/MiniChart.jsx`
- `pulseboost/ui/src/components/OnboardingOverlay.jsx`
- `pulseboost/ui/src/components/PagePrimitives.jsx`
- `pulseboost/ui/src/components/Sidebar.jsx`
- `pulseboost/ui/src/components/StatusBadge.jsx`
- `pulseboost/ui/src/pages/AccountPage.jsx`
- `pulseboost/ui/src/pages/AuditPage.jsx`
- `pulseboost/ui/src/pages/BenchmarkPage.jsx`
- `pulseboost/ui/src/pages/DashboardPage.jsx`
- `pulseboost/ui/src/pages/GpuPage.jsx`
- `pulseboost/ui/src/pages/NetworkPage.jsx`
- `pulseboost/ui/src/pages/OptimizationsPage.jsx`
- `pulseboost/ui/src/pages/ProfilesPage.jsx`
- `pulseboost/ui/src/pages/SettingsPage.jsx`
- `pulseboost/ui/src/pages/TrustPage.jsx`

### Tests
- `pulseboost/tests/test_anomaly.py`
- `pulseboost/tests/test_app_startup_routes.py`
- `pulseboost/tests/test_auth_foundation.py`
- `pulseboost/tests/test_collector.py`
- `pulseboost/tests/test_desktop_shell.py`
- `pulseboost/tests/test_foundation.py`
- `pulseboost/tests/test_memory.py`
- `pulseboost/tests/test_optimization.py`
- `pulseboost/tests/test_optimizer_core.py`
- `pulseboost/tests/test_phase10_hardening.py`
- `pulseboost/tests/test_phase2_database_migrations.py`
- `pulseboost/tests/test_phase3.py`
- `pulseboost/tests/test_phase5_benchmark.py`
- `pulseboost/tests/test_phase6_adaptive.py`
- `pulseboost/tests/test_phase7_network.py`
- `pulseboost/tests/test_phase8_gpu.py`
- `pulseboost/tests/test_phase9_profiles_trust.py`
- `pulseboost/tests/test_plans.py`
- `pulseboost/tests/test_scorer.py`
- `pulseboost/tests/test_ui_backend_wiring.py`

