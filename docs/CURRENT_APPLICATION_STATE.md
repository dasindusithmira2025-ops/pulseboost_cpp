# PulseBoost AI - Current Application State

Last updated: May 12, 2026
Source of truth: current repository working tree, existing project documentation, CMake configuration, QML resources, C++ bridge headers, smoke-test results, and safety-test results.

## Executive Summary

PulseBoost AI is a Windows-native desktop system optimization and diagnostics application built with C++20, Qt 6 Quick/QML, and native Windows telemetry APIs. The application is no longer positioned as a Tauri/web-shell product; the canonical shipping UI is the Qt/QML desktop shell under `ui/qml/`, backed by C++ modules under `core/`, `modules/`, `ai/`, `data/`, and `ui_backend/`.

The current application has a broad native feature surface:

- live system telemetry and health scoring
- QML dashboard and multi-screen native shell
- process, startup, storage, driver, thermal, network, RAM, backup, audit, restore, game, optimization, and AI surfaces
- local AI diagnosis and action-routing pipeline
- safety policy descriptors, dry-run support, audit logging, confirmation rules, and restore/snapshot flows
- CLI/headless operations for scans, cleanup, QML validation, benchmark, safety policy, tweak, game, network, RAM, startup, snapshot, license, update, and AI operations
- CMake/Visual Studio build configuration and Qt deployment resources

The repo contains a working Release executable and test binary. The smoke test script completed successfully with strict JSON validation, and the `PulseBoostSafetyTests.exe` binary exited successfully. Phase 2.7 resolved the previously observed Qt/SQLite validation warnings by initializing Qt before CLI SQLite usage and cleaning up database connections correctly.

## Repository State

Current top-level structure:

```text
ai/                 Local AI agent pipeline and model components
ai_model/           Python training and ONNX export pipeline
app/                Application entry point and CLI command routing
archive/            Deprecated/reference material, including Tauri reference
build/              Local generated build output
build_run/          Deployed/run build output
common/             Shared Windows/WMI utilities
core/               Native telemetry, scanners, managers, daemon, benchmark, advisor
data/               Telemetry and optimization persistence code/data
design-reference/   Design reference material
docs/               Product, architecture, safety, release, privacy, handoff docs
include/            Public/internal C++ headers
modules/            Optimization and safety modules
scripts/            Dependency, build, test, and release validation scripts
tauri_ui/           Deprecated reference path, not canonical production UI
tests/              Native safety and cleanup tests
ui/                 Qt/QML application shell, screens, components, styles, fonts
ui_backend/         C++ to QML bridge and UI-facing controller classes
```

Current Git working tree note:

- Several QML/style files are modified in the working tree.
- `Replicate Existing Design.zip` is untracked at the repository root.
- This document is based on the current working tree and does not revert or normalize those existing changes.

## Product Direction

PulseBoost AI is aimed at power users, developers, and gamers who want a safer PC optimization tool than generic "booster" utilities. Its stated direction is:

- provide real telemetry rather than fake improvement claims
- make optimization actions explicit and auditable
- keep high-risk system changes behind confirmation and advanced/manual execution
- use AI as an advisory and explanation layer, not an unrestricted executor
- preserve proof of work through history, audit, snapshots, and reports

The current native product information architecture is organized around a simplified primary shell plus advanced tools. The main product areas include Home, Optimizations, Boost-Up, Games, AI, Backup, Settings, Action Center, Audit Log, Restore Center, and several specialist diagnostic screens.

## Technology Stack

### Runtime Platform

- Windows 10/11 target
- Visual Studio 2022 toolchain
- C++20
- Qt 6.6.x with Qt Quick, Qt Quick Controls 2, Qt SQL, Qt Concurrent, and Qt Network
- Win32, PDH, WMI, registry, service, process, shell, restore-point, networking, and system APIs

### Build System

- CMake minimum version: 3.20
- CMake project name/version: `PulseBoostAI` version `1.0.0`
- Main executable target: `PulseBoost`
- Output executable name: `PulseBoostAI.exe`
- Safety test target: `PulseBoostSafetyTests`
- Optional ONNX Runtime integration through `ONNXRUNTIME_DIR`

### UI

- Qt Quick/QML production UI
- `Basic` Qt Quick Controls style
- Direct3D 11 scene graph selection in GUI mode
- QML resources embedded through `qml.qrc`
- Custom fonts embedded:
  - Rajdhani
  - IBM Plex Sans
  - IBM Plex Mono

### Data Storage

The app uses local storage rather than a remote service:

- telemetry CSV logs
- optimization history CSV logs
- QSettings/INI configuration
- local snapshots
- quarantine directory
- SQLite-backed safety audit table for system-changing actions
- local AI/adaptive metadata where implemented

## Application Entry Point and Runtime Wiring

The main entry point is `app/main.cpp`. It handles both GUI startup and a large set of CLI/headless commands.

### GUI Startup

In GUI mode, the application:

1. Initializes Qt application metadata.
2. Sets QSettings to INI format.
3. Loads embedded font resources.
4. Constructs core services:
   - `ProcessManager`
   - `ServiceManager`
   - `RegistryOptimizer`
   - `StartupOptimizer`
   - `MemoryAnalyzer`
   - `DiskAnalyzer`
   - `SystemScanner`
   - `TelemetryLogger`
   - `OptimizationHistory`
   - `JunkCleaner`
   - `GameMode`
   - `DeveloperMode`
   - `SafetyGuard`
   - `TelemetryCache`
   - `NetworkOptimizer`
   - `TweakEngine`
   - `PulseBench`
   - `SystemAdvisor`
   - `GameOptimizer`
   - `LicenseManager`
   - `FeatureGate`
   - `UiPreferences`
   - `AutoUpdater`
5. Checks for pending crash reports.
6. Checks for updates.
7. Constructs `UiController`.
8. Constructs `AgentEngine`.
9. Starts `TelemetryEngine`.
10. Registers QML context properties:
    - `SystemCtrl`
    - `AgentEngine`
    - `FeatureGate`
    - `UiPrefs`
    - startup crash/update flags and details
11. Loads `qrc:/ui/qml/main.qml`.

### Telemetry Flow

Telemetry is designed to avoid blocking the QML render thread:

1. `TelemetryEngine` runs scanner work on a worker thread.
2. Snapshots are emitted back to the UI bridge.
3. `TelemetryCache` stores rolling history.
4. `UiController` exposes the latest state and derived data to QML.
5. `TelemetryLogger` appends telemetry records.
6. `AgentEngine` receives the latest snapshot for local AI reasoning.

## Native UI State

The canonical QML root is:

```text
ui/qml/main.qml
```

It creates `AppWindow` with:

- default size: 1360 x 860
- minimum size: 1024 x 680
- title: `PulseBoost AI`

The main shell is composed from:

- `ui/qml/layout/AppWindow.qml`
- `ui/qml/layout/Sidebar.qml`
- `ui/qml/layout/TopBar.qml`
- `ui/qml/layout/ContentArea.qml`
- `ui/qml/layout/StatusBar.qml`

The sidebar drives the current screen, and `ContentArea.qml` uses a QML `Loader` to render the selected screen.

## QML Screens Included in the Production Resource File

The current `qml.qrc` includes these screens:

- `Home.qml`
- `ActionCenter.qml`
- `Optimizations.qml`
- `AuditLog.qml`
- `RestoreCenter.qml`
- `BoostUp.qml`
- `Games.qml`
- `Backup.qml`
- `Tools.qml`
- `Dashboard.qml`
- `Charts.qml`
- `GameMode.qml`
- `RamOptimizer.qml`
- `Temps.qml`
- `ProcessManager.qml`
- `StorageAnalyzer.qml`
- `AiChat.qml`
- `StartupManager.qml`
- `DriverManager.qml`
- `NetworkMonitor.qml`
- `HealthHistory.qml`
- `SecurityScanner.qml`
- `Settings.qml`
- `Onboarding.qml`

This means the native shell currently contains both the newer simplified product surfaces and deeper specialist utilities. The advanced/specialist tools are retained in the resource graph even though product direction prefers a simpler top-level nav.

## QML Component System

The UI resource file includes foundation, card, chart, feedback, AI, and legacy/shared components.

Foundation and controls:

- `GlowButton.qml`
- `IconButton.qml`
- `GlassPanel.qml`
- `GlassCard.qml`
- `GlassOverlay.qml`
- `TrafficLightWindowControls.qml`
- `PrimaryInput.qml`
- `DataTableRow.qml`
- `StatusPill.qml`
- `SectionHeader.qml`

Cards and charts:

- `MetricCard.qml`
- `HealthRingLarge.qml`
- `LineChart.qml`
- `SparkLine.qml`

Feedback:

- `ToastContainer.qml`
- `ToastNotification.qml`
- `EmptyState.qml`
- `ErrorState.qml`
- `LoadingState.qml`

AI:

- `StreamingText.qml`
- `ChatBubble.qml`
- `TypingIndicator.qml`
- `QuickActionChips.qml`
- `ContextChip.qml`

Other app components:

- `AppCard.qml`
- `AppSidebar.qml`
- `SidebarItem.qml`
- `TopStatusBar.qml`
- `HealthScoreRing.qml`
- `RiskBadge.qml`
- `ActionCard.qml`
- `ActionDetailPanel.qml`
- `ConfirmationDialog.qml`
- `DryRunResultDialog.qml`
- `AuditTable.qml`
- `AuditDetailDrawer.qml`
- `RestoreCard.qml`
- `ProofComparisonCard.qml`
- `AiRecommendationCard.qml`

## C++ to QML Bridge

The primary bridge is `UiController` in:

```text
include/PulseBoostAI/ui_backend/ui_controller.hpp
ui_backend/ui_controller.cpp
```

### Exposed QML Properties

The current bridge exposes:

- live metrics: CPU, RAM, disk, GPU, network
- health score and sub-scores
- 24-hour health forecast
- summary text and startup count
- chart series and thermal series
- process list
- driver list and driver summary
- storage categories
- recent actions and saved-today metric
- network, memory, and thermal overview maps
- recoverable RAM estimate
- notification center entries
- game mode status
- system snapshots
- pulse score and latest benchmark delta
- advisor items
- optimization presets
- detected games
- action-center actions
- latest proof report
- audit log entries
- restore-center items
- AI mode and cloud configuration state
- UI readiness, UI error message, and telemetry age

### Exposed QML Actions

The current bridge exposes actions for:

- safe cleanup
- basic optimization
- enabling/disabling game mode
- killing or suspending a process
- creating restore points
- sorting process lists
- rendering storage treemap data
- finding large files
- flushing DNS
- optimizing TCP
- RAM optimization
- disk optimization
- latency checks
- startup item fetch/disable/delay
- chart CSV export
- taking and restoring system snapshots
- scheduled optimization list/update
- tweak list/apply/revert
- optimization preset application
- detected-game optimization and optimized game launch
- game optimization revert
- optimization dry-runs and confirmed execution
- audit log refresh
- AI preference updates
- full UI cache refresh

## Core Native Feature State

### Telemetry and System Scanner

The scanner model includes:

- CPU usage
- RAM usage, used MB, total MB
- disk usage, used GB, total GB, read/write throughput
- GPU usage
- network throughput
- CPU/GPU/drive temperatures
- fan RPM
- CPU throttling state
- startup program count
- running service count
- health score
- CPU, memory, disk, network, process, thermal, boot, age, and security sub-scores
- 24-hour health forecast
- issues and human summary
- heavy process list
- startup items
- service list
- drivers
- storage categories

The telemetry cache supports rolling chart history used by charts, health history, thermal views, and AI context.

### Process Management

Process management supports:

- process enumeration
- CPU and memory attribution
- critical process marking
- process termination
- process suspension/resume support through CLI and bridge paths
- risk labeling for UI display

High-risk process actions are represented in the safety policy and require explicit treatment.

### Storage and Cleanup

Storage features include:

- disk analysis
- storage categories
- large-file scanning
- duplicate-file scanning
- safe junk estimation
- safe junk cleanup
- quarantine/recycle/permanent cleanup modes
- dry-run support

Cleanup defaults are safety-oriented. Permanent deletion is high risk and requires advanced confirmation.

### RAM

RAM features include:

- memory overview generation
- RAM breakdown
- working-set trim optimization
- recoverable RAM estimate
- RAM saver/flush standby CLI commands

The safety model describes RAM working-set trimming as manual/advanced and not a guaranteed speed boost.

### Network

Network features include:

- latency measurement
- DNS flush
- TCP optimization
- network diagnostics
- network tuning revert

Global network changes are high risk or critical depending on the operation. The safety model expects backup/confirmation for advanced network changes.

### Startup Management

Startup features include:

- startup item scanning
- disable startup item
- enable startup item through CLI
- delay startup item
- scheduled optimization tasks
- snapshot-based startup baseline restore

Startup flows are present both in CLI and QML surfaces.

### Game Optimization

Game features include:

- known-game detection using `data/common_games.json` with fallback executable names
- active-game candidate detection from process behavior
- basic game mode activation
- RAM, DNS, TCP, process priority, and power/session-related optimization paths
- optimized launch path
- revert optimization path
- game session status

Open work remains around deeper per-title orchestration, launcher-specific handling, and stronger auto-revert/session-completion behavior.

### Benchmark and Pulse Score

Benchmark features include:

- quick benchmark
- full benchmark
- benchmark history
- benchmark delta and pulse score exposure
- pulse grade calculation

Open work remains around polished proof-of-work storytelling and richer native benchmark presentation.

### Advisor

System advisor features include:

- advisor items derived from snapshot data and tweak definitions
- action labels/action IDs
- category, status, impact, and actionable metadata
- display in Home, AI, Games, and optimization-related views

Open work remains around deeper hardware and BIOS-level guidance where detection is feasible.

### Drivers, Thermals, Security

The current UI and bridge expose:

- driver inventory
- signed/current/outdated status labeling
- thermal overview
- thermal charts
- security score and scanner screen

Some security scanner UI data appears partly presentation-driven and should be validated before treating it as a complete security product.

## AI State

The AI system is local-first and integrated into the native app through `AgentEngine`.

Key components:

- `AgentEngine`
- `AiDiagnosticsEngine`
- `ChatRouter`
- `ContextBuilder`
- `PromptBuilder`
- `PulseModel`
- `SessionMemory`
- `ProactiveMonitor`
- `IntentClassifier`
- `ContextFusion`
- `ResponseGenerator`
- `AdaptiveEngine`
- `SystemPrediction`

Current capabilities include:

- receiving user questions from QML
- classifying user intent
- reading latest telemetry and history context
- producing diagnostics, recommendations, and action metadata
- streaming/chunk-style UI hooks
- collecting feedback and action-result metadata
- executing approved action IDs through controlled paths
- reporting total interactions and satisfaction rate
- resetting adaptive learning
- CLI `--chat` diagnostics path

The AI should be treated as advisory-first. High-risk actions must remain confirmed and safety-routed; AI should not silently execute high-risk or advanced optimizations.

## Safety Model

Safety policy is implemented in `modules/safety_policy.cpp` and shared across UI and CLI flows.

### Risk Levels

Supported risk levels:

- `read_only`
- `low`
- `moderate`
- `high`
- `critical`

### Descriptor Fields

Each safety descriptor includes:

- action ID
- risk level
- dry-run support
- confirmation requirement
- backup requirement
- rollback availability
- audit requirement
- advanced-only flag
- human summary

### Current Descriptor Coverage

The current descriptor map includes:

- `junk.clean`
- `junk.clean.permanent`
- `file.delete`
- `process.kill`
- `process.suspend`
- `startup.disable`
- `startup.enable`
- `startup.delay`
- `network.flush_dns`
- `network.optimize_tcp`
- `network.winsock_reset`
- `network.revert`
- `ram.trim_working_sets`
- `ram.flush_standby`
- `ram.saver`
- `disk.optimize`
- `restore.create`
- `snapshot.restore`
- `tweak.apply`
- `tweak.revert`
- `game.optimize`
- `game.revert`
- `schedule.task`

### Audit Model

System-changing actions are written to an SQLite table named `action_audit_log` with fields for:

- creation time
- action type
- status
- summary
- risk level
- dry-run flag
- request JSON
- result JSON

The Audit Log screen reads audit entries for demonstration and traceability.

## CLI and Headless State

The executable supports a broad CLI surface. Commands currently detected in `app/main.cpp` include:

- `--daemon`
- `--safety-policy`
- `--validate-qml`
- `--list-tweaks`
- `--apply-tweak`
- `--revert-tweak`
- `--apply-safe-tweaks`
- `--apply-high-impact-tweaks`
- `--revert-all-tweaks`
- `--quick-benchmark`
- `--full-benchmark`
- `--benchmark-history`
- `--system-advisor`
- `--detect-games`
- `--get-ai-preferences`
- `--set-ai-preferences`
- `--license-info`
- `--activate-license`
- `--check-for-updates`
- `--error-log`
- `--export-error-log`
- `--pulse-score`
- `--scan`
- `--status`
- `--snapshot-json`
- `--refresh-all`
- `--optimize-ram`
- `--flush-standby`
- `--enable-ram-saver`
- `--optimize-disk`
- `--flush-dns`
- `--optimize-tcp`
- `--revert-network`
- `--check-latency`
- `--network-diagnostics`
- `--ram-breakdown`
- `--open-file-location`
- `--delete-file`
- `--kill-pid`
- `--resume-pid`
- `--create-restore-point`
- `--enable-game-mode`
- `--optimize-game`
- `--launch-optimized-game`
- `--disable-game-mode`
- `--revert-game-optimization`
- `--game-mode-status`
- `--disable-startup-by-name`
- `--enable-startup-by-name`
- `--delay-startup-by-name`
- `--get-scheduled-tasks`
- `--set-scheduled-task`
- `--take-snapshot`
- `--restore-snapshot`
- `--recent-actions-json`
- `--scan-large-files`
- `--run-security-scan`
- `--find-duplicates`
- `--estimate-junk`
- `--clean`
- `--self-test`
- `--chat`

The daemon path also exposes a subset of JSON command handling in `core/backend_daemon.cpp`.

## Build and Test State

### Build Artifacts Present

The workspace contains a deployed Release executable at:

```text
build/Release/PulseBoostAI.exe
```

It also contains:

```text
build/Release/PulseBoostSafetyTests.exe
build_run/Release/PulseBoostAI.exe
```

### Tests Run During This Review

The following commands were run on May 12, 2026:

```powershell
.\scripts\test.ps1 -Configuration Release
.\build\Release\PulseBoostSafetyTests.exe
```

Results:

- Smoke tests completed with strict JSON validation.
- `PulseBoostSafetyTests.exe` exited successfully.

Smoke-test coverage from `scripts/test.ps1`:

- `--scan`
- `--chat analyze performance`
- `--clean --dry-run`
- `--self-test`
- `--validate-qml`

Validation warning status:

- No known Qt/SQLite validation warnings after Phase 2.7.
- The previous warnings were traced to CLI/headless commands using SQLite-backed safety/audit code before Qt application initialization and to SQLite connections being removed while query/database wrappers were still live.

### Existing Release Gates

The release checklist expects:

- Release build completion
- safety tests through CTest
- strict JSON CLI smoke tests
- QML production screens included in `qml.qrc`
- no deprecated Tauri paths in release resources
- no generated/runtime files committed
- Action Center dry-run support
- Audit Log SQLite loading
- Restore/Undo Center snapshots, quarantine inventory, and rollback entries
- AI assistant advisory-first behavior for high-risk actions

## Packaging and Deployment State

The project is designed for local Windows executable distribution.

Relevant deployment files:

- `PulseBoost_Installer.iss`
- `scripts/build.ps1`
- `scripts/release-validate.ps1`
- `scripts/install-deps.ps1`
- `qml.qrc`
- CMake presets

Qt deployment is expected through `windeployqt` or the provided release process. The current release directory contains Qt DLLs, plugins, QML deployment folders, and `PulseBoostAI.exe`.

## Privacy and Data Handling State

The documented privacy model is local-first:

- telemetry remains local by default
- action history remains local
- audit logs remain local
- snapshots and proof reports remain local
- quarantined cleanup files remain local
- cloud AI mode is optional
- API keys and secrets must not be committed or packaged

If cloud AI mode is configured, prompts may include system-health context. This should remain explicit in UI copy and settings.

## Deprecated and Non-Canonical Areas

The repository still contains:

- `tauri_ui/`
- `archive/deprecated/tauri-reference/`
- design reference ZIP files

Current docs state that Qt/QML is canonical and Tauri is deprecated/reference-only. Before a production release, the packaging path should continue to exclude deprecated Tauri surfaces and generated reference assets.

## Strengths

Current strengths:

- Native Qt/QML shell is broad and resource-linked.
- Telemetry is separated from UI rendering through a worker/threaded model.
- QML has many production surfaces, not only a dashboard prototype.
- C++ bridge exposes a large and practical UI API.
- Safety policy is explicit and testable.
- CLI/headless surface is broad enough for automation and release validation.
- Local-first privacy posture is clear.
- AI is integrated with telemetry and controlled action paths rather than arbitrary command execution.
- Release artifacts and smoke-test scripts already exist.

## Known Gaps and Risks

### Release Validation Warnings

Resolved in Phase 2.7:

- CLI/headless commands now create a `QCoreApplication` before safety/audit SQLite code can run.
- SQLite audit readers/writers now destroy query handles before closing and removing named database connections.
- The previous QSQLITE driver, `QCoreApplication`, null QObject connect, and connection-in-use warnings are no longer known active validation warnings.

### Uncommitted UI Work

Several UI files are modified in the working tree. The current source may not exactly match any previously committed release baseline. Before packaging, review and either commit or intentionally discard those changes.

### Tweak Catalog Depth

The tweak engine is present and wired, but the documented gap remains: the real tweak catalog is still more of a first production slice than the final desired coverage.

### Game Optimization Depth

The game flow is visible and wired, but deeper per-title handling, launcher-specific behavior, and stronger auto-revert/session completion still need work.

### Benchmark Presentation

Benchmark execution and score exposure exist. The native proof/storytelling layer can be improved so users understand before/after value without overclaiming.

### Advisor Depth

Advisor content exists, but deeper hardware, BIOS, XMP, Resizable BAR, DirectStorage, and similar guidance remains dependent on feasible detection work.

### Proof and Restore Depth

Backup, snapshots, audit, and restore flows exist. Remaining polish includes richer persisted history, restore/report visualization, and cleanup of generated runtime state before shipping.

### AI Parity and Metadata

The native AI screen has streaming and action metadata hooks. Richer action metadata, chat-stream parity, and clearer cloud/local configuration UX can still be improved.

### Settings Consolidation

Settings currently covers AI preferences and app configuration surfaces. Further convergence of update, license, diagnostics, schedule, privacy, and advanced controls would reduce fragmentation.

### Specialist Screens Need Product Review

The resource file includes many specialist screens. Some may be complete enough for advanced tools; others may be demo/prototype-like. Before release, each screen should be classified as:

- production-ready top-level
- production-ready advanced
- demo-only
- hidden until hardened

### Generated and Runtime Files

The workspace contains build output, logs, runtime CSVs, and local generated state. This is expected locally but should not be included in release commits or packages unless explicitly intended.

## Recommended Next Steps

1. Re-run `scripts/release-validate.ps1 -Configuration Release` and capture a clean result.
2. Investigate and remove the Qt/SQLite warnings seen during smoke validation.
3. Review all modified QML/style files and decide the intended commit scope.
4. Verify the current source builds from a clean configure/build, not only from existing build artifacts.
5. Review every QML screen in `qml.qrc` and mark release visibility.
6. Confirm Tauri/reference paths are excluded from production packaging.
7. Harden Action Center, Audit Log, Restore Center, and proof-report flows because these are central to the safety story.
8. Expand tests around safety policy, cleanup modes, startup restore, network advanced actions, and game optimization revert.
9. Audit settings and privacy flows for cloud AI key handling.
10. Prepare a final release package only after the validation warnings are resolved.

## Bottom Line

PulseBoost AI is currently a substantial native Windows Qt/QML application with real C++ telemetry, optimization modules, safety policy, local AI diagnostics, CLI operations, and a wide QML UI surface. It is past a simple prototype stage and has passing smoke/safety validation in the current workspace.

The main remaining work is release hardening: clean validation output, packaging hygiene, UI scope control, deeper proof/restore polish, and tightening areas where the product surface is broader than the fully hardened implementation.
