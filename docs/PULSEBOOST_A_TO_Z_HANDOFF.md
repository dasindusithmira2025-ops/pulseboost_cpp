# PulseBoost AI - Canonical A-to-Z Master Handoff

Last updated: April 1, 2026  
Repository root: `d:\HelaDev\Finished And ongoing freelancing Projects\Pulseboost`  
Target: Windows desktop app (Qt/QML + C++20 + Win32 APIs)

---

## 1) Scope and scan method

This document was rebuilt from a source-level scan and is intended to be the single handoff reference for a new agent.

### Scan scope (included)
- `app/`
- `ai/` and `ai/model/`
- `core/`
- `common/`
- `modules/`
- `ui_backend/`
- `include/PulseBoostAI/`
- `ui/qml/`
- `data/*.cpp` and data runtime artifacts used by app
- `scripts/`
- root configs (`CMakeLists.txt`, `CMakePresets.json`, `qml.qrc`, installer script)

### Excluded from deep explanation
- `build_run/`, generated Qt/CMake artifacts, `moc`/`rcc` outputs, packaged Qt runtime files, binaries.

### Line-anchor convention
- `path:Lx-Ly` means code-backed reference from repository root.
- Explanations are function/block anchored (not literal line-by-line transcription).

---

## 2) Product and runtime identity

- App target: `PulseBoost` executable renamed to `PulseBoostAI.exe` (`CMakeLists.txt:L32-L115`)
- Version: `1.0.0` (`CMakeLists.txt:L3`, `app/main.cpp:L143-L146`)
- Runtime style: Qt Quick Controls `Basic` + Direct3D11 scene graph (`app/main.cpp:L133-L134`)
- Organization/app names: `PulseBoost`, `PulseBoost AI` (`app/main.cpp:L143-L145`)
- QML entrypoint: `qrc:/ui/qml/main.qml` (`app/main.cpp:L251-L266`, `qml.qrc:L15`)

---

## 3) Build / run / deploy / smoke runbook

## 3.1 Build graph
- Qt modules required: `Core`, `Gui`, `Quick`, `QuickControls2`, `Sql`, `Concurrent` (`CMakeLists.txt:L21-L28`)
- Win32 link dependencies: `advapi32`, `iphlpapi`, `ole32`, `oleaut32`, `pdh`, `powrprof`, `psapi`, `runtimeobject`, `shell32`, `srclient`, `wbemuuid`, `wininet`, `urlmon`, `dbghelp`, `crypt32` (`CMakeLists.txt:L139-L156`)

## 3.2 Commands
- Current workspace build artifact check passed for:
  - `build_run/Release/PulseBoostAI.exe`
  - `scripts/build.ps1`
  - `scripts/test.ps1`
  - `qml.qrc`
  - `CMakeLists.txt`

Manual/build_run path:
```powershell
cmake --build build_run --config Release
.\build_run\Release\PulseBoostAI.exe
```

Scripted build path (`build/Release` output):
```powershell
.\scripts\build.ps1 -QtPath "C:\Qt\6.6.3\msvc2019_64"
```
(`scripts/build.ps1:L1-L62`)

Smoke CLI script:
```powershell
.\scripts\test.ps1
```
(`scripts/test.ps1:L1-L31`)

Qt deploy example:
```powershell
& "C:\Qt\6.6.3\msvc2019_64\bin\windeployqt.exe" --release --qmldir ".\ui\qml" ".\build_run\Release\PulseBoostAI.exe"
```

---

## 4) Architecture and thread model

## 4.1 Boot path and dependency wiring
- Crash reporter initialized first (`app/main.cpp:L55`)
- CLI-mode early exit branch before GUI init (`app/main.cpp:L57-L131`)
- GUI mode:
  - style/renderer setup (`app/main.cpp:L133-L134`)
  - font loading from `:/fonts/*` (`app/main.cpp:L148-L162`)
  - service construction and dependency injection (`app/main.cpp:L164-L212`)
  - context object registration (`app/main.cpp:L237-L245`)
  - QML load + objectCreated failure handling (`app/main.cpp:L251-L266`)
  - telemetry start + event loop (`app/main.cpp:L269-L275`)

## 4.2 Thread model
- `TelemetryWorker` moved to dedicated `QThread` (`core/telemetry_engine.cpp:L156-L167`)
- Timers inside worker thread:
  - 1s CPU/RAM/gpu/net vitals (`core/telemetry_engine.cpp:L82`, `L108-L120`)
  - 3s disk/storage (`core/telemetry_engine.cpp:L83`, `L122-L135`)
  - 5s process/startup/services/drivers/thermals (`core/telemetry_engine.cpp:L84`, `L137-L154`)
- UI receives snapshots through queued signal (`core/telemetry_engine.cpp:L163-L167`)
- Stop path is queued + thread join with timeout (`core/telemetry_engine.cpp:L177-L183`)

## 4.3 Runtime data flow
1. `SystemScanner` produces snapshots (`core/system_scanner.cpp:L274-L366`)
2. `TelemetryEngine` emits `snapshotReady` (`core/telemetry_engine.cpp:L108-L154`)
3. `main.cpp` fan-out:
   - cache push
   - `UiController::onSnapshotReady`
   - telemetry log append
   - `AgentEngine::updateSnapshot`
   (`app/main.cpp:L219-L235`)
4. QML binds via `SystemCtrl` properties (`include/PulseBoostAI/ui_backend/ui_controller.hpp:L26-L58`)

---

## 5) Dependency map (Pass 1 output)

- Boot path: `app/main.cpp` -> all services/controllers.
- Telemetry path: `core/system_scanner.cpp` + `core/telemetry_engine.cpp` + `data/telemetry_cache.cpp` + `data/telemetry_logger.cpp`.
- AI path: `ai/agent_engine.cpp` -> `ai/pulse_model.cpp` -> `ai/model/*` + action modules.
- UI bridge path: `ui_backend/ui_controller.cpp` + `feature_gate.cpp` + `ui_preferences.cpp`.
- QML shell path: `ui/qml/main.qml` -> `layout/AppWindow.qml` -> `TopBar/Sidebar/ContentArea/StatusBar` -> screens/components.

---

## 6) C++ subsystem deep dive (file/function anchored)

## 6.1 App bootstrap

### `app/main.cpp`
- Purpose: single process entry, CLI path, GUI path, service wiring.
- Inputs: CLI args, config/env (`CMAKE_PREFIX_PATH` at build time), runtime telemetry.
- Outputs: GUI app loop or CLI JSON/text output; startup context properties.
- Side effects:
  - crash system init (`L55`)
  - log writes to `logs/gui_runtime.log` via helper (`L44-L51`)
  - settings default format set to INI (`L148`)
  - telemetry logging and cache mutation (`L219-L235`)
- Error/fallback:
  - object creation failure exits `-1` (`L257-L264`)
  - CLI commands return without GUI boot (`L57-L131`).

## 6.2 Common and hardware access

### `common/wmi_wrapper.cpp`
- `WmiWrapper::WmiWrapper` initializes COM + security (`L25-L38`).
- `connect` opens WMI namespace and sets proxy blanket (`L44-L79`).
- `querySingleString` runs WQL, returns first BSTR property (`L81-L114`).
- `executeMethod` intentionally returns `false` (fallback route expected) (`L116-L123`).
- Risk: method execution path is stubbed; callers must keep fallback behavior.

### `include/PulseBoostAI/common/models.hpp`
- Canonical runtime structs:
  - process/startup/service/driver/storage entities (`L20-L58`)
  - memory/disk summaries (`L60-L74`)
  - action and plan records (`L83-L100`)
  - `SystemSnapshot` state contract (`L102-L140`).

## 6.3 Core telemetry and diagnostics

### `core/system_scanner.cpp`
- Constructor opens PDH query/counters (`L74-L90`).
- `nextCounterValue` guarded sampling (`L98-L111`).
- Enrichment blocks:
  - drivers from `Win32_PnPSignedDriver` (`L113-L152`)
  - network bytes + GPU utilization (`L154-L212`)
  - thermal/fan from `MSAcpi_ThermalZoneTemperature` + fallback formulas (`L214-L259`)
  - storage (`L290-L300`)
  - startup/services (`L302-L310`)
- Health calculation:
  - weighted score (`L261-L272`)
  - issue list + subscore derivation + summary string (`L312-L361`)
- Full scan orchestration: vitals -> enrich -> finalize (`L363-L366`).

### `core/telemetry_engine.cpp`
- Snapshot merge policy (`L11-L67`) keeps deltas coherent.
- Worker start and timer scheduling (`L73-L94`).
- Tick handlers with per-stage exception guards:
  - CPU/RAM tick (`L108-L120`)
  - disk tick (`L122-L135`)
  - process/driver/thermal tick (`L137-L154`).

### `core/process_manager.cpp`
- Process enumeration + CPU/memory collection (`L38`).
- Process control APIs:
  - set priority (`L125`)
  - set affinity (`L136`)
  - suspend (`L147`)
  - resume (`L175`).

### `core/memory_analyzer.cpp`
- `analyze` computes usage and top memory consumers (`L11`).

### `core/disk_analyzer.cpp`
- `analyzeSystemDrive` computes totals/percent and large files (`L61`).
- `buildStorageMap` builds top-level category map (`L88`).
- `findLargestFiles` recursive file ranking (`L150`).

### `core/startup_optimizer.cpp`
- Startup scan over run keys/tasks/etc wrappers (`L142`).
- Disable item with backup support (`L155`).
- Delay item insertion (`L172`).

### `core/service_manager.cpp`
- Enumerate services (`L42`), stop (`L94`), start (`L113`).

### `core/registry_optimizer.cpp`
- Restore point proxy (`L11`), run-key backup (`L26`), value delete (`L56`).

## 6.4 Safety / license / updater / crash

### `modules/safety_guard.cpp`
- `createRestorePoint` via `srclient` and/or fallback (`L71`).
- Registry backup helper (`L109`).
- Signed audit-style action log append (`L120`).

### `core/license_manager.cpp`
- Local tier/trial state in `QSettings` + cached file (`L21-L25`, `L94-L99`).
- Key validation and tier decision (`L103-L133`).
- Trial countdown and expiry gates (`L145-L160`).

### `core/auto_updater.cpp`
- Manifest fetch + parse + semver compare (`L100-L118`).
- Default endpoint: `https://api.heladevstudio.com/v1/updates/manifest.json` (`L104`).
- Installer download to temp (`L120-L141`).
- Install/restart launcher (`L143-L158`).

### `core/crash_reporter.cpp`
- Init and unhandled exception hook setup (`L120`).
- Pending crash detection APIs for startup banner (`L136-L153`).

## 6.5 Optimization modules

### Action modules in active route
- `modules/junk_cleaner.cpp`: target discovery + safe delete (`L33`, `L47`).
- `modules/game_mode.cpp`: enable/disable game profile (`L28`, `L52`, `L62`).
- `modules/developer_mode.cpp`: profile actions (`L22`).
- `modules/network_optimizer.cpp`: DNS flush / TCP optimize / latency probe (`L15`, `L20`, `L33`).
- `modules/ram_optimizer.cpp`: working-set trim (`L10`).
- `modules/defrag_trigger.cpp`: system drive optimize trigger (`L9`).
- `modules/duplicate_file_finder.cpp`: hash + duplicate grouping (`L15`, `L32`).
- `modules/large_file_scanner.cpp`: recursive top file scan (`L7`).

### Legacy utility wrappers (console-style namespace wrappers)
- `modules/process_optimizer.cpp`, `modules/service_manager.cpp`, `modules/startup_optimizer.cpp`, `modules/disk_analyzer.cpp` expose lightweight wrappers under `pulseboost::modules::*` and are not the primary UI action path.

## 6.6 AI subsystem

### `ai/agent_engine.cpp`
- Constructor initializes PulseModel and streaming timer (`L14-L63`).
- `sanitizeErrorForUi` blocks raw error leakage (`L70-L73`).
- `executeActionByName` central action router (`L99-L224`):
  - maps action ids to concrete modules
  - records optimization history for each operation
  - enforces critical-process guard in kill path.
- `ask` orchestration (`L242-L337`):
  - local memory update
  - optional rule-based plan execution
  - PulseModel inference call
  - action metadata emission
  - streamed reply output.
- Feedback APIs to adaptive model (`L339-L370`).

### `ai/pulse_model.cpp`
- `initialize` builds four-layer local stack (`L7-L14`).
- `processInput` path: classify -> fuse -> generate -> record interaction (`L20-L66`).
- `submitFeedback`/`submitActionResult` feed adaptive engine (`L68-L88`).
- `resetLearning` and stats accessors (`L97-L118`).

### `ai/model/intent_classifier.cpp`
- Ruleset build for greeting/diagnostic/action intents (`L12-L211`).
- TF-IDF-like keyword scoring (`L214-L241`).
- Entity extraction for process actions (`L244-L261`).
- Slot extraction (delay/metric/category/entity) (`L263-L304`).
- Final classification with confidence threshold (`L306-L341`).

### `ai/model/context_fusion.cpp`
- Fuses snapshot + history + recent actions + conversation summary (`L38-L101`).
- Primary issue, usage mode, game/dev detection (`L103-L152`).
- Health grade labeling (`L154-L165`).

### `ai/model/response_generator.cpp`
- Template library build and auto-pack expansion (`L15-L219`).
- Weighted template selection with jitter (`L221-L239`).
- Action decoration to runnable action ids (`L250-L297`).
- Slot filling and final response generation (`L299-L345`).

### `ai/model/adaptive_engine.cpp`
- SQLite initialization and schema setup (`L30-L99`).
- Interaction persistence + weight decay/update (`L102-L141`).
- Feedback mutation paths (`L143-L178`).
- Weight counter updates and read APIs (`L191-L261`).
- Preference inference and aggregate stats (`L279-L353`).
- Reset learned data (`L355-L363`).

### Other AI utilities
- `ai/chat_router.cpp` intent-to-plan fallback router (`L7`, `L49`).
- `ai/ai_diagnostics_engine.cpp` local risk reasoning + summary QA fallback (`L136`, `L288`, `L319`).
- `ai/proactive_monitor.cpp` snapshot-fed proactive alert builder (`L10`, `L45`).
- `ai/context_builder.cpp`, `ai/prompt_builder.cpp`, `ai/system_prediction.cpp`, `ai/session_memory.cpp`, `ai/chatbot_interface.cpp` provide legacy/local helper flows and compatibility paths.

## 6.7 UI backend bridge

### `include/PulseBoostAI/ui_backend/ui_controller.hpp`
- QML-exposed state contract:
  - metrics/charts/process/storage/actions/game/snapshots/ui-state (`L26-L58`)
- Action invokables and control APIs (`L109-L130`)
- New shell-level health/state props:
  - `uiDataReady`
  - `uiErrorMessage`
  - `telemetryAgeMs`.

### `ui_backend/ui_controller.cpp`
- Snapshot ingest + notify fan-out (`L260-L273`).
- Rich view models:
  - processes sorted/risk-labeled (`L283-L310`)
  - driver status synthesis (`L312-L347`)
  - treemap layout engine (`L385-L453`)
  - recent action typing/tone/saved-mb parsing (`L469-L503`)
  - network/memory/thermal overview synthesis (`L504-L563`)
  - notification center aggregation (`L565-L607`).
- Snapshot capture/restore workflow (`L658-L783`).
- Action methods (clean/optimize/game/network/ram/disk/startup/export) (`L807-L996`).
- `refreshAll` UI-wide manual refresh signal fan-out (`L997-L1006`).

### `ui_backend/feature_gate.cpp`
- Tier view model + activation + refresh (`feature_gate.cpp:L5-L31`).

### `ui_backend/ui_preferences.cpp`
- Persistent UI/settings state with notifier semantics (`ui_preferences.cpp:L24-L60`).

### `ui_backend/notification_manager.cpp`
- Native toast adapter class (`notification_manager.cpp:L12-L25`).

## 6.8 Data persistence layer

### `data/telemetry_cache.cpp`
- Ring buffer push/history/latest/chart projection (`L7-L29`).

### `data/telemetry_logger.cpp`
- CSV creation and append (`L10-L24`).
- Recent CSV parse (`L26-L53`).

### `data/optimization_history.cpp`
- CSV action history write/load (`L8-L38`).

---

## 7) QML subsystem deep dive

## 7.1 Style system
- `ui/qml/style/Style.qml`:
  - tokenized color/spacing/radius/typography/motion functions (`Style.qml:L5-L181`)
- `Typography.qml`: font loaders for Rajdhani and IBM Plex families (`Typography.qml:L5-L13`)
- `Animations.qml`: shared durations/easing constants (`Animations.qml:L6-L23`)
- `Icons.qml`: glyph mapping helper (`Icons.qml:L5`)

## 7.2 Shell

### `ui/qml/main.qml`
- Root window config: `AppWindow`, default size/min size (`main.qml:L6-L13`).

### `ui/qml/layout/AppWindow.qml`
- Frameless shell + layered ambient background (`AppWindow.qml:L11-L90`).
- Main composition: `Sidebar` + glass host panel + `TopBar` + `ContentArea` + `StatusBar` (`L91-L236`).
- Drag move in windowed mode (`L240-L248`).
- Overlay onboarding gate (`L250-L267`).
- Toast bridge and telemetry-driven smart alerts (`L271-L333`).
- Tray integration and minimize-to-tray close interception (`L296-L315`).
- Scheduling hook exists but currently empty (`checkScheduledTasks`) (`L342`).

### `ui/qml/layout/TopBar.qml`
- Traffic-light window controls (`L25-L31`).
- Search input + refresh (`SystemCtrl.refreshAll`) (`L54-L85`).
- Notification bell and popup center bound to `SystemCtrl.notifications` (`L95-L216`).

### `ui/qml/layout/Sidebar.qml`
- Navigation model with all screen ids (`L23-L39`).
- Active-state glass row delegate and collapse behavior (`L97-L163`).
- Bottom PulseModel readiness card and collapse toggle (`L170-L234`).

### `ui/qml/layout/ContentArea.qml`
- Async `Loader` for screen components + enter animation (`L18-L41`).
- Built-in loading/error overlays via `SystemCtrl.uiDataReady` and `uiErrorMessage` (`L43-L55`).
- Screen component routing table (`L57-L85`).

### `ui/qml/layout/StatusBar.qml`
- Bottom heartbeat row for PulseModel state + screen breadcrumb + health pill + live clock (`StatusBar.qml:L8-L73`).

## 7.3 Shared components

- Foundation: `GlassPanel`, `GlassCard`, `GlassOverlay`, `TrafficLightWindowControls`, `PrimaryInput`, `DataTableRow`, `StatusPill`, `SectionHeader`
- Controls: `GlowButton`, `IconButton`
- Cards/charts: `MetricCard`, `HealthRingLarge`, `LineChart`, `SparkLine`
- Feedback: `ToastContainer`, `ToastNotification`, `LoadingState`, `EmptyState`, `ErrorState`
- AI: `ChatBubble`, `StreamingText`, `TypingIndicator`, `QuickActionChips`, `ContextChip`

Key interaction contracts:
- hover/press animations and stateful visuals are implemented across core controls/components (`ui/qml/components/**/*`).

## 7.4 Screens

- `Dashboard.qml`:
  - health ring + KPI cards + rolling chart + action row + top processes + recent actions (`Dashboard.qml:L81-L424`)
- `Charts.qml`:
  - range selector + export + multi-metric chart blocks (`Charts.qml:L83-L243`)
- `AiChat.qml`:
  - local session model + proactive cards + streaming reply + action metadata feedback loop (`AiChat.qml:L18-L391`)
- `ProcessManager.qml`:
  - sortable/filterable process rows + suspend/kill actions (`ProcessManager.qml:L23-L285`)
- `StorageAnalyzer.qml`:
  - treemap + large files + clean action (`StorageAnalyzer.qml:L15-L326`)
- `NetworkMonitor.qml`:
  - overview cards + chart + flush/tune/refresh actions (`NetworkMonitor.qml:L16-L296`)
- `StartupManager.qml`:
  - startup list + delay/disable batch operations (`StartupManager.qml:L61-L308`)
- `HealthHistory.qml`:
  - range tabs + snapshots + restore + report dialog (`HealthHistory.qml:L17-L357`)
- `SecurityScanner.qml`:
  - score visuals + findings + low-risk autofix UX (`SecurityScanner.qml:L17-L321`)
- `Settings.qml`:
  - licensing + PulseModel stats + schedule/system toggles (`Settings.qml:L29-L431`)
- `GameMode.qml`, `RamOptimizer.qml`, `Temps.qml`, `DriverManager.qml`, `Onboarding.qml`:
  - dedicated views wired to `SystemCtrl` and reusable components.

---

## 8) C++ <-> QML contracts

## 8.1 Context objects registered at startup
- `SystemCtrl` -> `UiController` (`app/main.cpp:L237`)
- `AgentEngine` -> AI orchestration (`app/main.cpp:L238`)
- `FeatureGate` -> licensing/tier state (`app/main.cpp:L239`)
- `UiPrefs` -> local UI preferences (`app/main.cpp:L240`)
- startup flags:
  - `StartupCrashDetected`, `StartupCrashReportPath`
  - `StartupUpdateAvailable`, `StartupUpdateVersion`, `StartupUpdateUrl`
  (`app/main.cpp:L241-L245`)

## 8.2 Contract ownership
- `SystemCtrl` owns system state and imperative optimization actions.
- `AgentEngine` owns AI chat lifecycle + action metadata + adaptive feedback.
- `FeatureGate` owns trial/pro activation and restrictions.
- `UiPrefs` owns persisted UX/system toggles.

## 8.3 Interface stability guidance
- Do not rename `Q_PROPERTY`/`Q_INVOKABLE` symbols in `UiController` without synchronized QML updates (`ui_controller.hpp:L26-L130`).
- New UI state should be added as property+notifier pair to keep binding updates deterministic.

---

## 9) AI action routing and safety flow

## 9.1 Routing
- AI-intent actions are mapped in one place: `AgentEngine::executeActionByName` (`ai/agent_engine.cpp:L99-L224`).
- Supported action IDs include:
  - `create_restore_point`
  - `clean_junk`
  - `enable_game_mode`
  - `optimize_developer_mode`
  - `analyze_startup`
  - `optimize_ram`
  - `optimize_disk`
  - `optimize_network`
  - `full_scan`
  - `optimize_all`
  - `kill_process:<entity>`

## 9.2 Safety checks
- Kill path skips critical processes and only targets matched non-critical names (`ai/agent_engine.cpp:L194-L223`).
- Major optimize flows can create restore points (`ai/agent_engine.cpp:L106-L110`, `ui_controller.cpp:L815-L819`).
- All executed actions are recorded to optimization history (`agent_engine.cpp` and `ui_controller.cpp` action methods).

## 9.3 Error behavior
- `sanitizeErrorForUi` drops raw internal errors from user chat output (`ai/agent_engine.cpp:L70-L73`).
- Streaming and action feedback are resilient to empty/failed states (`ai/agent_engine.cpp:L226-L240`, `L347-L355`).

---

## 10) Data models and persistence

## 10.1 Core runtime model
- `SystemSnapshot` and related structs define primary data contracts (`include/PulseBoostAI/common/models.hpp:L10-L142`).

## 10.2 Persistence locations
- Telemetry CSV: `logs/telemetry.csv` (`data/telemetry_logger.cpp:L10-L24`)
- Optimization history CSV: `logs/optimization_history.csv` (`data/optimization_history.cpp:L8-L24`)
- Adaptive AI DB: `data/pulsemodel_adaptive.sqlite` (`ai/agent_engine.cpp:L32`, `ai/model/adaptive_engine.cpp:L30-L42`)
- Snapshot JSONs: `%AppData%/.../snapshots` with fallback to local data dir (`ui_controller.cpp:L74-L79`, `L658-L719`)
- License cache: `logs/license.dat` (`core/license_manager.cpp:L25`)
- Crash logs/dumps: `logs/crash/*` (`core/crash_reporter.cpp:L120-L153`)

## 10.3 Settings
- `QSettings` INI format globally (`app/main.cpp:L148`)
- UI preference keys in `UiPreferences` (`ui_backend/ui_preferences.cpp:L24-L60`)
- License/trial keys in `LicenseManager` (`core/license_manager.cpp:L21-L24`).

---

## 11) Installer / updater / crash / license / trial behavior

## 11.1 Installer
- Repo includes Inno Setup script: `PulseBoost_Installer.iss` (distribution packaging path).
- CMake and scripts handle build/deploy; installer signing pipeline is external to current source tree.

## 11.2 Updater
- Manifest check + download + install path in `AutoUpdater` (`core/auto_updater.cpp:L100-L171`).
- Startup update flags are projected to QML at boot (`app/main.cpp:L200-L205`, `L243-L245`).

## 11.3 Crash recovery
- Crash reporter initialized before any heavy startup (`app/main.cpp:L55`).
- Pending crash surfaced and then cleared on launch (`app/main.cpp:L189-L194`).

## 11.4 License and trial
- License and trial are local-state driven (`core/license_manager.cpp:L94-L163`).
- Feature gate exposes state to UI (`ui_backend/feature_gate.cpp:L8-L31`).
- Trial gating in AppWindow blocks non-dashboard/settings pages when expired (`AppWindow.qml:L180-L231`).

---

## 12) Known risks and validated status

## 12.1 Validated current status (from source scan)
- Local PulseModel pipeline is primary active chat path; no runtime dependence on cloud AI for main chat flow (`ai/agent_engine.cpp:L14-L370`, `ai/pulse_model.cpp:L7-L118`).
- Shell supports loading/error guard states via `SystemCtrl.uiDataReady` / `uiErrorMessage` (`ContentArea.qml:L43-L55`).
- Frameless window + drag move + tray integration are implemented (`AppWindow.qml:L11-L13`, `L240-L248`, `L296-L315`).

## 12.2 Risks to track
- `WmiWrapper::executeMethod` is intentionally stubbed false (`common/wmi_wrapper.cpp:L116-L123`).
- `AppWindow.checkScheduledTasks` currently empty placeholder (`AppWindow.qml:L342`).
- Updater uses simple JSON extraction and direct URL download; malformed manifest handling is basic (`core/auto_updater.cpp:L107-L118`).
- There are parallel legacy AI helper paths (`chatbot_interface`, `ai_diagnostics_engine`) that are not the primary QML chat contract; maintainers should avoid mixing paths unintentionally.
- Scripts default to `build/Release`, while many local workflows use `build_run/Release`; keep launch/deploy scripts consistent per environment.

---

## 13) New agent first 60 minutes playbook

## 0-10 minutes
1. Read this file end-to-end.
2. Open `app/main.cpp`, `ui_controller.hpp`, `agent_engine.cpp`, `AppWindow.qml`, `ContentArea.qml`.
3. Confirm active runtime path (`build_run/Release/PulseBoostAI.exe`).

## 10-25 minutes
1. Build/run baseline.
2. Validate startup boot path and that `SystemCtrl` data binds in dashboard.
3. Check `logs/gui_runtime.log` for QML warnings.

## 25-40 minutes
1. Trace one end-to-end action: QML button -> `SystemCtrl` invokable -> module call -> action history.
2. Trace AI path: `AiChat.qml` send -> `AgentEngine.ask` -> `PulseModel` -> reply stream.

## 40-60 minutes
1. Pick one isolated fix area (UI, telemetry, AI routing, or module).
2. Confirm no contract break in `ui_controller.hpp` and `agent_engine.hpp`.
3. Re-run smoke path and verify no new runtime warnings.

---

## 14) Full source index (non-generated files in scope)

## 14.1 Root
- `.gitignore`
- `CMakeLists.txt`
- `CMakePresets.json`
- `PulseBoost_Installer.iss`
- `README.md`
- `qml.qrc`

## 14.2 App
- `app/main.cpp`

## 14.3 Common
- `common/wmi_wrapper.cpp`
- `include/PulseBoostAI/common/models.hpp`
- `include/PulseBoostAI/common/windows_utils.hpp`
- `include/PulseBoostAI/common/wmi_utils.hpp`
- `include/PulseBoostAI/common/wmi_wrapper.hpp`

## 14.4 Core
- `core/auto_updater.cpp`
- `core/crash_reporter.cpp`
- `core/disk_analyzer.cpp`
- `core/license_manager.cpp`
- `core/memory_analyzer.cpp`
- `core/process_manager.cpp`
- `core/registry_optimizer.cpp`
- `core/service_manager.cpp`
- `core/startup_optimizer.cpp`
- `core/system_scanner.cpp`
- `core/telemetry_engine.cpp`
- `core/telemetry_engine.h`
- `include/PulseBoostAI/core/auto_updater.hpp`
- `include/PulseBoostAI/core/crash_reporter.hpp`
- `include/PulseBoostAI/core/disk_analyzer.hpp`
- `include/PulseBoostAI/core/license_manager.hpp`
- `include/PulseBoostAI/core/memory_analyzer.hpp`
- `include/PulseBoostAI/core/process_manager.hpp`
- `include/PulseBoostAI/core/registry_optimizer.hpp`
- `include/PulseBoostAI/core/service_manager.hpp`
- `include/PulseBoostAI/core/startup_optimizer.hpp`
- `include/PulseBoostAI/core/system_scanner.hpp`
- `include/PulseBoostAI/core/telemetry_engine.hpp`

## 14.5 AI
- `ai/agent_engine.cpp`
- `ai/ai_diagnostics_engine.cpp`
- `ai/chat_router.cpp`
- `ai/chatbot_interface.cpp`
- `ai/context_builder.cpp`
- `ai/proactive_monitor.cpp`
- `ai/prompt_builder.cpp`
- `ai/pulse_model.cpp`
- `ai/session_memory.cpp`
- `ai/system_prediction.cpp`
- `ai/model/adaptive_engine.cpp`
- `ai/model/context_fusion.cpp`
- `ai/model/intent_classifier.cpp`
- `ai/model/response_generator.cpp`
- `include/PulseBoostAI/ai/agent_engine.hpp`
- `include/PulseBoostAI/ai/ai_diagnostics_engine.hpp`
- `include/PulseBoostAI/ai/chat_router.hpp`
- `include/PulseBoostAI/ai/chatbot_interface.hpp`
- `include/PulseBoostAI/ai/context_builder.hpp`
- `include/PulseBoostAI/ai/proactive_monitor.hpp`
- `include/PulseBoostAI/ai/prompt_builder.hpp`
- `include/PulseBoostAI/ai/pulse_model.hpp`
- `include/PulseBoostAI/ai/session_memory.hpp`
- `include/PulseBoostAI/ai/system_prediction.hpp`
- `include/PulseBoostAI/ai/model/adaptive_engine.hpp`
- `include/PulseBoostAI/ai/model/context_fusion.hpp`
- `include/PulseBoostAI/ai/model/intent_classifier.hpp`
- `include/PulseBoostAI/ai/model/response_generator.hpp`

## 14.6 Modules
- `modules/defrag_trigger.cpp`
- `modules/developer_mode.cpp`
- `modules/disk_analyzer.cpp`
- `modules/duplicate_file_finder.cpp`
- `modules/game_mode.cpp`
- `modules/junk_cleaner.cpp`
- `modules/large_file_scanner.cpp`
- `modules/network_optimizer.cpp`
- `modules/process_optimizer.cpp`
- `modules/ram_optimizer.cpp`
- `modules/safety_guard.cpp`
- `modules/service_manager.cpp`
- `modules/startup_optimizer.cpp`
- `include/PulseBoostAI/modules/defrag_trigger.hpp`
- `include/PulseBoostAI/modules/developer_mode.hpp`
- `include/PulseBoostAI/modules/duplicate_file_finder.hpp`
- `include/PulseBoostAI/modules/game_mode.hpp`
- `include/PulseBoostAI/modules/junk_cleaner.hpp`
- `include/PulseBoostAI/modules/large_file_scanner.hpp`
- `include/PulseBoostAI/modules/network_optimizer.hpp`
- `include/PulseBoostAI/modules/ram_optimizer.hpp`
- `include/PulseBoostAI/modules/safety_guard.hpp`

## 14.7 Data
- `data/optimization_history.cpp`
- `data/telemetry_cache.cpp`
- `data/telemetry_logger.cpp`
- `include/PulseBoostAI/data/optimization_history.hpp`
- `include/PulseBoostAI/data/telemetry_cache.hpp`
- `include/PulseBoostAI/data/telemetry_logger.hpp`
- `data/common_games.json`
- `data/pulsemodel_adaptive.sqlite` (runtime artifact used by AdaptiveEngine)

## 14.8 UI backend
- `ui_backend/feature_gate.cpp`
- `ui_backend/notification_manager.cpp`
- `ui_backend/ui_controller.cpp`
- `ui_backend/ui_preferences.cpp`
- `include/PulseBoostAI/ui_backend/feature_gate.hpp`
- `include/PulseBoostAI/ui_backend/notification_manager.hpp`
- `include/PulseBoostAI/ui_backend/ui_controller.hpp`
- `include/PulseBoostAI/ui_backend/ui_preferences.hpp`

## 14.9 QML shell/style/components/screens
- `ui/qml/main.qml`
- Layout:
  - `ui/qml/layout/AppWindow.qml`
  - `ui/qml/layout/TopBar.qml`
  - `ui/qml/layout/Sidebar.qml`
  - `ui/qml/layout/ContentArea.qml`
  - `ui/qml/layout/StatusBar.qml`
- Style:
  - `ui/qml/style/Style.qml`
  - `ui/qml/style/Typography.qml`
  - `ui/qml/style/Icons.qml`
  - `ui/qml/style/Animations.qml`
  - `ui/qml/style/qmldir`
- Font resources:
  - `ui/qml/resources/fonts/Rajdhani-Regular.ttf`
  - `ui/qml/resources/fonts/Rajdhani-Medium.ttf`
  - `ui/qml/resources/fonts/Rajdhani-SemiBold.ttf`
  - `ui/qml/resources/fonts/Rajdhani-Bold.ttf`
  - `ui/qml/resources/fonts/IBMPlexSans-Regular.ttf`
  - `ui/qml/resources/fonts/IBMPlexSans-Medium.ttf`
  - `ui/qml/resources/fonts/IBMPlexSans-SemiBold.ttf`
  - `ui/qml/resources/fonts/IBMPlexMono-Regular.ttf`
  - `ui/qml/resources/fonts/IBMPlexMono-Medium.ttf`
- Foundation/components:
  - `ui/qml/components/foundation/GlassPanel.qml`
  - `ui/qml/components/foundation/GlassCard.qml`
  - `ui/qml/components/foundation/GlassOverlay.qml`
  - `ui/qml/components/foundation/TrafficLightWindowControls.qml`
  - `ui/qml/components/foundation/PrimaryInput.qml`
  - `ui/qml/components/foundation/DataTableRow.qml`
  - `ui/qml/components/foundation/StatusPill.qml`
  - `ui/qml/components/foundation/SectionHeader.qml`
  - `ui/qml/components/controls/GlowButton.qml`
  - `ui/qml/components/controls/IconButton.qml`
  - `ui/qml/components/cards/MetricCard.qml`
  - `ui/qml/components/cards/HealthRingLarge.qml`
  - `ui/qml/components/charts/LineChart.qml`
  - `ui/qml/components/charts/SparkLine.qml`
  - `ui/qml/components/feedback/ToastContainer.qml`
  - `ui/qml/components/feedback/ToastNotification.qml`
  - `ui/qml/components/feedback/EmptyState.qml`
  - `ui/qml/components/feedback/ErrorState.qml`
  - `ui/qml/components/feedback/LoadingState.qml`
  - `ui/qml/components/ai/ChatBubble.qml`
  - `ui/qml/components/ai/ContextChip.qml`
  - `ui/qml/components/ai/QuickActionChips.qml`
  - `ui/qml/components/ai/StreamingText.qml`
  - `ui/qml/components/ai/TypingIndicator.qml`
- Screens:
  - `ui/qml/screens/Dashboard.qml`
  - `ui/qml/screens/Charts.qml`
  - `ui/qml/screens/GameMode.qml`
  - `ui/qml/screens/RamOptimizer.qml`
  - `ui/qml/screens/Temps.qml`
  - `ui/qml/screens/ProcessManager.qml`
  - `ui/qml/screens/StorageAnalyzer.qml`
  - `ui/qml/screens/AiChat.qml`
  - `ui/qml/screens/StartupManager.qml`
  - `ui/qml/screens/DriverManager.qml`
  - `ui/qml/screens/NetworkMonitor.qml`
  - `ui/qml/screens/HealthHistory.qml`
  - `ui/qml/screens/SecurityScanner.qml`
  - `ui/qml/screens/Settings.qml`
  - `ui/qml/screens/Onboarding.qml`
- QtGraphicalEffects shims:
  - `ui/qml/QtGraphicalEffects/qmldir`
  - `ui/qml/QtGraphicalEffects/DropShadow.qml`
  - `ui/qml/QtGraphicalEffects/RectangularGlow.qml`

## 14.10 Scripts and docs
- `scripts/build.ps1`
- `scripts/install-deps.ps1`
- `scripts/test.ps1`
- `docs/ARCHITECTURE.md`
- `docs/AUDIT_REPORT_PHASE0.md`
- `docs/PRODUCT_DOCUMENT.md`

---

## 15) Canonical glossary

- `SystemCtrl`: `UiController` object exposed to QML.
- `PulseModel`: local adaptive AI response engine.
- `TelemetryCache`: in-memory ring buffer for last N snapshots.
- `OptimizationHistory`: CSV action journal used by UI/AI.
- `FeatureGate`: trial/pro entitlement state for screen access.
- `UiPrefs`: persisted UI/system toggles surfaced to QML.

