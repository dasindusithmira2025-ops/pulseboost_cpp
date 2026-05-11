# PulseBoost AI - Entire Product Dump

Last updated: April 3, 2026  
Repository root: `d:\HelaDev\Finished And ongoing freelancing Projects\Pulseboost`  
Canonical desktop shell: `ui/qml/` + Qt/C++ app shell  
Deprecated reference shell: `tauri_ui/`  
Product direction: gaming-first Windows optimizer with native telemetry, reversible optimization, local AI guidance, cleanup, benchmarking, backup/recovery, and advanced controls

---

## 1. What this document is

This is the current system-level dump of the product as it exists in source after the native Qt/QML convergence pass.

It answers:
- what the shipping product is
- which shell is canonical
- what screens and flows exist in the native app
- which backend layers power the product
- where Tauri now sits in the repo

This is not a file-by-file source transcript.

---

## 2. Product identity

PulseBoost AI is a Windows optimization product centered on:
- real-time system telemetry
- safe, reversible actions
- gaming-focused optimization flows
- local AI guidance with optional cloud preference storage
- benchmark and proof-of-work framing
- snapshot, restore, and recovery safety

The repo still contains two UI shells:
- native Qt/QML in `ui/qml/`
- Tauri + React + TypeScript in `tauri_ui/`

Current product direction in source:
- Qt/QML is the active shipping shell
- Tauri remains in-repo as a deprecated reference surface

---

## 3. Top-level architecture

PulseBoost is currently organized into five practical layers.

### 3.1 Native GUI entrypoint

Main path:
- `app/main.cpp`

Responsibilities:
- launches the Qt/QML app path by default
- wires controllers, engines, telemetry, updater, AI, and product modules into the GUI
- still exposes CLI and daemon modes for non-GUI/backend operations

### 3.2 Qt/QML presentation shell

Main paths:
- `ui/qml/`
- `ui_backend/`

Responsibilities:
- renders the shipping product shell
- owns the 7-screen product IA
- binds native C++ controller APIs into QML
- keeps heavy logic out of QML and in C++ services/controllers

### 3.3 Native execution core

Main folders:
- `core/`
- `modules/`
- `ai/`
- `data/`
- `common/`

Responsibilities:
- telemetry and scanning
- cleanup, tuning, benchmarking, gaming, startup, process, storage, network, and safety workflows
- AI reasoning and chat orchestration
- history, restore, license, updater, and diagnostics persistence

### 3.4 Native UI controller layer

Main paths:
- `include/PulseBoostAI/ui_backend/ui_controller.hpp`
- `ui_backend/ui_controller.cpp`

Responsibilities:
- exposes C++ product state into QML
- provides invokable product actions for the native shell
- surfaces summaries, presets, detected games, AI preferences, advisor items, proof-of-work, and advanced tool access

### 3.5 Deprecated Tauri reference path

Main paths:
- `tauri_ui/src/`
- `tauri_ui/src-tauri/src/main.rs`

Responsibilities now:
- reference implementation only
- secondary surface for comparison and retained experimentation
- not the release UX target

---

## 4. Current shipping information architecture

The native Qt/QML shell now uses 7 top-level destinations:

1. `Home`
2. `Optimizations`
3. `Boost-Up`
4. `Games`
5. `AI`
6. `Backup`
7. `Settings`

Advanced specialist surfaces are no longer top-level destinations. They are retained under:
- `Settings > Advanced`

The old tool-console style navigation is no longer the primary product IA.

---

## 5. Native shell structure

Main files:
- `ui/qml/layout/AppWindow.qml`
- `ui/qml/layout/Sidebar.qml`
- `ui/qml/layout/TopBar.qml`
- `ui/qml/layout/ContentArea.qml`
- `ui/qml/layout/StatusBar.qml`

### Responsibilities

`AppWindow.qml`
- composes the main native shell
- hosts sidebar, top bar, content area, status bar, and shared overlays

`Sidebar.qml`
- renders the 7-item product navigation
- shows condensed health / shell state
- removes the old specialist tool stack from top-level nav

`TopBar.qml`
- exposes app identity, active context, search, refresh, notifications, and window controls
- is intentionally lighter than the older heavy shell chrome

`ContentArea.qml`
- mounts the active route screen
- shows loading and error feedback for screen-level load issues

`StatusBar.qml`
- shows condensed runtime/product state

---

## 6. Native screen inventory

### 6.1 Home

File:
- `ui/qml/screens/Home.qml`

Purpose:
- answer what matters right now

Core content:
- PulseScore hero
- live vitals summary
- advisor summary
- quick actions
- benchmark delta summary
- recent actions
- compact AI guidance framing

### 6.2 Optimizations

File:
- `ui/qml/screens/Optimizations.qml`

Purpose:
- primary safe optimization workflow

Core content:
- preset-first workflow
- tweak browser below presets
- batch apply/revert actions
- restart awareness
- restore-first framing for bulk operations

### 6.3 Boost-Up

File:
- `ui/qml/screens/BoostUp.qml`

Purpose:
- cleanup and pre-launch prep workflow

Core content:
- junk cleanup
- RAM cleanup / standby flush
- network refresh
- repair / maintenance actions
- pre-game clean flow
- proof-of-work summaries

### 6.4 Games

File:
- `ui/qml/screens/Games.qml`

Purpose:
- detect, optimize, launch, and revert game sessions

Core content:
- detected games
- optimize / launch-and-optimize / revert actions
- gaming-only advisor context
- RAM / thermal / network context for gaming

### 6.5 AI

File:
- `ui/qml/screens/AiChat.qml`

Purpose:
- context-aware guidance and AI conversation

Core content:
- local/cloud mode state
- chat conversation UI
- guided prompt framing
- native controller-backed AI preferences

### 6.6 Backup

File:
- `ui/qml/screens/Backup.qml`

Purpose:
- restore, snapshot, history, and export workflow

Core content:
- history view
- snapshots
- restore / export actions
- revert and recovery framing

### 6.7 Settings

File:
- `ui/qml/screens/Settings.qml`

Purpose:
- product and advanced configuration

Core content:
- license
- updates
- AI preferences
- scheduled tasks / diagnostics
- advanced tools host
- explicit note that Tauri is a deprecated reference surface

### 6.8 Advanced tools retained in native shell

Files retained and hosted from Settings > Advanced:
- `ui/qml/screens/ProcessManager.qml`
- `ui/qml/screens/StorageAnalyzer.qml`
- `ui/qml/screens/NetworkMonitor.qml`
- `ui/qml/screens/StartupManager.qml`
- `ui/qml/screens/RamOptimizer.qml`
- `ui/qml/screens/Temps.qml`

These are no longer top-level destinations.

---

## 7. Shared native components and style

### Layout / shell
- `ui/qml/layout/*`

### Foundation / controls
- `ui/qml/components/foundation/*`
- `ui/qml/components/controls/*`
- `ui/qml/components/cards/*`
- `ui/qml/components/feedback/*`
- `ui/qml/components/ai/*`
- `ui/qml/components/charts/*`

### Style system
- `ui/qml/style/Style.qml`
- `ui/qml/style/Animations.qml`
- `ui/qml/style/Icons.js`

Current native styling direction:
- flatter, firmer panels
- reduced glow and reduced ambient motion
- fewer always-running effects
- stronger contrast and lower UI cost than the prior heavy shells

---

## 8. Native controller contract

Main native bridge:
- `UiController`

Primary responsibilities now include:
- telemetry / readiness state for QML
- pulse score and benchmark delta
- advisor summary items
- optimization preset list and application
- tweak listing/apply/revert
- detected games and game-session control
- AI mode / AI cloud configuration state
- settings and diagnostics surfaces for the native shell

The native shell does not reimplement business logic in QML. Product logic remains in C++ services and is surfaced through controller APIs.

---

## 9. Native execution core map

### Telemetry and scanning
- `core/system_scanner.cpp`
- `core/telemetry_engine.cpp`
- `data/telemetry_cache.cpp`
- `data/telemetry_logger.cpp`

### Core execution services
- `core/process_manager.cpp`
- `core/memory_analyzer.cpp`
- `core/disk_analyzer.cpp`
- `core/service_manager.cpp`
- `core/startup_optimizer.cpp`
- `core/registry_optimizer.cpp`

### Product engines
- `core/pulse_bench.cpp`
- `core/game_optimizer.cpp`
- `core/system_advisor.cpp`
- `modules/tweak_engine.cpp`
- `modules/network_optimizer.cpp`
- `modules/ram_optimizer.cpp`
- `modules/junk_cleaner.cpp`
- `modules/game_mode.cpp`
- `modules/safety_guard.cpp`

### Product infrastructure
- `core/license_manager.cpp`
- `core/auto_updater.cpp`
- `core/crash_reporter.cpp`
- `core/backend_daemon.cpp`

### AI stack
- `ai/agent_engine.cpp`
- `ai/pulse_model.cpp`
- `ai/ai_diagnostics_engine.cpp`
- `ai/chat_router.cpp`
- `ai/prompt_builder.cpp`
- `ai/context_builder.cpp`
- `ai/model/*`

---

## 10. AI product behavior

Primary native entry:
- `ui/qml/screens/AiChat.qml`

Backend path:
- `ai/agent_engine.cpp`
- `ai/pulse_model.cpp`
- `app/main.cpp`
- optional daemon / CLI chat paths still present in native backend

Current AI capabilities:
- local AI response path
- context-aware diagnostics and optimization guidance
- stored local/cloud preference state
- integration with native product context

---

## 11. Data and persistence

Runtime and history artifacts include:
- `data/pulsemodel_adaptive.sqlite`
- `data/pulsebench_history.jsonl`
- `data/tauri_snapshots/*.json`
- telemetry history through `data/telemetry_logger.cpp`
- optimization history through `data/optimization_history.cpp`
- error / update / license / AI preference persistence through native backend logic

---

## 12. Build and run

### Native release build
```powershell
cmake --build build_run --config Release
```

### Convenience script
```powershell
.\scripts\build.ps1 -QtPath "C:\Qt\6.6.3\msvc2019_64"
```

### Launch shipping shell
```powershell
.\build_run\Release\PulseBoostAI.exe
```

### Deprecated Tauri reference build
```powershell
cd tauri_ui
npm run build
cd src-tauri
cargo build --release
```

### Main executable paths
- Native shipping app: `build_run\Release\PulseBoostAI.exe`
- Deprecated Tauri app: `tauri_ui\src-tauri\target\release\pulseboost_tauri_ui.exe`

---

## 13. Current source-of-truth files

Most important files for the shipping product now are:
- `app/main.cpp`
- `include/PulseBoostAI/ui_backend/ui_controller.hpp`
- `ui_backend/ui_controller.cpp`
- `ui/qml/layout/AppWindow.qml`
- `ui/qml/layout/Sidebar.qml`
- `ui/qml/layout/ContentArea.qml`
- `ui/qml/screens/Home.qml`
- `ui/qml/screens/Optimizations.qml`
- `ui/qml/screens/BoostUp.qml`
- `ui/qml/screens/Games.qml`
- `ui/qml/screens/AiChat.qml`
- `ui/qml/screens/Backup.qml`
- `ui/qml/screens/Settings.qml`
- `ui/qml/style/Style.qml`
- `core/game_optimizer.cpp`
- `core/pulse_bench.cpp`
- `core/system_advisor.cpp`
- `modules/tweak_engine.cpp`

---

## 14. Tauri status

Tauri remains in the repo as a deprecated reference surface.

It still contains:
- a React product shell
- a Rust bridge
- retained screen implementations and bridge logic

But it is no longer the release UI target.

---

## 15. Remaining gaps

For the current unresolved backlog, use:
- `GAPS.md`

Broad remaining areas still include:
- deeper tweak catalog coverage
- broader game-specific optimization depth
- richer proof-of-work summaries and benchmark presentation
- additional native-shell convergence of secondary flows
- final deprecation hardening around old reference surfaces

---

## 16. One-line screen pitches

- `Home`: See what matters right now.
- `Optimizations`: Apply safe, reversible improvements.
- `Boost-Up`: Clean and prep before gaming or heavy work.
- `Games`: Detect, optimize, launch, and revert game sessions.
- `AI`: Ask PulseBoost AI for guided help.
- `Backup`: Capture, restore, and export recovery state.
- `Settings`: Control product behavior, license, updates, schedules, diagnostics, and advanced tools.
