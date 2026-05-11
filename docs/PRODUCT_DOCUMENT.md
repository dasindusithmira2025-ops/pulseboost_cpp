# PulseBoost AI

## What It Is

PulseBoost AI is a modern, production-grade Windows-native desktop application designed for system optimization and diagnostics. It is built for power users, developers, and gamers who want to safely monitor and tune their PCs. Unlike traditional "RAM booster" snake oil, PulseBoost AI relies on genuine Windows telemetry (CPU, RAM, usage queues, disk I/O) and integrates a local AI reasoning engine that analyzes this data to provide contextual, safe, and effective system optimizations.

## Tech Stack

- **Frontend / UI**: Qt 6.6 (Qt Quick, QML, QtQuickControls2), leveraging pure QML Canvas for GPU-accelerated charts instead of heavy charting libraries.
- **Backend**: ISO C++20, utilizing native Windows APIs (PDH, WMI, Process API, Registry API, srrestoreptapi).
- **Concurrency**: `QThread`, `QTimer`, and `QtConcurrent` for strictly separating telemetry polling from the UI rendering thread.
- **AI Integration**: Asynchronous HTTP client via `QNetworkAccessManager` connecting to a local Ollama LLM endpoint (default: `http://127.0.0.1:11434/api/generate` with the `llama3` model) or any OpenAI-compatible remote endpoint.
- **Database / Storage**: Local flat files for logging (CSV formats for telemetry and optimization history) and `QSettings` (INI format) for application configuration. No external SQL/NoSQL databases.
- **Build System**: CMake 3.20+ generating Visual Studio 2022 solutions.

## Full Feature List

- **Background Telemetry Engine**: Asynchronous, non-blocking hardware polling for:
  - CPU usage (1-second intervals via PDH)
  - RAM consumption and available bounds
  - Active Processes and their metric breakdown (5-second intervals)
  - Disk I/O and storage volume utilization (3-second intervals)
- **Interactive QML Dashboard**:
  - Real-time System Health Score ring.
  - Granular telemetry cards displaying live percentages.
  - Action buttons triggering system cleanups or boosts.
- **GPU-Accelerated Charts Engine**: Rolling 60-second window visualizations for CPU, RAM, and Disk using custom QML Canvas gradient rendering.
- **Live Process Manager**: Sortable process list displaying CPU/RAM footprint with the ability to safely kill non-critical processes.
- **Drive Storage Treemap**: Proportional visual representation of disk usage categories (System, Apps, Games, Temp, etc.).
- **PulseBoost AI Agent Chat**:
  - Context-aware chat panel utilizing live telemetry snapshots to answer system performance questions (e.g., "Why is my PC slow right now?").
  - Capability for the AI to proactively execute predefined safe system routines based on user intent (Router).
  - Auto-fallback to offline heuristic diagnostics if the LLM backend is out of reach.
- **Safe Remediation Modules**:
  - **Junk Cleaner**: Identifies and clears temporary system files and caches.
  - **Game Mode**: Modifies process priorities and suspends background services to free up resources for gaming.
  - **Developer Mode**: Auto-tunes processes for compilation/build toolchains.
- **Safety Guard System**:
  - Integrates with Windows System Restore to silently create restore points before high-impact changes.
  - Dumps registry key backups before editing critical hives.
  - A definitive safety action audit trail written to `logs/safety.log`.
- **Headless CLI Support**: Allows triggering scans (`--scan`), cleaning (`--clean`), or diagnostics (`--self-test`) via command line without spinning up the Qt GUI.

## How It Works (User Flow)

1. **Launch**: The user opens `PulseBoost.exe`. The Qt GUI loads instantly while the C++ `TelemetryWorker` boots up on a background thread.
2. **Monitoring**: The user observes their live system metrics on the Dashboard and inspects historical spikes in the Charts view.
3. **Analysis**: Discovering high RAM usage, the user navigates to the Process Table. They can sort by RAM and manually kill offending background tasks.
4. **AI Diagnosis**: Alternatively, the user utilizes the AI Chat panel, typing "My system feels sluggish while gaming".
5. **Prompt Contextualization**: The C++ `AgentEngine` captures the current system snapshot (which reveals elevated background CPU from a browser) and appends it to the user's prompt invisibly.
6. **Execution**: The local LLM processes the data, suggests closing the browser, and independently triggers the "Game Mode" module.
7. **Safety Catch**: The application creates a Windows Restore Point.
8. **Optimization**: Process priorities are adjusted, temp files are cleared, and the user experiences an immediate performance recovery, with all actions logged securely.

## API Reference

_Note: As a standalone desktop application, PulseBoost AI does not expose external REST APIs. Below is the internal bridged API surface._

### QML ↔ C++ Bridge (`UiController`)

- `Q_PROPERTY(double cpuUsage)` / `ramUsage` / `diskUsage` / `healthScore`: Read-only telemetry bindings.
- `Q_INVOKABLE void runClean()`: Triggers the Junk Cleaner module.
- `Q_INVOKABLE void runOptimize()`: Triggers basic system optimization.
- `Q_INVOKABLE void runGameBoost()`: Triggers Game Mode process reprioritization.
- `Q_INVOKABLE void killProcess(int pid)`: Force kills a Windows process by ID via Win32 API.
- `Q_INVOKABLE void createRestorePoint()`: Forces a manual system restore point.

### Local LLM Outbound Calls (`LlmClient`)

- **Endpoint**: `POST http://127.0.0.1:11434/api/generate` (Configurable via `PulseBoost.ini`)
- **Payload**: JSON containing `{"model": "llama3", "prompt": "<context + user_input>", "stream": false}`
- **Response Handling**: Parses the `response` key from the returning JSON. Emits `agentReply(text, actionTaken)` to the QML UI.

## Database Schema

_Note: Uses flat local files rather than SQL. Defined schemas are as follows:_

**`telemetry.csv`** (Append-only Log):
`Timestamp (UTC)`, `CPU %`, `RAM %`, `Disk %`, `Health Score`

**`optimization_history.csv`** (Append-only Log):
`Timestamp (UTC)`, `Action Type (e.g., cli-clean)`, `Details (e.g., Recovered X bytes)`, `Success (Bool)`

**`safety.log`** (Append-only Log):
`Timestamp (UTC)`, `Module Trigger`, `Status (OK/FAIL)`, `Details/Description/PID`

## Current State

- **Fully Built & Working**:
  - CMake build pipeline generating visual studio solutions.
  - Completely detached background telemetry system successfully passing real-time snapshots stringently enforcing thread-safety.
  - Entire QML UI suite integrating custom drawn Canvas charts and animated metric displays.
  - API binding pipelines connecting Qt Quick to C++ controllers.
  - The AI Orchestrator (Prompt building, LLM client dispatching, intent routing).
  - Safety structures (Restore Points via `srclient.dll`).
- **Partially Built / Broken / Incomplete**:
  - No current broken functionality. The app compiles and links perfectly.

## What Is Missing

Based on the codebase and architecture definitions:

- **Network Optimizer Module**: The module stub (`network_optimizer.cpp`) exists and is intended to flush DNS and tweak TCP parameters, but is primarily a structural placeholder at this point.
- **Startup Optimizer GUI**: The backend logic to scan and manage Windows startup items exists in C++, but there is currently no dedicated QML screen to allow the user to manually toggle these items.
- **Detailed LLM Configuration UI**: Users currently have to edit the `PulseBoost.ini` file manually to switch models or point the AI client away from local Ollama to a remote OpenAI key endpoint. An in-app settings modal is missing.

## Monetization

- **Unclear from codebase**: There is no billing, licensing, stripe integration, or payment walling code present. The tool appears to be engineered as a free or openly distributable utility in its current state.

## Deployment & Infrastructure

- **Deployment**: Local Windows executable distribution. Can use Qt's `windeployqt.exe` for bundling DLLs.
- **Infrastructure**: Compiles via standard VC++ MSVC toolchain using CMake.
- **AI Infrastructure**: Decentralized relying entirely on the host machine having a local instance of Ollama running, avoiding cloud hosting costs and ensuring strict data privacy for telemetry.

## Honest Assessment

### Strengths

- **Incredible Thread Safety**: The architecture enforces an extremely strict boundary between the thread-blocking Windows APIs (WMI, PDH) and the UI render thread loop. This guarantees a silky smooth UI no matter how heavily the system is taxed.
- **Dependency Minimization**: Eliminating bulky modules like `QtCharts` in favor of bespoke pure QML canvases keeps the binary size smaller and speeds up startup times.
- **Agentic AI Approach**: Relying on deterministic routing (the `ChatRouter` detecting explicit intents to run compiled C++ modules) mixed with LLM generation for explanations ensures safe, guaranteed execution of critical tasks rather than hallucinated console commands.
- **Safety First**: The non-negotiable registry backups and restore point creation prior to optimization execution is a top-tier production practice rarely seen in consumer tweaker tools.

### Weaknesses & Technical Debt

- **Platform Coupling**: The telemetry layer relies exclusively on deeply coupled Windows structures. Porting this application to Linux or macOS would require an entire rewrite of `core/`.
- **WMI Performance Quirk**: WMI queries (`Win32_SystemRestore`, network configs) can occasionally hang or be exceptionally slow dynamically on broken Windows installations. WMI failure fallbacks could be improved.
- **QML Component Duplication**: While the visual elements are robust, some QML color palette definitions (`surface`, `textMuted`, etc.) are duplicated across files (`Charts.qml`, `StorageMap.qml`) instead of utilizing a singular shared UI singleton or `qmldir` theme.

### What Needs Fixing Before Scaling

1. **Centralized QML Theming**: Move all custom colors and palette definitions into a centralized `Style.qml` singleton to prevent diverging designs as more screens are added.
2. **LLM Error Resilience**: While there is a network timeout handling system for the local LLM, integrating proper streaming chunks handling would immensely improve UX for slower models.
3. **Settings UI**: Construct a dedicated GUI to allow users to configure custom prompts and inject remote API keys so they aren't bound exclusively to needing the RAM buffer constraint required to run local Ollama models.
