# PulseBoost AI Architecture

## Overview

PulseBoost AI is a Windows desktop optimizer built with C++20, Win32/WMI telemetry, and a Qt Quick QML interface. The application is organized around four layers:

- `core/` gathers telemetry and system state through Win32, PDH, WMI, and the registry.
- `modules/` contains explicit optimization actions such as cleanup, game mode, and startup analysis.
- `ai/` turns telemetry plus action history into diagnostic prompts, plans, and LLM requests.
- `ui_backend/` and `ui/qml/` provide the presentation layer without letting telemetry work block rendering.

The current application preserves the existing core functionality while moving the UI to a smoother QML shell and keeping telemetry work off the GUI thread.

## Runtime model

### GUI thread

The main thread owns:

- `QQmlApplicationEngine`
- `UiController`
- `AgentEngine`
- the QML scene graph and controls

This thread is only responsible for rendering, user interaction, and dispatching actions.

### Telemetry thread

`TelemetryEngine` creates a dedicated worker thread with three timers:

- every 1 second: CPU, RAM, GPU, and network vitals
- every 3 seconds: disk usage and storage category refresh
- every 5 seconds: process, startup, and service refresh

The worker emits `snapshotReady(SystemSnapshot)` back to the GUI thread through queued connections. No PDH, WMI, or process enumeration work is performed on the UI thread.

## Data flow

### Telemetry pipeline

1. `TelemetryWorker` samples a partial or full `SystemSnapshot`.
2. `SystemScanner` enriches the snapshot with the requested subsystem data.
3. `TelemetryCache` stores the rolling history for charts and AI context.
4. `UiController` exposes the latest state to QML through `Q_PROPERTY`.
5. `TelemetryLogger` persists samples to CSV.

### AI pipeline

1. The user submits a message from `ChatPanel.qml`.
2. `AgentEngine::ask()` reads the latest snapshot and history window from `TelemetryCache`.
3. `ChatRouter` classifies the message and builds a candidate action plan.
4. `PromptBuilder` formats telemetry, issue summary, process pressure, storage pressure, and plan context.
5. `LlmClient` sends the request asynchronously to the configured endpoint.
6. If the endpoint is unavailable, `LlmClient::offlineFallback()` returns a telemetry-based diagnostic response.
7. For safe action intents such as cleanup or restore-point-backed optimization, `AgentEngine` can execute modules directly before returning the reply.

## Main components

### `core/system_scanner.*`

Produces `SystemSnapshot` values. The scanner is split into focused stages:

- `scanVitals()` for CPU, RAM, GPU, network, and top memory consumers
- `enrichStorage()` for disk saturation and storage categories
- `enrichProcesses()` for startup items and services
- `enrichDrivers()` for driver inventory
- `finalizeSnapshot()` for issue detection, health score, and human summary text

### `core/telemetry_engine.*`

Runs the worker thread and timer cadence. It merges partial scans into a consistent snapshot so the UI updates smoothly without rescanning every subsystem on every tick.

### `ui_backend/ui_controller.*`

Acts as the QML bridge. It exposes:

- live metrics
- rolling chart samples
- sorted process lists
- storage treemap nodes
- recent optimization history
- safe optimization actions

### `ai/agent_engine.*`

Coordinates the AI experience. It:

- consumes live telemetry
- routes commands
- creates restore points before higher-impact actions
- triggers safe modules for direct requests
- builds rich prompts for LLM-backed reasoning

### `data/telemetry_cache.*`

Stores recent snapshots in a thread-safe ring buffer. The UI charts and AI prompt builder both read from this cache.

## QML surface

The QML shell is organized into focused pages:

- `MainWindow.qml`: application shell, navigation, toast feedback
- `Dashboard.qml`: health summary, quick actions, recent history, storage preview
- `Charts.qml`: rolling telemetry charts backed by cached samples
- `ProcessTable.qml`: sortable heavy-process view with termination controls
- `StorageMap.qml`: treemap-style storage overview
- `ChatPanel.qml`: embedded AI assistant

The UI uses a custom dark palette, animated transitions, and Qt Quick rendering with the `Basic` control style to avoid native-style customization limits.

## Safety model

PulseBoost AI does not apply broad or unsafe registry and service changes blindly.

Before destructive or higher-risk actions, the system can:

- create a Windows restore point through `SafetyGuard`
- back up targeted registry keys
- append optimization history and safety logs

Cleanup is restricted to safe disposable paths in the current implementation.

## Build and toolchain

The project is configured with CMake and currently targets:

- Visual Studio 2022
- Windows 10/11 SDK
- Qt 6.6.x MSVC runtime

Primary Qt dependencies:

- `Qt6::Core`
- `Qt6::Gui`
- `Qt6::Quick`
- `Qt6::QuickControls2`
- `Qt6::Network`
- `Qt6::Concurrent`

## Validation status

The current refactor has been validated with:

- successful Release build through `scripts/build.ps1`
- clean QML startup with no runtime warnings in `logs/gui_runtime.log`
- CLI smoke tests through `scripts/test.ps1`

The Vulkan wrapper warning from Qt discovery is non-blocking for this Qt Quick desktop application.
