# PulseBoost Progress

## Current Status
- Project: PulseBoost
- Execution Mode: Phase-by-phase autonomous implementation
- Current Phase: Completed
- Overall Goal: Finish PulseBoost as a premium, safe, transparent, AAA-feeling Windows optimization suite

---

## Phase Checklist

- [x] Phase 0 - Repo Assessment and Execution Plan
- [x] Phase 1 - Foundation Platform
- [x] Phase 2 - System Optimizer Core
- [x] Phase 3 - Session Engine + Live Metrics + API Base
- [x] Phase 4 - Frontend Foundation
- [x] Phase 5 - Proof Engine / Benchmark Mode
- [x] Phase 6 - Adaptive Engine V1
- [x] Phase 7 - Network Optimizer
- [x] Phase 8 - GPU Controller + BIOS Advisory
- [x] Phase 9 - Game Profiles + Trust Center
- [x] Phase 10 - Polish, Hardening, Installer Readiness

---

## Post-Phase Fixes

### SQLite Lock Contention Reduction During Benchmark Polling
#### Status
- Completed on 2026-05-20

#### Notes
- Reduced desktop runtime SQLite lock contention by configuring every `DatabaseService` connection for:
  - `PRAGMA journal_mode = WAL`
  - `PRAGMA synchronous = NORMAL`
  - `PRAGMA busy_timeout = 15000`
- Reduced unnecessary benchmark-page database traffic during an active benchmark run:
  - the silent polling path now refreshes status only while `benchmark_running` is true
  - benchmark/tweak/audit result polling resumes after the run completes
- Synced the frontend `benchmarkRunning` state from `/api/status` so the UI respects backend run state instead of only local button state.

#### Files changed
- `pulseboost/core/db/service.py`
- `pulseboost/ui/src/App.jsx`
- `pulseboost/tests/test_foundation.py`
- `PROGRESS.md`

#### Validation and checks run
- `pulseboost\tools\python\python.exe -m compileall pulseboost/core/db/service.py pulseboost/tests/test_foundation.py`
- `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_foundation.py`
- `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_phase5_benchmark.py`
- `cd pulseboost\ui && npm run build`
- `cd pulseboost\ui && npm run desktop:check`

#### Risks/tradeoffs
- WAL greatly reduces reader/writer contention, but a second external process holding the same database open incorrectly can still cause lock errors.
- The benchmark page now delays some detail refreshes while a run is active to prioritize write stability over mid-run history/tweak/audit freshness.

### Benchmark Frame-Time Evidence Integration
#### Status
- Completed on 2026-05-20

#### Notes
- Wired `core/benchmark_engine.py` to reuse `FrameTimeCapture` for baseline and optimized benchmark windows instead of keeping FPS/frame-time persistence permanently unsupported.
- Added persisted benchmark payload fields for:
  - `frametime_supported`
  - `frametime_source`
  - `baseline_fps_average` / `optimized_fps_average`
  - `baseline_fps_1_low` / `optimized_fps_1_low`
  - `baseline_average_frame_time_ms` / `optimized_average_frame_time_ms`
  - `baseline_p95_frame_time_ms` / `optimized_p95_frame_time_ms`
  - `baseline_frame_time_variance_ms` / `optimized_frame_time_variance_ms`
  - `fps_delta_percent`
  - `p95_frame_time_delta_percent`
  - `frame_time_reason`
- Kept unsupported states explicit: without `PRESENTMON_CSV_PATH`, benchmark results still return unavailable frame-time fields plus the exact `FrameTimeCapture` reason.
- Strengthened benchmark verdict scoring with frame-time evidence when available while preserving existing CPU/network/GPU evidence behavior.
- Updated the Benchmark page to show average FPS, 1% low FPS, p95 frame-time, and the unavailable reason without redesigning the page.

#### Files changed
- `pulseboost/core/performance_diagnostics.py`
- `pulseboost/core/models.py`
- `pulseboost/core/benchmark_engine.py`
- `pulseboost/api/routes.py`
- `pulseboost/tests/test_phase5_benchmark.py`
- `pulseboost/ui/src/pages/BenchmarkPage.jsx`
- `docs/PULSEBOOST_FEATURE_MATRIX.md`
- `docs/KNOWN_LIMITATIONS.md`
- `docs/PULSEBOOST_TECHNICAL_ARCHITECTURE_CURRENT.md`
- `PROGRESS.md`

#### Validation and checks run
- `pulseboost\tools\python\python.exe -m compileall pulseboost/api pulseboost/core pulseboost/tests`
- `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
- `cd pulseboost\ui && npm run build`
- `cd pulseboost\ui && npm run desktop:check`

#### Risks/tradeoffs
- Benchmark frame-time persistence still depends on an external trusted PresentMon-compatible CSV feed; PulseBoost does not launch or manage PresentMon yet.
- Window-bounded capture assumes the CSV source is being appended during the benchmark run; stale/non-appending files remain explicit unavailable evidence, not guessed values.

### Multi-Launcher Game Discovery Expansion (Epic/GOG/Xbox/Manual)
#### Status
- Completed on 2026-04-17

#### Notes
- Added `core/game_library.py` with a cached `InstalledGameScanner` that broadens installed-game discovery beyond Steam:
  - Steam catalog passthrough (existing scanner integration)
  - Epic Games manifest parsing (`.item` manifests)
  - GOG and Xbox registry-backed install discovery (best-effort, non-failing on unsupported keys)
  - Manual/common install-root scanning with executable heuristics
- Integrated shared installed-game scanner into runtime startup (`api/main.py`) and wired it into:
  - `core/game_detection.py` for path-verified active-game detection with source/app metadata
  - `core/game_profile_service.py` for `/api/games` and generated profile enrichment from non-Steam catalogs
- Preserved backward compatibility:
  - Existing Steam scanner injection still works in both detector and profile service
  - Existing profile fields (`steam_app_id`) are still populated for Steam entries
- Expanded tests:
  - New scanner tests for Epic manifest and manual-root detection
  - Detector test for Epic path match
  - Profile catalog test for Epic-discovered titles

#### Validation and checks run
- `.\pulseboost\tools\python\python.exe -m compileall pulseboost\core\game_library.py pulseboost\core\game_detection.py pulseboost\core\game_profile_service.py pulseboost\api\main.py pulseboost\tests\test_game_library.py pulseboost\tests\test_game_detection.py pulseboost\tests\test_phase9_profiles_trust.py`
- `..\pulseboost\tools\python\python.exe -m unittest tests.test_game_library tests.test_game_detection tests.test_phase9_profiles_trust` (from `pulseboost\`)
- `..\pulseboost\tools\python\python.exe -m unittest tests.test_app_startup_routes` (from `pulseboost\`)

#### Risks/tradeoffs
- Registry-based GOG/Xbox discovery is intentionally best-effort due install-layout variance across launcher versions and Microsoft Store/UWP packaging.
- Manual path scanning is bounded and cached to avoid heavy repeated I/O, but still depends on executable-heuristic quality for naming precision.
- For protected or inaccessible install paths, detection safely falls back to existing process-hint logic instead of reporting false success.

### Hotfix: `/api/games` 500 Regression After Steam Catalog Merge
#### Status
- Completed on 2026-04-17

#### Notes
- Fixed a backend runtime regression in `core/game_profile_service.py` where `list_games()` could raise:
  - `UnboundLocalError: cannot access local variable 'entry' where it is not associated with a value`
- Root cause: session-catalog merge used `entry[...]` updates without assigning the `catalog.setdefault(...)` return value to `entry`.
- Applied fix by capturing the setdefault result and updating the same object safely.
- Restarted desktop runtime to load patched backend module.

#### Validation and checks run
- `pulseboost\tools\python\python.exe -m compileall core\game_profile_service.py`
- `pulseboost\tools\python\python.exe -m unittest tests.test_phase9_profiles_trust tests.test_app_startup_routes`
- Live API verification after restart:
  - `GET http://127.0.0.1:18400/api/games -> 200`
  - response length: `29` entries

### Steam Game Discovery + Detection Accuracy Pass
#### Status
- Completed on 2026-04-17

#### Notes
- Implemented a dedicated Steam library scanner in `core/steam_library.py`:
  - detects Steam install roots from env/registry/default locations
  - parses `libraryfolders.vdf` and `appmanifest_*.acf`
  - returns installed Steam catalog with `app_id`, game name, install directory, and best-effort executable path
  - uses in-memory cache with refresh interval to avoid repeated heavy disk scans
- Integrated Steam-aware detection into `core/game_detection.py`:
  - active game detection now first maps running process executable paths to discovered Steam install directories
  - returns friendly Steam game names (not just process executable names) when matched
  - added `steam_app_id` and `detected_by=steam_library_path` metadata for transparency
- Reduced false positives in heuristic fallback:
  - removed broad token matching that incorrectly treated `Code.exe` as a game (`cod` substring issue)
  - expanded launcher/process exclusions (`steamwebhelper.exe`, overlay executables, etc.)
- Extended process telemetry in `core/tools/collector.py` with best-effort `executable_path` capture to support path-based matching.
- Integrated Steam discovery into game profiles in `core/game_profile_service.py`:
  - `/api/games` and `/api/games/profiles` catalogs now include installed Steam titles even before a local gameplay session is recorded
  - generated profiles include Steam provenance hints (`steam_app_id`, `steam_discovered`)
- Wired shared Steam scanner instance in API startup (`api/main.py`) so session detection and profile discovery use the same catalog source.

#### Validation and checks run
- `pulseboost\tools\python\python.exe -m compileall core\steam_library.py core\game_detection.py core\tools\collector.py core\game_profile_service.py api\main.py tests\test_game_detection.py tests\test_phase9_profiles_trust.py tests\test_steam_library.py`
- `pulseboost\tools\python\python.exe -m unittest tests.test_game_detection tests.test_phase9_profiles_trust tests.test_steam_library`
- `pulseboost\tools\python\python.exe -m unittest tests.test_app_startup_routes`
- Local scanner sanity probe:
  - `steam_games_detected=14` on this machine

#### Risks/tradeoffs
- Steam detection quality depends on readable process executable paths; access-restricted processes can fall back to heuristic matching.
- Manifest discovery is Steam-specific and does not yet include Epic/EA/Battle.net installed-library scanning.
- Best-effort executable inference from install directories may not always identify the exact launch binary for every title.

### AnyIO ResourceWarning Teardown Cleanup (Competition Step 23)
#### Status
- Completed on 2026-04-17

#### Notes
- Eliminated `anyio` `ResourceWarning` noise caused by leaked `starlette.TestClient` stapled streams in this environment.
- Added `tests/test_client_utils.py` with `managed_test_client(app)`:
  - wraps `TestClient` usage
  - guarantees explicit `client.close()`
  - explicitly closes `stream_receive` and `stream_send` via `aclose()`
- Updated all API test modules that used `TestClient` to use the managed helper for deterministic async stream teardown.

#### Validation and checks run
- `tools\python\python.exe -Wd -m unittest tests.test_app_startup_routes`
- `tools\python\python.exe -Wd -m unittest discover -s tests`

#### Risks/tradeoffs
- A remaining third-party warning still appears from `python_multipart` deprecation in `starlette.formparsers`; this is upstream dependency behavior and not a runtime leak.

### Competition Master Prompt Execution Pass (PulseCore + Trust + Shortcuts)
#### Status
- Completed on 2026-04-17

#### Notes
- Implemented a major competition-focused product pass across backend, Electron shell, and React frontend:
  - Backend:
    - Added explicit `DBPool` semaphore-backed connection pooling in `core/db/service.py`.
    - Added API response caching with TTL and invalidation for expensive reads:
      - `GET /api/capabilities` (60s)
      - `GET /api/health/history` (30s)
      - `GET /api/games/profiles` (10s)
    - Added `GET /api/metrics` for lightweight metrics reads.
    - Added benchmark export endpoint: `GET /api/benchmark/results/{benchmark_id}/export`.
    - Added Trust Center category undo endpoint: `POST /api/trust-center/undo/{category}`.
    - Upgraded websocket broadcast error logs to structured format.
    - Added startup prompt loading from file (`prompts/pulse_ai_system_prompt.txt`) with fallback.
    - Added game-close watchdog broadcast path (`type=game_closed`) using consolidated game detection monitor hooks.
  - Electron/desktop:
    - Added global shortcuts:
      - `Ctrl+Shift+B` -> boost now
      - `Ctrl+Shift+A` -> focus PulseAI
      - `Ctrl+Shift+H` -> jump to PulseCore
      - `Escape` -> dismiss active dialog
    - Wired shortcut IPC from `main.cjs` -> `preload.cjs` -> frontend.
  - Frontend:
    - Rebuilt `PulseCorePage.jsx` to competition spec structure:
      - System Intel Bar with live metrics and health ring.
      - Smart Fix panel with category tabs, applied counters, risk pills, toggle cards, inline shimmer loading, inline errors, and Revert All confirmation.
      - Boost panel with staged progress flow and result messaging.
      - What-changed expansion + tweak timeline cards.
      - PulseScore 7-day sparkline history.
      - Smart Suggestions cards with dismiss + Fix This actions.
      - Embedded upgraded PulseAI panel.
    - Added new components:
      - `SuggestionCard.jsx`
      - `SparklineChart.jsx`
    - Upgraded `AIPanel.jsx`:
      - Collapsible hardware context pill
      - Empty-state suggested prompts
      - Chat markdown rendering with safe escaping and code/list support
      - Typing indicator
      - Multiline input (`Shift+Enter` newline, `Enter` send)
      - Clear-chat destructive confirmation
      - Global focus hook (`pulseboost:focus-chat-input`)
    - Sidebar updates:
      - PulseCore-first navigation posture
      - mini health ring at bottom with score and tooltip
      - active-item treatment and game-detect pulse flash state support
    - Dashboard upgrades:
      - spec-positioned ambient orb
      - Quick Actions row (`Boost Now`, `Chat with AI`, `View Audit`)
    - Added 4-step onboarding flow in `OnboardingOverlay.jsx` with:
      - detected hardware
      - first-score scan stage
      - PulseAI example
      - direct handoff to PulseCore
    - Trust Center upgrade (`TrustPage.jsx`) with:
      - capability touch matrix table
      - live permission audit rows
      - per-row Undo button support
    - Benchmark page upgrade (`BenchmarkPage.jsx`) with:
      - before/after comparison bars
      - per-result export action
    - Settings shortcut panel added under inline section.
    - Account page now includes one-click report export trigger.

#### Validation and checks run
- `python -m compileall pulseboost/api/main.py pulseboost/api/routes.py pulseboost/core/trust_center.py pulseboost/core/db/service.py pulseboost/core/report_generator.py`
- `node --check pulseboost/electron/main.cjs`
- `node --check pulseboost/electron/preload.cjs`
- `npm run build` (from `pulseboost/ui`)
- `npm run desktop:check` (from `pulseboost/ui`)
- `tools\python\python.exe -m unittest tests.test_auth_foundation tests.test_health_history_api tests.test_game_detection tests.test_report_generator tests.test_suggestions_engine tests.test_phase9_profiles_trust tests.test_app_startup_routes`
- `tools\python\python.exe -m unittest discover -s tests`

#### Risks/tradeoffs
- Python suite still emits pre-existing `anyio` `ResourceWarning` stream warnings under `TestClient`; all tests pass functionally.
- Markdown rendering in PulseAI is implemented with safe escaping + lightweight parser logic (no extra dependency install), trading parser completeness for reliability/offline compatibility.
- Trust-category undo currently performs safe temporary-tweak rollback for supported categories (`registry`, `network`) rather than granular per-entry replay.

### Monetization Readiness And Desktop Startup Reliability Pass
#### Status
- Completed on 2026-04-17

#### Notes
- Hardened desktop startup signaling so `start_app_and_wait.ps1` no longer false-fails when Electron is alive and backend health is ready but a titled window handle is not immediately detectable.
- Set an explicit Electron window title (`PulseBoost`) in `pulseboost/electron/main.cjs` to make launcher/runtime detection deterministic.
- Upgraded capability reporting so `supports_electron_runtime` reflects actual runtime availability (bundled Electron binaries or installed Electron), not just the currently active runtime identity.
- Switched production-safe defaults to disable local placeholder auth mode by default (`AUTH_DEV_MODE=false`) and set desktop runtime default to Electron (`DESKTOP_RUNTIME=electron`).
- Updated free-tier entitlement posture for monetization clarity:
  - `advanced_gpu_controls`: locked on free
  - `advanced_network_controls`: locked on free
- Updated Account UI to hide local placeholder sign-in controls unless dev mode is explicitly enabled by backend authority posture.
- Added error handling for account actions in the main app shell (`local sign-in`, `refresh entitlements`, `sign out`) so monetization/account failures surface cleanly in UI state.
- Updated `.env.example` with explicit `AUTH_DEV_MODE` and `DESKTOP_RUNTIME` defaults aligned with production posture.

### Full Scan + Optimization Coverage Expansion
#### Status
- Completed on 2026-04-16

#### Notes
- Ran a full backend/frontend validation sweep (`unittest`, Python compile, Vite production build, and desktop shell syntax checks) and confirmed no test failures or build breaks.
- Fixed registry reliability gaps in `SafeRegistry` so first-time or missing registry values no longer fail snapshot/read/restore flows:
  - `read_value` now treats missing path/value as a supported `None` state.
  - `restore` now tolerates missing value deletion when the intended restored state is "absent".
- Expanded the optimizer catalog with additional gaming-focused, revertible registry tweaks aligned with public one-click optimizer categories:
  - `disable_game_dvr`
  - `disable_app_capture`
  - `disable_network_throttling`
  - `disable_cortana_assist`
  - `disable_search_web_results`
  - `disable_cortana_consent`
- Added apply handlers for the new tweak IDs in `SystemOptimizer.apply_tweak` with full audit/revert integration through existing registry snapshot flow.
- Hardened the React "Apply All Safe" flow so one failing tweak no longer aborts the whole batch; it now continues per tweak and reports partial-failure details.
- Added regression coverage for the expanded tweak catalog and for registry missing-value safety behavior.
- Validation and checks run:
  - `tools\python\python.exe -m unittest tests.test_optimizer_core tests.test_registry_wrapper`
  - `tools\python\python.exe -m compileall core\optimizer.py core\registry_wrapper.py tests\test_optimizer_core.py tests\test_registry_wrapper.py`
  - `tools\python\python.exe -m unittest discover -s tests`
  - `npm run build` (from `pulseboost\ui`)
  - `npm run desktop:check` (from `pulseboost\ui`)
- Risks/tradeoffs:
  - The new registry tweaks stay capability-gated behind admin-backed registry support and remain temporary/revertible by design.
  - Public category alignment was implemented without reverse-engineering proprietary competitor internals.
  - Pre-existing `anyio` `ResourceWarning` messages still appear during the Python test suite, but all tests pass.

### Final Release-Readiness Validation + Trust Hardening Pass
#### Status
- Completed on 2026-04-14

#### Notes
- Executed a full live desktop-runtime validation sweep through the real Electron path (`start_app_and_wait.ps1`) with startup, shutdown, and relaunch verification, including backend reuse checks on `127.0.0.1:18400`.
- Ran end-to-end live probes across status, settings, auth/account/licensing, tweaks, adaptive, network, GPU, BIOS advisory, audit, benchmark, profiles, trust-center, and timeline route families with explicit success/empty/invalid/missing-path checks and no observed HTTP 500 responses.
- Verified live SSE/WebSocket transport health (`/api/metrics/live`, `/ws`) and persistence integrity in SQLite, including normalized table usage, retired legacy table absence, and secure-token-at-rest boundaries.
- Fixed signed-out auth posture leakage by sanitizing cached activation metadata in `AuthService.get_auth_status` so sign-out no longer exposes prior account linkage (`account_id`) or machine hash in API payloads.
- Added auth regression assertions for sign-out cleanup and sanitized signed-out activation state in `tests/test_auth_foundation.py`.
- Fixed Account page dead-control behavior by disabling website-auth/account-management buttons when authority URLs are unavailable and adding explicit unavailable-state guidance.
- Revalidated settings data actions in live runtime, including `export_all_data`, `import_settings`, `clear_benchmark_history`, and `reset`-compatible preference roundtrips; benchmark history was deliberately cleared and then repopulated with fresh validation runs to confirm post-clear behavior.
- Validation and live checks run:
  - `tools\python\python.exe -m compileall api core tests desktop_app.py`
  - `tools\python\python.exe -m compileall core\auth_service.py tests\test_auth_foundation.py`
  - `tools\python\python.exe -m unittest discover -s tests`
  - `tools\python\python.exe -m unittest tests.test_app_startup_routes tests.test_auth_foundation tests.test_phase2_database_migrations tests.test_ui_backend_wiring`
  - `npm run build` (from `pulseboost/ui`)
  - `npm run desktop:check` (from `pulseboost/ui`)
  - Live Electron runtime launch/relaunch plus full API probe scripts against `http://127.0.0.1:18400`
- Risks/tradeoffs:
  - Desktop validation is now endpoint-complete and runtime-live, but there is still no click-by-click desktop UI automation suite.
  - Python tests still emit pre-existing `anyio` resource warnings despite passing.
  - Historical backend err-log still contains one prior `forrtl` close-event entry from earlier sessions; no new occurrences were introduced during this pass.

### Final Product Polish + Live Wiring Pass
#### Status
- Completed on 2026-04-14

#### Notes
- Wired the Settings `Data` tab controls to live backend settings data-action endpoints (`export_all_data`, `import_settings`, `clear_benchmark_history`, `reset_all_settings`) without changing architecture or redesigning the page.
- Added import-file handling for settings data in the desktop React shell, including safe payload normalization for exported settings JSON structure and clear UI feedback for success/error states.
- Kept destructive benchmark-history clearing honest and non-implicit during verification: the action is wired and callable, but live validation skipped execution when history was non-empty to preserve existing artifacts.
- Revalidated the required command set and completed one final live desktop/API verification sweep for settings persistence, settings data actions, tweak apply/revert, benchmark flow, trust center, profiles, and account/licensing.
- Validation and live checks run:
  - `npm run build` (from `pulseboost/ui`)
  - `npm run desktop:check` (from `pulseboost/ui`)
  - `tools\python\python.exe -m unittest discover -s tests` (from `pulseboost`)
  - Live desktop launch via `start_app_and_wait.ps1` and end-to-end API verification against `http://127.0.0.1:18400`
- Risks/tradeoffs:
  - Desktop verification is runtime-live and endpoint-complete, but there is still no click-by-click automated UI E2E suite.
  - `clear_benchmark_history` remains intentionally destructive by design; this pass verified wiring and behavior while preserving existing benchmark artifacts when non-empty.
  - The known `anyio` resource warnings still appear during Python tests without functional failures.

### Product Hardening And Capability Repair
#### Status
- Completed on 2026-04-12

#### Notes
- Repaired GPU capability detection so PulseBoost now reads real Windows display adapter metadata, recognizes the local NVIDIA runtime through `nvidia-smi`, and exposes live GPU telemetry on supported NVIDIA machines instead of incorrectly reporting the feature as unavailable.
- Tightened GPU capability reasoning so unsupported generic registry-backed settings now explain that administrator-backed registry access is required, while supported NVIDIA vendor-runtime controls clearly present as dry-run-supported instead of contradicting themselves.
- Hardened Electron backend startup so a healthy backend already bound on `127.0.0.1:18400` is reused instead of spawning a second competing backend process on relaunch.
- Fixed benchmark unsupported-reason text to describe the real current runtime/capture limitation instead of stale `Python + WebView` wording, and verified that fresh benchmark runs now capture GPU utilization when supported on the current machine.
- Stabilized the targeted UI/backend wiring test on Windows by tolerating transient SQLite temp-file cleanup lag after the app lifecycle closes.
- Tests and live checks run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests`
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests -p test_app_startup_routes.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests -p test_auth_foundation.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests -p test_phase5_benchmark.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests -p test_phase8_gpu.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests -p test_ui_backend_wiring.py`
  - `pulseboost\tools\python\python.exe -m compileall api core tests desktop_app.py`
  - `npm run build` (from `pulseboost\ui`)
  - `npm run desktop:check` (from `pulseboost\ui`)
  - Live Electron launch, restart, API exercise, auth round-trip, tweak apply/revert, profile export/import, benchmark run, settings persistence round-trip, and log inspection
- Risks/tradeoffs:
  - NVIDIA live telemetry now works on this machine, but AMD runtime support still depends on a supported AMD tool being present and is still surfaced honestly when absent.
  - GPU writes and QoS writes remain dry-run by design; this pass improved reasoning and detection, not unsafe write enablement.
  - Desktop validation is now much stronger and includes live runtime feature exercise, but there is still no click-by-click desktop automation suite.

### UI Truthfulness And Live Wiring Validation
#### Status
- Completed on 2026-04-12

#### Notes
- Removed remaining fake or misleading desktop UI values from the shipped Network, GPU, Benchmark, Profiles, Audit, Trust Center, Settings, Account, and Optimizations pages so they now render backend payloads, explicit unsupported states, or clearly disabled actions instead of fabricated metrics and dead controls.
- Added persisted settings preferences storage and retrieval through FastAPI plus SQLite-backed `app_state`, then wired the Settings page to save real preference changes through the backend instead of mutating React-only local state.
- Added real rollback entry points for audit-item revert and Trust Center rollback-all flows, and fixed `/api/tweaks` so active temporary tweaks surface as applied/revertable in the UI after backend actions occur.
- Added `pulseboost/tests/test_ui_backend_wiring.py` to cover settings persistence, applied-tweak catalog state, audit-driven revert, Trust Center rollback-all, and regression checks against previously shipped fake literals.
- Validated the live Electron desktop runtime on `http://127.0.0.1:18400`, confirmed the desktop window title `PulseBoost`, exercised a safe settings write/readback through `/api/settings/preferences`, restored the original value, and confirmed the persisted row in `D:\PulseBoost Python Project\data\memory.db` plus matching audit entries.
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests`
  - `pulseboost\tools\python\python.exe -m unittest discover -s tests -p test_ui_backend_wiring.py`
  - `npm run build` (from `pulseboost\ui`)
  - Live desktop smoke via `start_app_and_wait.ps1` plus route checks for `/api/status`, `/api/settings`, `/api/tweaks`, `/api/network/diagnostics`, `/api/gpu/stats`, `/api/trust-center/status`, and `/api/audit`
- Risks/tradeoffs:
  - Validation is now substantially broader and includes the live Electron shell, but there is still no automated click-level desktop E2E suite covering every button press and every page transition.
  - Several capability-gated product areas remain honestly unavailable on this machine and in this runtime, including registry/service-backed tweaks without elevation, live GPU vendor telemetry/runtime control, and OS-backed QoS writes.
  - The Python suite still emits pre-existing `anyio` resource warnings under `TestClient`, but all tests passed and no functional failures were present.

### Spec-Aligned Premium UI Replacement
#### Status
- Completed on 2026-04-10

#### Notes
- Replaced the previous mixed frontend shell with a spec-aligned desktop UI across the app shell, sidebar, title bar, dashboard, optimizations, network, GPU, audit, benchmark, profiles, trust, settings, and account surfaces.
- Rebuilt the shared frontend design system around the requested PulseBoost tokens, typography, card primitives, status badges, mini charts, health ring, and navigation structure instead of layering the new visuals on top of the older interface.
- Kept the rewrite in place on the existing React, Tailwind, Electron, FastAPI, and Zustand foundations so the new UI still binds to live backend-backed telemetry, entitlement, audit, benchmark, GPU, network, profile, and trust-center data rather than introducing placeholder states.
- Validated the rewritten frontend with a clean production Vite build and desktop integration checks covering the launcher script, Electron main/preload files, and Python desktop entrypoint.

### Realtime Shell Realignment
#### Status
- Completed on 2026-04-10

#### Notes
- Rebuilt the visible desktop shell and dashboard surface in place around a new realtime command deck and aligned telemetry presentation instead of extending the previous mixed layout.
- Normalized live numeric rendering with tabular-number styling so auto-updating values hold their columns and card geometry more consistently across the shell.
- Removed the dead search-style top bar treatment, fixed the Dashboard to Trust Center navigation handoff, and added quiet active-page refresh polling for non-WebSocket page data.
- Hardened `start_app_and_wait.ps1` so it waits for the actual desktop window and backend readiness even when the `npm` wrapper process exits early after handing off to Electron.

### Auth Foundation Validation Repair
#### Status
- Completed on 2026-04-09

#### Notes
- Repaired a corrupted `pulseboost/config.py` source file that had an invalid BOM-prefixed `cofrom` token and was preventing clean imports, compile checks, auth tests, and startup-route validation.
- Re-ran the auth/account foundation tests, startup-route smoke test, full Python unit suite, frontend production build, desktop script checks, and compile checks after the repair.
- Confirmed the existing account/auth/licensing foundation remains intact and validation-clean after the config fix.

### Desktop Launcher Hardening
#### Status
- Completed on 2026-04-09

#### Notes
- Corrected the top-level app launch flow so `start_app_and_wait.ps1` and `run_app.ps1` no longer report success for a standalone Uvicorn server on `127.0.0.1:8000`.
- The verified desktop launcher path is `start_desktop.ps1` -> `pulseboost/ui` desktop script -> `pulseboost/electron/main.cjs`, with `pulseboost/desktop_app.py` retained as the explicit WebView fallback only when Electron is unavailable.
- `start_app_and_wait.ps1` now builds the UI, launches the desktop shell, waits for the desktop backend on `127.0.0.1:18400`, and requires a real desktop window process titled `PulseBoost` before returning success.
- Fixed missing FastAPI `app.state` wiring for desktop-loaded routes so `/api/status`, `/api/network/diagnostics`, `/api/gpu/stats`, `/api/games`, and `/api/trust-center/status` now initialize correctly in desktop runtime.
- Verified a live Electron window opened with title `PulseBoost` and confirmed the desktop backend routes returned `200` after relaunch.

### Account, Auth, and Licensing Foundation
#### Status
- Completed on 2026-04-09

#### Notes
- Added account/auth/licensing domain models, SQLite persistence, and a Windows DPAPI-backed secure token storage path for local session material.
- Added an auth service that manages local placeholder sign-in, secure session persistence, sign-out cleanup, entitlement refresh, and device activation caching without making the desktop app the licensing authority.
- Added account, auth, entitlement, and activation API routes plus honest placeholder token-exchange structure for future website integration.
- Integrated entitlement-aware account state into the orchestrator and desktop UI so plan posture, signed-in state, activation status, and premium locks are driven by entitlement snapshots instead of hardcoded plan checks.
- Documented the website-as-authority model and local security strategy in `docs/AUTH_LICENSING_ARCHITECTURE.md`.

---

## Phase 0 - Repo Assessment and Execution Plan
### Status
- Completed on 2026-04-07

### Notes
- Assessment recorded in `docs/ARCHITECTURE_MAP.md`
- Current working desktop runtime is Python + WebView, which is a tracked divergence from the Electron target
- Existing orchestrator/memory/API/UI flows are substantial and are being refactored in place instead of replaced

---

## Phase 1 - Foundation Platform
### Status
- Completed on 2026-04-07

### Notes
- Foundation services are integrated into FastAPI startup/shutdown, not left as standalone dead modules
- Existing orchestrator and desktop entrypoints were preserved
- Executor actions now flow through capability checks, safety rules, audit logging, and revert snapshot capture where applicable

---

## Phase 2 - System Optimizer Core
### Status
- Completed on 2026-04-07

### Notes
- Added a real optimizer service with validated tweak catalog, apply/revert logic, and temporary tweak lifecycle tracking
- Added safe wrappers for registry, service start types, process priority, and process affinity with honest unsupported states and dry-run behavior
- Startup recovery now attempts cleanup of temporary tweaks from an unclean previous session

---

## Phase 3 - Session Engine + Live Metrics + API Base
### Status
- Completed on 2026-04-07

### Notes
- Added explicit session manager, game detector, metrics SSE service, and session persistence tables
- Orchestrator now tracks active and recent sessions in live state
- API now exposes `/api/metrics/live`, `/api/status`, `/api/tweaks`, and `/api/audit` with real backend data

---

## Phase 4 - Frontend Foundation
### Status
- Completed on 2026-04-07

### Notes
- Added centralized frontend API client helpers
- Split the UI into page-level Dashboard, Optimizations, and Audit flows without replacing the existing live dashboard components
- Added real backend-backed tweak catalog and foundation audit views plus loading/error/empty-state handling in the app shell

---

## Phase 5 - Proof Engine / Benchmark Mode
### Goal
Create benchmark/evidence system that proves whether tweaks help.

### Status
- Completed on 2026-04-07

### Notes
- Benchmark runs persist with verdicts, tweak sets, unsupported metric evidence, and session ties
- Benchmark Mode is exposed in the shipped shell with history and detail views
- Unsupported frame-hook metrics remain explicit rather than fabricated

---

## Phase 6 - Adaptive Engine V1
### Status
- Completed on 2026-04-07

### Notes
- Local-only adaptive logic is rule-based, cooldown-limited, auditable, and revert-aware
- Adaptive actions surface in the app and in backend persistence

---

## Phase 7 - Network Optimizer
### Status
- Completed on 2026-04-07

### Notes
- Network diagnostics, QoS preview surfaces, and NIC capability reporting are integrated
- Unsupported router/game-server probing and live QoS writes remain explicit runtime limitations

---

## Phase 8 - GPU Controller + BIOS Advisory
### Status
- Completed on 2026-04-07

### Notes
- GPU telemetry and setting surfaces are vendor-aware and capability-gated
- BIOS guidance remains advisory-only and GPU writes remain dry-run until safe writers exist

---

## Phase 9 - Game Profiles + Trust Center
### Status
- Completed on 2026-04-07

### Notes
- Game profiles persist with `.pbprofile` export support and recommendation history
- Trust Center reports real recovery, rollback, safeguard, and unsupported-capability state

---

## Phase 10 - Polish, Hardening, Installer Readiness
### Status
- Completed on 2026-04-07

### Notes
- Startup/shutdown cleanup, expert mode persistence, and release-readiness docs are in place
- Remaining limitations are documented honestly instead of hidden behind placeholder UI

---

## Change Log

#### Phase 0 Summary
- Summary:
  - Read repo rules, product spec, execution plan, runbook, and current progress state.
  - Assessed the existing codebase as a working desktop-oriented guardian/optimization app with a strong orchestrator/UI skeleton but without the planned explicit foundation platform modules.
  - Documented the normalized architecture map, preserved entrypoints, current runtime topology, major gaps, and execution assumptions in `docs/ARCHITECTURE_MAP.md`.
  - Confirmed the main structural divergence: the current desktop runtime is Python + WebView, while the target stack calls for Electron. This remains a tracked gap, not a hidden rewrite trigger.
- Files changed:
  - `docs/ARCHITECTURE_MAP.md`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
- Risks/tradeoffs:
  - The current runtime divergence from Electron must be handled carefully to avoid breaking the only known-good desktop entrypoint.
  - `core/cognition/memory.py` currently acts as both storage layer and repository/service surface, so Phase 1 needs controlled extraction instead of replacement.
  - Existing monitoring/optimization features do not yet satisfy the explicit safety, revert, and trust requirements from the product spec.
- Next phase:
  - Phase 1 - Foundation Platform

#### Phase 1 Summary
- Summary:
  - Added the explicit foundation platform modules required by the plan: typed models, DB bootstrap, event bus, capability detection, platform profiling, compatibility checks, safety guard, revert manager, session recovery, and audit log.
  - Integrated those services into FastAPI startup and shutdown so the app now persists capability snapshots, startup recovery decisions, clean-exit state, foundation audit entries, and revert snapshots.
  - Upgraded the executor to respect capability gating and protected-process safety rules while recording audit entries and revert snapshots for process-affecting actions.
  - Added `/api/status` so the app exposes startup runtime, recovery state, capability state, and hardware profile immediately after boot.
- Files changed:
  - `pulseboost/core/models.py`
  - `pulseboost/core/database.py`
  - `pulseboost/core/event_bus.py`
  - `pulseboost/core/platform_info.py`
  - `pulseboost/core/capabilities.py`
  - `pulseboost/core/compatibility.py`
  - `pulseboost/core/safety_guard.py`
  - `pulseboost/core/audit_log.py`
  - `pulseboost/core/revert_manager.py`
  - `pulseboost/core/session_recovery.py`
  - `pulseboost/core/agents/executor.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/tests/test_foundation.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost\api\main.py pulseboost\api\routes.py pulseboost\core\agents\executor.py pulseboost\core\models.py pulseboost\core\database.py pulseboost\core\event_bus.py pulseboost\core\platform_info.py pulseboost\core\capabilities.py pulseboost\core\compatibility.py pulseboost\core\safety_guard.py pulseboost\core\audit_log.py pulseboost\core\revert_manager.py pulseboost\core\session_recovery.py`
  - FastAPI lifespan smoke check with `TestClient` for `/healthz` and `/api/status` returning `200`
- Risks/tradeoffs:
  - Process termination still cannot be truly reverted; the new revert layer records pre-change state honestly, but full restoration semantics for destructive actions remain a Phase 2 concern.
  - Capability detection is intentionally conservative and currently reports unsupported GPU/runtime control rather than faking integration.
  - The working desktop runtime remains WebView-based; Electron compliance is still a tracked product gap.
- Next phase:
  - Phase 2 - System Optimizer Core

#### Phase 2 Summary
- Summary:
  - Added the optimizer core with validated tweak catalog, safe registry/service/process wrappers, CPU affinity control, and temporary tweak lifecycle persistence.
  - Integrated the optimizer into FastAPI startup so temporary session tweaks can be restored after an unclean prior exit.
  - Exposed the optimizer through `/api/tweaks`, `/api/tweaks/{id}/apply`, and `/api/tweaks/{id}/revert`, while keeping dry-run behavior and honest unsupported states.
  - Extended the safe action catalog so the planner and later phases can reason about service, affinity, and registry-backed tweaks without inventing parallel logic.
- Files changed:
  - `pulseboost/core/registry_wrapper.py`
  - `pulseboost/core/service_wrapper.py`
  - `pulseboost/core/process_priority_manager.py`
  - `pulseboost/core/affinity_manager.py`
  - `pulseboost/core/temporary_tweaks.py`
  - `pulseboost/core/game_detection_hooks.py`
  - `pulseboost/core/optimizer.py`
  - `pulseboost/core/tools/system_tools.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/schemas.py`
  - `pulseboost/tests/test_optimizer_core.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost\api\main.py pulseboost\api\routes.py pulseboost\api\schemas.py pulseboost\core\optimizer.py pulseboost\core\registry_wrapper.py pulseboost\core\service_wrapper.py pulseboost\core\process_priority_manager.py pulseboost\core\affinity_manager.py pulseboost\core\temporary_tweaks.py pulseboost\core\game_detection_hooks.py`
  - FastAPI lifespan smoke check with `TestClient` for `/healthz`, `/api/status`, and `/api/tweaks` returning `200`
- Risks/tradeoffs:
  - Registry and service writes remain dry-run by default and intentionally require capability support plus validated allow-lists; this is safer but means real optimization impact is still policy-gated.
  - Only a narrow validated tweak set is exposed in this phase; more aggressive tweaks are intentionally deferred.
  - The session hook for real game lifecycle handling is still heuristic until Phase 3 builds explicit session management.
- Next phase:
  - Phase 3 - Session Engine + Live Metrics + API Base

#### Phase 3 Summary
- Summary:
  - Added explicit `MetricsService`, `GameDetector`, and `SessionManager` modules and extended the foundation DB with persisted session tables.
  - Integrated session tracking into the orchestrator so active and recent sessions flow through the live app state.
  - Added `/api/metrics/live` as an SSE endpoint, plus `/api/audit` for foundation audit entries, and kept the existing WebSocket path intact.
  - Added focused tests for session lifecycle persistence and SSE payload generation.
- Files changed:
  - `pulseboost/core/database.py`
  - `pulseboost/core/game_detector.py`
  - `pulseboost/core/metrics.py`
  - `pulseboost/core/session_manager.py`
  - `pulseboost/core/agents/orchestrator.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/tests/test_phase3.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost\core\database.py pulseboost\core\game_detector.py pulseboost\core\metrics.py pulseboost\core\session_manager.py pulseboost\core\agents\orchestrator.py pulseboost\api\main.py pulseboost\api\routes.py pulseboost\tests\test_phase3.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_phase3.py`
  - FastAPI lifespan smoke checks for `/healthz`, `/api/status`, `/api/tweaks`, and `/api/audit` returning `200`
- Risks/tradeoffs:
  - The SSE route is implemented and unit-tested at the service layer, but long-lived client streaming smoke checks through `TestClient` are awkward because the stream intentionally never closes.
  - Game session detection is still heuristic and process-name based.
  - The current API surface is broader, but the frontend has not yet been reorganized into page-level flows that fully exploit it.
- Next phase:
  - Phase 4 - Frontend Foundation

#### Phase 4 Summary
- Summary:
  - Added a centralized frontend API client and extended the Zustand store with page state plus backend-backed tweak/audit data.
  - Reworked the React shell into real Dashboard, Optimizations, and Audit views while reusing the existing live dashboard components.
  - Wired the new pages to the backend status, tweak catalog, tweak apply/revert, timeline, and audit routes with explicit empty/error handling.
  - Preserved the working desktop entry path and existing WebSocket-driven dashboard updates.
- Files changed:
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/styles/globals.css`
  - `PROGRESS.md`
- Tests run:
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost\api\main.py pulseboost\api\routes.py pulseboost\core\agents\orchestrator.py`
- Risks/tradeoffs:
  - Frontend behavior is build-validated and backend-backed, but not browser-automation-tested in this pass.
  - The JS bundle remains large and still triggers Vite's chunk-size warning.
  - The UI now has page structure, but Benchmark, Network, GPU, Game Profiles, Trust Center, and Settings pages still remain for later phases.
- Next phase:
  - Phase 5 - Proof Engine / Benchmark Mode

#### Phase 5 Summary
- Summary:
  - Added a real benchmark/proof engine that captures before/after windows, persists benchmark results, computes `HELPED`, `NO_MEASURABLE_IMPACT`, `REGRESSION`, and `UNSTABLE`, and records benchmark audit entries.
  - Extended the foundation database and typed models to store benchmark payloads, tweak sets, session ties, and unsupported metric evidence without faking FPS/GPU/network captures.
  - Added `POST /api/benchmark/run`, `GET /api/benchmark/results`, and `GET /api/benchmark/results/{id}` plus benchmark-running visibility in status.
  - Added a real Benchmark page in the existing React shell with workload input, tweak-set selection, benchmark history, and detail rendering for deltas and unsupported metrics.
- Files changed:
  - `pulseboost/core/models.py`
  - `pulseboost/core/database.py`
  - `pulseboost/core/benchmark_engine.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/schemas.py`
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/styles/globals.css`
  - `pulseboost/tests/test_phase5_benchmark.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_phase5_benchmark.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost/api/main.py pulseboost/api/routes.py pulseboost/api/schemas.py pulseboost/core/models.py pulseboost/core/database.py pulseboost/core/benchmark_engine.py`
- Risks/tradeoffs:
  - FPS, 1% low, frame-time variance, GPU utilization, ping, and jitter remain honest unsupported states until later phases add those capability-specific surfaces.
  - Current benchmark proof is CPU-centric on this runtime, which is still useful but not yet equivalent to a frame-hook-backed game benchmark.
  - The frontend bundle warning remains and will be addressed in later polish/hardening work.
- Next phase:
  - Phase 6 - Adaptive Engine V1

#### Phase 6 Summary
- Summary:
  - Added a local-only rule-based adaptive engine with conservative CPU/disk relief rules, cooldown persistence, toggle state, and anti-spam behavior.
  - Wired the adaptive engine into the orchestrator cycle so it uses the existing safe optimizer surfaces, records adaptive actions in SQLite, and attaches adaptive session actions where a game session is active.
  - Extended `/api/status` plus new `/api/adaptive/status` and `/api/adaptive/toggle` routes so the frontend can show real state and control the engine explicitly.
  - Added adaptive UI visibility in the existing shell with live status, a toggle, and recent adaptive actions on the Optimizations page.
- Files changed:
  - `pulseboost/core/adaptive_engine.py`
  - `pulseboost/core/database.py`
  - `pulseboost/core/agents/orchestrator.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/schemas.py`
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/tests/test_phase6_adaptive.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_phase6_adaptive.py`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost/core/adaptive_engine.py pulseboost/core/database.py pulseboost/core/agents/orchestrator.py pulseboost/api/main.py pulseboost/api/routes.py pulseboost/api/schemas.py pulseboost/tests/test_phase6_adaptive.py`
- Risks/tradeoffs:
  - The first adaptive rule set is intentionally conservative and limited to existing validated tweaks; broader behavior will arrive through later product areas rather than risky one-off actions.
  - Adaptive action state is live and persisted, but there is not yet a dedicated notifications/history page for the engine beyond the Optimizations surface and alerts.
  - Frontend bundle size warning remains unchanged.
- Next phase:
  - Phase 7 - Network Optimizer

#### Phase 7 Summary
- Summary:
  - Added a capability-aware network optimizer with NIC abstraction, session-based TCP/UDP protocol profiling, persisted diagnostics, and honest target probing behavior.
  - Added `/api/network/diagnostics` and `/api/network/qos` plus status integration so the frontend can show real diagnostics and the safe QoS support surface.
  - Added a real Network page with latency/jitter/packet-loss summary, bufferbloat grading, protocol reasoning, NIC capability cards, and QoS controls that remain honest dry-runs.
  - Wired benchmark capture to use the network optimizer when public target probing succeeds so ping/jitter deltas can be measured instead of remaining permanently unsupported.
- Files changed:
  - `pulseboost/core/network_optimizer.py`
  - `pulseboost/core/database.py`
  - `pulseboost/core/benchmark_engine.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/schemas.py`
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/tests/test_phase7_network.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_phase7_network.py`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost/core/network_optimizer.py pulseboost/core/database.py pulseboost/core/benchmark_engine.py pulseboost/api/main.py pulseboost/api/routes.py pulseboost/api/schemas.py pulseboost/tests/test_phase7_network.py`
- Risks/tradeoffs:
  - Router and game-server probes remain honest unsupported states until a safe local gateway/server discovery path exists.
  - QoS is intentionally dry-run only; the support surface is real, but OS policy writes remain gated until a production-safe writer is implemented.
  - Public latency/jitter is based on TCP connect timing, not raw ICMP.
- Next phase:
  - Phase 8 - GPU Controller + BIOS Advisory

#### Phase 8 Summary
- Summary:
  - Added a vendor-aware GPU controller that separates supported telemetry, dry-run-safe setting paths, and unsupported vendor/runtime cases clearly.
  - Added `/api/gpu/stats`, `/api/gpu/settings`, and `/api/bios/checklist` plus a real GPU page in the existing frontend shell.
  - Added BIOS advisory/checklist guidance for Resizable BAR, Above 4G Decoding, BIOS currency, and XMP/EXPO without pretending to read unavailable firmware state.
  - Wired benchmark capture to use the GPU controller when supported telemetry becomes available so GPU deltas can be measured honestly.
- Files changed:
  - `pulseboost/core/gpu_controller.py`
  - `pulseboost/core/benchmark_engine.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/schemas.py`
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/tests/test_phase8_gpu.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_phase8_gpu.py`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost/core/gpu_controller.py pulseboost/core/benchmark_engine.py pulseboost/api/main.py pulseboost/api/routes.py pulseboost/api/schemas.py pulseboost/tests/test_phase8_gpu.py`
- Risks/tradeoffs:
  - Live GPU telemetry remains unsupported on the current capability snapshot unless a supported vendor runtime is actually present.
  - GPU setting writes are dry-run only; the control surface is real and auditable, but production-safe writes remain gated.
  - BIOS guidance is intentionally advisory-only rather than pretending to introspect firmware settings that are not safely available.
- Next phase:
  - Phase 9 - Game Profiles + Trust Center

#### Phase 9 Summary
- Summary:
  - Added persisted game profiles with safe recommendation/history structure, profile save/load, and `.pbprofile` export support.
  - Added a Trust Center service and page that expose real admin, recovery, rollback, protected-process, unsupported-capability, and safeguard state from the foundation layer.
  - Added `/api/games`, `/api/games/{id}/profile`, `/api/games/{id}/export`, `/api/games/import`, and `/api/trust-center/status` routes.
  - Integrated the new pages into the existing React shell without replacing the current launcher/runtime path.
- Files changed:
  - `pulseboost/core/database.py`
  - `pulseboost/core/game_profile_service.py`
  - `pulseboost/core/trust_center.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/schemas.py`
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/tests/test_phase9_profiles_trust.py`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests -p test_phase9_profiles_trust.py`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost/core/game_profile_service.py pulseboost/core/trust_center.py pulseboost/core/database.py pulseboost/api/main.py pulseboost/api/routes.py pulseboost/api/schemas.py pulseboost/tests/test_phase9_profiles_trust.py`
- Risks/tradeoffs:
  - Game detection still depends on session and benchmark history rather than a full installed-games scanner.
  - Profile import is intentionally conservative JSON ingestion rather than a broad untrusted file execution path.
  - Trust Center currently reports expert mode from settings state, but there is not yet a dedicated Settings page to manage it.
- Next phase:
  - Phase 10 - Polish, Hardening, Installer Readiness

#### Phase 10 Summary
- Summary:
  - Hardened startup/shutdown by adding clean-shutdown temporary tweak cleanup before final recovery state is recorded.
  - Added explicit expert-mode persistence and a Settings page so dangerous tweaks remain visibly gated instead of hidden in backend state only.
  - Added installer/uninstaller/revert architecture documentation, known limitations, and a production-readiness checklist.
  - Ran a full compile/test/build validation sweep across the backend, services, API routes, and frontend shell.
- Files changed:
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/schemas.py`
  - `pulseboost/core/benchmark_engine.py`
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/tests/test_phase10_hardening.py`
  - `docs/INSTALLER_ARCHITECTURE.md`
  - `docs/KNOWN_LIMITATIONS.md`
  - `docs/PRODUCTION_READINESS.md`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m compileall pulseboost/api pulseboost/core pulseboost/tests`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m py_compile pulseboost/core/benchmark_engine.py pulseboost/api/main.py pulseboost/api/routes.py pulseboost/api/schemas.py pulseboost/tests/test_phase10_hardening.py`
- Risks/tradeoffs:
  - The desktop runtime remains Python + WebView; Electron parity is still a tracked product gap.
  - Full browser automation and installer packaging are still documented future work rather than implemented in this repo pass.
  - Test runs surface `anyio` resource warnings from `TestClient`, but the tests pass and no functional failures remain.
- Next phase:
  - Project completed through Phase 10


#### Final Stabilization Summary
- Summary:
  - Reconciled documentation drift so phase status sections, architecture notes, and release-readiness docs now reflect the completed repo state.
  - Audited the React shell and fixed a real frontend/backend mismatch where completed pages existed in code but were not exposed in navigation, while also restoring missing imports and state for those pages.
  - Tightened shipped UX copy around unavailable capabilities, dry-run surfaces, Trust Center wording, Settings wording, empty/loading states, and badge consistency without changing the current runtime.
  - Added release-oriented docs: `CHANGELOG.md`, `TESTING.md`, and `RELEASE_CHECKLIST.md`, plus clear V1 scope and Post-V1 roadmap guidance in release docs.
- Files changed:
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/styles/globals.css`
  - `pulseboost/ui/src/components/AlertBanner.jsx`
  - `pulseboost/ui/src/components/AIPanel.jsx`
  - `docs/ARCHITECTURE_MAP.md`
  - `docs/KNOWN_LIMITATIONS.md`
  - `docs/PRODUCTION_READINESS.md`
  - `docs/INSTALLER_ARCHITECTURE.md`
  - `docs/PULSEBOOST_SPEC.md`
  - `CHANGELOG.md`
  - `TESTING.md`
  - `RELEASE_CHECKLIST.md`
  - `PROGRESS.md`
- Tests run:
  - `pulseboost\tools\python\python.exe -m compileall pulseboost/api pulseboost/core pulseboost/tests`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
- Risks/tradeoffs:
  - The supported desktop runtime remains Python + WebView; Electron parity is still a tracked roadmap item, not part of this stabilization pass.
  - GPU writes, QoS writes, and frame-hook benchmark capture remain honest limitations rather than newly added risky integrations.
  - Frontend validation remains build-based; browser automation is still future work.
- Final state:
  - Project completed through Phase 10 with post-completion stabilization applied.

#### Desktop Redesign And Productization Pass
- Summary:
  - Added `docs/DESIGN_SYSTEM.md` as the redesign source of truth for this pass because the configured `figma` MCP OAuth completed through the CLI, but in-agent MCP resource enumeration still failed with an auth-handshake error.
  - Reworked the React shell into a denser desktop layout with a persistent sidebar, workspace top bar, modular page surfaces, in-app confirmation dialogs, and a unified badge/status system.
  - Extended the PyWebView desktop bridge with runtime metadata, data-directory access, and window-control hooks while preserving the current working runtime.
  - Added `.pbprofile` import support and a focused desktop-shell unit test.
- Files changed:
  - `docs/DESIGN_SYSTEM.md`
  - `docs/ARCHITECTURE_MAP.md`
  - `CHANGELOG.md`
  - `TESTING.md`
  - `RELEASE_CHECKLIST.md`
  - `PROGRESS.md`
  - `pulseboost/desktop_app.py`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/api/client.js`
  - `pulseboost/ui/src/components/AlertBanner.jsx`
  - `pulseboost/ui/src/components/AIPanel.jsx`
  - `pulseboost/ui/src/components/AuditLog.jsx`
  - `pulseboost/ui/src/components/ConfirmDialog.jsx`
  - `pulseboost/ui/src/components/MetricCard.jsx`
  - `pulseboost/ui/src/components/OptimizationFeed.jsx`
  - `pulseboost/ui/src/components/PagePrimitives.jsx`
  - `pulseboost/ui/src/components/PillarSidebar.jsx`
  - `pulseboost/ui/src/components/ProcessList.jsx`
  - `pulseboost/ui/src/components/StatusBadge.jsx`
  - `pulseboost/ui/src/components/StreamChart.jsx`
  - `pulseboost/ui/src/components/Timeline.jsx`
  - `pulseboost/ui/src/pages/AuditPage.jsx`
  - `pulseboost/ui/src/pages/BenchmarkPage.jsx`
  - `pulseboost/ui/src/pages/DashboardPage.jsx`
  - `pulseboost/ui/src/pages/GpuPage.jsx`
  - `pulseboost/ui/src/pages/NetworkPage.jsx`
  - `pulseboost/ui/src/pages/OptimizationsPage.jsx`
  - `pulseboost/ui/src/pages/ProfilesPage.jsx`
  - `pulseboost/ui/src/pages/SettingsPage.jsx`
  - `pulseboost/ui/src/pages/TrustPage.jsx`
  - `pulseboost/ui/src/styles/globals.css`
  - `pulseboost/tests/test_desktop_shell.py`
- Tests run:
  - `pulseboost\\tools\\python\\python.exe -m compileall pulseboost/api pulseboost/core pulseboost/tests pulseboost/desktop_app.py`
  - `pulseboost\\tools\\python\\python.exe -m py_compile pulseboost/desktop_app.py`
  - `pulseboost\\tools\\python\\python.exe -m unittest discover -s pulseboost\\tests`
  - `cmd.exe /c "set PATH=d:\\PulseBoost Python Project\\pulseboost\\tools\\node-tmp\\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
- Risks/tradeoffs:
  - The supported desktop runtime is still PyWebView rather than Electron; the shell is more productized, but runtime migration remains a separate risk-managed project.
  - Frontend validation remains build-based; there is still no browser automation coverage.
  - Full Figma MCP resource access remains blocked in-agent, so the repo-local design system document was used as the practical source of truth.

#### Full UI Refactor Pass
- Summary:
  - Created the official Figma redesign file `PulseBoost AAA Redesign` (`c3fnoUlR1cbDTCtiAGX5lw`) and used it as the design anchor before refactoring the live UI.
  - Rebuilt the desktop shell around grouped navigation, contextual actions, a runtime context strip, stronger page hierarchy, denser workstation spacing, and more consistent desktop-state surfaces.
  - Refactored all major product pages in place so Dashboard, Optimizations, Network, GPU, Audit Log, Benchmark, Game Profiles, Trust Center, and Settings now share one sharper visual system.
  - Upgraded shared primitives and supporting components for notes, signal meters, audit surfaces, chart framing, timeline controls, toasts, and confirmation flows.
- Files changed:
  - `docs/DESIGN_SYSTEM.md`
  - `docs/ARCHITECTURE_MAP.md`
  - `CHANGELOG.md`
  - `PROGRESS.md`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/components/AIPanel.jsx`
  - `pulseboost/ui/src/components/AuditLog.jsx`
  - `pulseboost/ui/src/components/MetricCard.jsx`
  - `pulseboost/ui/src/components/PagePrimitives.jsx`
  - `pulseboost/ui/src/components/StreamChart.jsx`
  - `pulseboost/ui/src/components/Timeline.jsx`
  - `pulseboost/ui/src/pages/AuditPage.jsx`
  - `pulseboost/ui/src/pages/BenchmarkPage.jsx`
  - `pulseboost/ui/src/pages/DashboardPage.jsx`
  - `pulseboost/ui/src/pages/GpuPage.jsx`
  - `pulseboost/ui/src/pages/NetworkPage.jsx`
  - `pulseboost/ui/src/pages/OptimizationsPage.jsx`
  - `pulseboost/ui/src/pages/ProfilesPage.jsx`
  - `pulseboost/ui/src/pages/SettingsPage.jsx`
  - `pulseboost/ui/src/pages/TrustPage.jsx`
  - `pulseboost/ui/src/styles/globals.css`
- Tests run:
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `pulseboost\tools\python\python.exe -m compileall pulseboost\api pulseboost\core pulseboost\tests pulseboost\desktop_app.py`
- Risks/tradeoffs:
  - The Figma file exists and is usable, but Starter-plan MCP rate limiting still prevents repeated large design mutations in a single session.
  - The supported desktop runtime remains Python + WebView, not Electron.
  - Frontend verification is still build-based rather than end-to-end browser automation.

#### Stitch-Driven Desktop Product Refactor
- Summary:
  - Switched the practical design source of truth to the connected Stitch project `PulseBoost Desktop UI/UX Prototype` (`projects/1416118724942608660`) and mirrored its shell, tonal system, and screen mapping in `docs/DESIGN_SYSTEM.md`.
  - Replaced the top-level React shell with a denser workstation frame, stronger desktop chrome, context ribbon, runtime notices, and cleaner navigation hierarchy while preserving the existing backend integrations.
  - Added an Electron desktop shell with preload bridge, backend lifecycle management, and shared desktop API parity, while retaining the existing PyWebView launcher as the fallback path.
  - Updated backend runtime identity reporting so Trust Center, Settings, and shell surfaces can distinguish Electron from PyWebView honestly.
- Files changed:
  - `pulseboost/config.py`
  - `pulseboost/.env.example`
  - `pulseboost/api/main.py`
  - `pulseboost/core/capabilities.py`
  - `pulseboost/desktop_app.py`
  - `pulseboost/electron/main.cjs`
  - `pulseboost/electron/preload.cjs`
  - `pulseboost/ui/package.json`
  - `pulseboost/ui/scripts/launch-desktop.cjs`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/styles/globals.css`
  - `pulseboost/ui/src/pages/SettingsPage.jsx`
  - `pulseboost/tests/test_desktop_shell.py`
  - `start_desktop.ps1`
  - `docs/DESIGN_SYSTEM.md`
  - `docs/ARCHITECTURE_MAP.md`
  - `docs/KNOWN_LIMITATIONS.md`
  - `docs/PRODUCTION_READINESS.md`
  - `CHANGELOG.md`
  - `TESTING.md`
  - `RELEASE_CHECKLIST.md`
  - `PROGRESS.md`
- Tests run:
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd install"`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run desktop:check"`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
  - `pulseboost\tools\python\python.exe -m compileall pulseboost\api pulseboost\core pulseboost\tests pulseboost\desktop_app.py`
- Risks/tradeoffs:
  - Electron is now installed and preferred, but there is still no automated GUI smoke coverage for the actual desktop window lifecycle.
  - PyWebView remains in the repo as the fallback path to preserve a known-good launcher if Electron is unavailable in another environment.
  - Browser automation and installer packaging are still not implemented.

#### Stitch-Led Dashboard Simplification And Product-Ready Shell Pass
- Summary:
  - Used the connected Stitch project mapping plus the repo-local design system mirror to drive a second major UI pass focused on hierarchy discipline instead of more surface density.
  - Rebuilt Dashboard around a hero, key telemetry zone, priority action rail, and segmented deep-inspection area so process tables, timelines, and session review no longer all render at once on first load.
  - Added shared page-hero, segmented-control, and premium-gate primitives and applied them across Optimizations, Benchmark, Audit Log, Trust Center, Settings, Network, GPU, and Game Profiles.
  - Added plan-aware account and premium positioning surfaces without inventing fake billing or entitlements, keeping commercial readiness honest.
- Files changed:
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/components/PagePrimitives.jsx`
  - `pulseboost/ui/src/components/StatusBadge.jsx`
  - `pulseboost/ui/src/pages/AuditPage.jsx`
  - `pulseboost/ui/src/pages/BenchmarkPage.jsx`
  - `pulseboost/ui/src/pages/DashboardPage.jsx`
  - `pulseboost/ui/src/pages/GpuPage.jsx`
  - `pulseboost/ui/src/pages/NetworkPage.jsx`
  - `pulseboost/ui/src/pages/OptimizationsPage.jsx`
  - `pulseboost/ui/src/pages/ProfilesPage.jsx`
  - `pulseboost/ui/src/pages/SettingsPage.jsx`
  - `pulseboost/ui/src/pages/TrustPage.jsx`
  - `pulseboost/ui/src/styles/globals.css`
  - `docs/DESIGN_SYSTEM.md`
  - `docs/ARCHITECTURE_MAP.md`
  - `CHANGELOG.md`
  - `PROGRESS.md`
- Tests run:
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
- Risks/tradeoffs:
  - This pass materially improves hierarchy and product posture, but frontend validation remains build-based and does not yet include desktop UI automation.
  - Premium or locked states are architectural only; there is still no billing or license backend attached.
  - The Vite large-chunk warning remains.

#### Hard Reset UI Replacement Pass
- Summary:
  - Treated the captured Stitch project structure as the source of truth and replaced the visible React UI layer in place instead of extending the previous page composition.
  - Rebuilt the app shell, page primitives, page files, alerts, dialogs, audit runtime trail, copilot panel, and global styling around a new premium desktop workstation system.
  - Removed the older dashboard-era component layer entirely so there is no mixed old/new visual system left in the repo.
  - Simplified Dashboard into machine state, top telemetry, recommendations, trust posture, and staged drill-down views while keeping backend-connected behavior intact.
- Files changed:
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/components/AIPanel.jsx`
  - `pulseboost/ui/src/components/AlertBanner.jsx`
  - `pulseboost/ui/src/components/AuditLog.jsx`
  - `pulseboost/ui/src/components/ConfirmDialog.jsx`
  - `pulseboost/ui/src/components/PagePrimitives.jsx`
  - `pulseboost/ui/src/components/StatusBadge.jsx`
  - `pulseboost/ui/src/pages/AuditPage.jsx`
  - `pulseboost/ui/src/pages/BenchmarkPage.jsx`
  - `pulseboost/ui/src/pages/DashboardPage.jsx`
  - `pulseboost/ui/src/pages/GpuPage.jsx`
  - `pulseboost/ui/src/pages/NetworkPage.jsx`
  - `pulseboost/ui/src/pages/OptimizationsPage.jsx`
  - `pulseboost/ui/src/pages/ProfilesPage.jsx`
  - `pulseboost/ui/src/pages/SettingsPage.jsx`
  - `pulseboost/ui/src/pages/TrustPage.jsx`
  - `pulseboost/ui/src/styles/globals.css`
  - `docs/DESIGN_SYSTEM.md`
  - `docs/ARCHITECTURE_MAP.md`
  - `CHANGELOG.md`
  - `PROGRESS.md`
- Files removed:
  - `pulseboost/ui/src/components/EfficiencyRing.jsx`
  - `pulseboost/ui/src/components/HealthRing.jsx`
  - `pulseboost/ui/src/components/MetricCard.jsx`
  - `pulseboost/ui/src/components/OptimizationFeed.jsx`
  - `pulseboost/ui/src/components/PillarSidebar.jsx`
  - `pulseboost/ui/src/components/ProcessList.jsx`
  - `pulseboost/ui/src/components/StreamChart.jsx`
  - `pulseboost/ui/src/components/Timeline.jsx`
- Tests run:
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run desktop:check"`
  - `pulseboost\tools\python\python.exe -m compileall pulseboost\api pulseboost\core pulseboost\tests pulseboost\desktop_app.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
- Risks/tradeoffs:
  - Stitch MCP server enumeration was not available directly in this chat session, so the authenticated project mapping already captured in-repo was used as the practical source of truth.
  - Frontend verification is still build-based; there is no desktop E2E suite.
  - Billing and license flows are still UI-architecture only, not backend-complete.

#### Local Stitch Export Source-Of-Truth UI Rebuild
- Summary:
  - Stopped live MCP fetching work and used the local Stitch export under `data/stitch/pulseboost_final_mcp/` as the source of truth for the next UI refactor pass.
  - Replaced the remaining page layer in place around the exported Stitch shell language: calmer hierarchy, recessed surfaces, compact desktop density, clearer trust posture, and less simultaneous detail on screen.
  - Fully rewrote Trust Center, Network, GPU, Game Profiles, and Settings to match the local Stitch design direction while preserving the existing backend payloads, runtime truth, plan posture, and safety gates.
  - Kept the already-simplified Dashboard intact as the flagship landing page and validated the full app again through frontend build, desktop checks, backend compile checks, and Python unit tests.
- Files changed:
  - `pulseboost/ui/src/pages/TrustPage.jsx`
  - `pulseboost/ui/src/pages/NetworkPage.jsx`
  - `pulseboost/ui/src/pages/GpuPage.jsx`
  - `pulseboost/ui/src/pages/ProfilesPage.jsx`
  - `pulseboost/ui/src/pages/SettingsPage.jsx`
  - `PROGRESS.md`
  - `CHANGELOG.md`
  - `docs/DESIGN_SYSTEM.md`
- Tests run:
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"` (from `pulseboost/ui`)
  - `cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run desktop:check"` (from `pulseboost/ui`)
  - `pulseboost\tools\python\python.exe -m compileall pulseboost\api pulseboost\core pulseboost\tests pulseboost\desktop_app.py`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests`
- Risks/tradeoffs:
  - Frontend validation is still build-based and desktop-script-based; there is still no desktop E2E suite.
  - The Python test run still surfaces pre-existing `anyio` resource warnings even though the suite passes.
  - Billing and licensing remain UI-architecture-ready only, not backend-complete.

#### Phase 2 Persistence Unification (Migration-Owned SQLite Foundation)
- Summary:
  - Replaced competing persistence bootstrap ownership with one migration-owned database foundation (`core/db/migrations.py` + `core/db/service.py`).
  - Switched `core/database.py` to a compatibility wrapper over the unified service so existing imports continue to work without parallel data layers.
  - Implemented schema versioning with ordered idempotent migrations and expanded domain schema coverage for settings, runtime housekeeping, tweaks, rollback, benchmarks, adaptive/network/GPU snapshots, trust center snapshots, and account/auth/entitlement/device metadata.
  - Added legacy projection/backfill migration paths from old `app_state`, `revert_snapshots`, `benchmarks`, and `network_diagnostics` payloads into normalized tables.
  - Removed runtime feature reliance on generic `app_state` for core flows (settings/runtime adaptive toggles, temporary tweak lifecycle, session recovery, trust snapshots, GPU settings metadata).
  - Unified cognition schema bootstrap under the same migration engine (`MemorySystem` no longer executes its own DDL bootstrap).
- Files changed:
  - `pulseboost/core/database.py`
  - `pulseboost/core/db/__init__.py`
  - `pulseboost/core/db/migrations.py`
  - `pulseboost/core/db/service.py`
  - `pulseboost/core/cognition/memory.py`
  - `pulseboost/core/temporary_tweaks.py`
  - `pulseboost/core/session_recovery.py`
  - `pulseboost/core/trust_center.py`
  - `pulseboost/core/gpu_controller.py`
  - `pulseboost/core/adaptive_engine.py`
  - `pulseboost/core/network_optimizer.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/api/main.py`
  - `pulseboost/tests/test_phase2_database_migrations.py`
  - `docs/PERSISTENCE_ARCHITECTURE.md`
  - `PROGRESS.md`
- Tests/checks run:
  - `pulseboost/tools/python/python.exe -m compileall core\db\migrations.py core\db\service.py core\cognition\memory.py core\temporary_tweaks.py core\session_recovery.py core\trust_center.py core\gpu_controller.py core\adaptive_engine.py api\routes.py api\main.py`
  - `pulseboost/tools/python/python.exe -m compileall core\database.py tests\test_phase2_database_migrations.py`
  - `pulseboost/tools/python/python.exe -m unittest tests.test_phase2_database_migrations tests.test_foundation tests.test_phase9_profiles_trust tests.test_auth_foundation tests.test_phase8_gpu tests.test_phase7_network tests.test_phase6_adaptive tests.test_phase5_benchmark`
  - `pulseboost/tools/python/python.exe -m unittest discover -s pulseboost\tests`
  - `pulseboost/tools/python/python.exe -m compileall pulseboost\core pulseboost\api pulseboost\tests`
  - `npm run build` (from `pulseboost/ui`)
  - `npm run desktop:check` (from `pulseboost/ui`)
- Risks/tradeoffs:
  - Legacy compatibility tables are still present intentionally to avoid breaking existing installs and contracts during migration hardening.
  - Token storage remains encrypted-at-rest blob storage in SQLite via DPAPI-backed cipher rather than OS keychain row references; plaintext token storage is not used.
  - Unit tests continue to pass with pre-existing `anyio` resource warnings not introduced by this phase.

#### Phase 3 Persistence Cleanup And Verification
- Summary:
  - Audited active persistence flows and removed remaining legacy compatibility mirror writes/fallback reads from runtime feature code.
  - Retired `core/db/service.py` legacy table usage for:
    - `app_state` generic writes/reads
    - `revert_snapshots` mirror writes/fallback reads
    - `benchmarks` mirror writes
    - `network_diagnostics` mirror writes/fallback reads
  - Kept a minimal `set_app_state/get_app_state` compatibility shim, but it now maps to normalized settings/runtime tables or compatibility metadata in `app_metadata` rather than legacy table rows.
  - Updated base schema migration behavior so fresh installs do not create legacy compatibility tables; migration import logic still consumes legacy tables when they exist on upgraded installs.
  - Added cleanup-focused persistence tests that verify legacy mirror tables are no longer created for fresh installs and no longer written by active service flows.
- Files changed:
  - `pulseboost/core/db/service.py`
  - `pulseboost/core/db/migrations.py`
  - `pulseboost/tests/test_phase2_database_migrations.py`
  - `docs/PERSISTENCE_ARCHITECTURE.md`
  - `PROGRESS.md`
- Tests/checks run:
  - `pulseboost/tools/python/python.exe -m compileall core\db\migrations.py core\db\service.py tests\test_phase2_database_migrations.py`
  - `pulseboost/tools/python/python.exe -m unittest tests.test_phase2_database_migrations`
  - `pulseboost/tools/python/python.exe -m unittest discover -s pulseboost\tests`
  - `pulseboost/tools/python/python.exe -m compileall pulseboost\core pulseboost\api pulseboost\tests`
  - `npm run build` (from `pulseboost/ui`)
  - `npm run desktop:check` (from `pulseboost/ui`)
- Risks/tradeoffs:
  - Existing upgraded installs may still physically contain legacy compatibility tables from earlier versions; they are now migration-only artifacts and no longer active feature-write targets.
  - Compatibility shim methods (`set_app_state/get_app_state`) are intentionally retained for backward-safe API/test compatibility, but are no longer backed by `app_state` table storage.
  - Python unit suite still emits pre-existing `anyio` resource warnings while passing.

#### Competition Build Hardening Pass (PulseCore Stability + Demo Validation)
- Summary:
  - Fixed a live regression introduced by game-detection payload unification where `/api/tweaks` could raise `KeyError` for non-detected sessions; tweak catalog now safely handles empty detections.
  - Replaced the in-code PulseAI system prompt literal with the exact competition prompt text and kept startup prompt caching + per-message context injection path intact.
  - Re-ran a full local demo flow on a clean backend process: state fetch, tweak catalog load, optimization decision execution, audit fetch, and websocket chat response.
- Files changed:
  - `pulseboost/core/optimizer.py`
  - `pulseboost/api/main.py`
  - `PROGRESS.md`
- Tests/checks run:
  - `pulseboost/tools/python/python.exe -m py_compile api/main.py`
  - `pulseboost/tools/python/python.exe -m unittest discover -s pulseboost/tests`
  - Live demo checks against `http://127.0.0.1:18400`:
    - `GET /healthz`
    - `GET /api/state`
    - `GET /api/tweaks`
    - `GET /api/optimizations?limit=1`
    - `POST /api/optimizations/decision`
    - `GET /api/audit?limit=5`
    - `WS /ws` chat token stream
- Risks/tradeoffs:
  - No active game process was present during this run (`ACTIVE_GAME=none`), so automatic profile switching was validated structurally but not with a real game executable in this session.
  - The Python test suite still emits pre-existing `anyio` `ResourceWarning` messages while passing.

#### Official GitHub Repository Setup Pass
- Summary:
  - Prepared the local source tree for publishing as the official PulseBoost GitHub repository.
  - Added GitHub Actions CI for backend compile checks, Python tests, frontend build, and desktop entry-point validation.
  - Added Dependabot, issue templates, pull request template, security policy, contribution guide, proprietary license notice, editor settings, and Git attributes.
  - Tightened root ignore rules so generated artifacts, runtime databases, logs, portable runtimes, `node_modules`, installers, archives, and local secrets stay out of source control.
  - Rewrote the root README with source-first setup, stack, validation, safety principles, and documentation links.
  - Removed the stray empty root junk file named `{'`.
- Files changed:
  - `.editorconfig`
  - `.gitattributes`
  - `.github/workflows/ci.yml`
  - `.github/ISSUE_TEMPLATE/bug_report.yml`
  - `.github/ISSUE_TEMPLATE/feature_request.yml`
  - `.github/pull_request_template.md`
  - `.github/dependabot.yml`
  - `.gitignore`
  - `README.md`
  - `SECURITY.md`
  - `CONTRIBUTING.md`
  - `LICENSE.md`
  - `docs/GITHUB_REPOSITORY_SETUP.md`
  - `PROGRESS.md`
- Tests/checks run:
  - `pulseboost\tools\python\python.exe -m compileall pulseboost\api pulseboost\core pulseboost\tests`
  - `pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests` (first run hit a transient Windows temp-directory cleanup error; rerun passed 86 tests)
  - `cmd.exe /c "cd /d d:\PulseBoost Python Project\pulseboost\ui&&set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"`
  - `cmd.exe /c "cd /d d:\PulseBoost Python Project\pulseboost\ui&&set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run desktop:check"`
- Risks/tradeoffs:
  - GitHub CLI is installed but not authenticated in this environment, so final remote push requires `gh auth login` or another valid Git credential.
  - The target GitHub repository may contain existing history; replacing it with this source tree should be treated as an intentional repository takeover/cleanup action.

#### Repository Cleanup And Script Organization Pass
- Summary:
  - Removed the stale tracked duplicate `scripts/serve_app.py`; the root `serve_app.py` remains the single active FastAPI/static entrypoint used by Electron and local preview.
  - Fixed `scripts/debug_app.ps1`, `scripts/debug_backend.ps1`, and `scripts/debug_frontend.ps1` so they resolve the repository root correctly from the `scripts/` directory.
  - Updated script organization docs to point developers at the root app entrypoint instead of the removed duplicate.
  - Removed ignored local clutter from the working tree: Python bytecode caches, old `.archive` probe files, and runtime log files.
  - Intentionally kept dependency/runtime folders (`pulseboost/tools`, `pulseboost/ui/node_modules`, `pulseboost/ui/dist`) and SQLite runtime databases to preserve local runnability and app state.
- Files changed:
  - `PROJECT_ORGANIZATION.md`
  - `PROGRESS.md`
  - `scripts/README.md`
  - `scripts/debug_app.ps1`
  - `scripts/debug_backend.ps1`
  - `scripts/debug_frontend.ps1`
  - `scripts/serve_app.py` (removed)
- Tests/checks run:
  - PowerShell parser check for root and script launch helpers.
  - `pulseboost\tools\python\python.exe -B -m unittest discover -s pulseboost\tests`
  - `npm run desktop:check` (from `pulseboost/ui`)
- Risks/tradeoffs:
  - Runtime databases were not removed because they may contain local state and cognition memory.
  - Dependency folders and built frontend output were not removed because deleting them would make the local app slower or impossible to run without reinstall/rebuild steps.

#### Live Frame-Time And Bottleneck Diagnostics Pass
- Summary:
  - Added a real-time performance diagnostics module that classifies the current bottleneck into:
    - `CPU_BOUND`
    - `GPU_BOUND`
    - `RAM_BOUND`
    - `DISK_BOUND`
    - `NETWORK_BOUND`
    - `THERMAL_BOUND`
    - `BACKGROUND_PROCESS_BOUND`
  - Added foreground app capture on Windows through the foreground window PID and process metadata.
  - Added live frame-time ingestion from a configured PresentMon-compatible CSV source via `PRESENTMON_CSV_PATH`.
  - Kept unsupported frame-time behavior honest: without a trusted CSV source, FPS/frame-time fields return explicit unavailable status rather than fabricated values.
  - Enriched orchestrator snapshots, `/api/metrics`, `/api/metrics/live`, and the new `/api/metrics/bottleneck` endpoint with bottleneck input evidence and output label.
  - Surfaced the current bottleneck and current frame-time status on the Dashboard health panel.
- Files changed:
  - `pulseboost/config.py`
  - `pulseboost/core/performance_diagnostics.py`
  - `pulseboost/core/tools/collector.py`
  - `pulseboost/core/agents/orchestrator.py`
  - `pulseboost/core/metrics.py`
  - `pulseboost/api/main.py`
  - `pulseboost/api/routes.py`
  - `pulseboost/tests/test_performance_diagnostics.py`
  - `pulseboost/ui/src/App.jsx`
  - `pulseboost/ui/src/store/useSystemStore.js`
  - `pulseboost/ui/src/pages/DashboardPage.jsx`
  - `docs/KNOWN_LIMITATIONS.md`
  - `docs/PULSEBOOST_FEATURE_MATRIX.md`
  - `docs/PULSEBOOST_TECHNICAL_ARCHITECTURE_CURRENT.md`
  - `PROGRESS.md`
- Tests/checks run:
  - `tools\python\python.exe -B -m unittest tests.test_performance_diagnostics`
  - `tools\python\python.exe -B -m unittest discover -s tests`
  - `npm run build` (from `pulseboost/ui`)
  - `npm run desktop:check` (from `pulseboost/ui`)
- Risks/tradeoffs:
  - Native hook/ETW capture is still not bundled; live frame-time values require a PresentMon-compatible CSV source configured by environment variable.
  - GPU-bound confidence depends on vendor telemetry availability; when telemetry is unavailable, the classifier caps confidence and falls back to CPU/RAM/disk/network/process evidence.
  - Network-bound detection is throughput-based until game-server latency/jitter evidence is wired into the same classifier.
