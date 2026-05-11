# PulseBoost AI - Phase 0 Audit Report

Date: 2026-03-12  
Auditor: Codex

## Scope

- Audited full tracked source/config tree (`rg --files`) and runtime logs.
- Source/config files audited: 104
- All `.cpp/.h/.hpp/.qml/.md/.ps1/.iss/.qrc` files reviewed.
- Build artifacts (`build/`, `build_verify/`) were inspected only for runtime evidence, not treated as source of truth.

---

## 0.1 Structural Audit

Legend:
- `COMPLETE` = functional and production-usable in current architecture
- `PARTIAL` = functional but missing production-grade behavior/safety
- `BROKEN` = runtime errors, invalid bindings, or known behavior failure
- `STUB` = placeholder/MVP implementation
- `MISSING` = required by target architecture but not present

### Root / Build / Ops Files

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `CMakeLists.txt` | PARTIAL | Builds current app, but not modular target architecture; no ONNX Runtime integration path; mixed legacy/new modules | Depends on all source groups | HIGH |
| `qml.qrc` | PARTIAL | Registers current QML only; no modular UI package structure | Used by app startup | HIGH |
| `README.md` | PARTIAL | Claims "default local AI engine" but runtime `AgentEngine` is Gemini-first | User/docs only | MEDIUM |
| `PulseBoost_Installer.iss` | PARTIAL | Installer exists, but no robust dependency/version gates | Packaging | MEDIUM |
| `scripts/install-deps.ps1` | PARTIAL | Good baseline installer; no deterministic Qt component verification post-install | Dev env setup | MEDIUM |
| `scripts/build.ps1` | COMPLETE | Build bootstrap works with VS + Qt path | Dev build flow | LOW |
| `scripts/test.ps1` | PARTIAL | Smoke-only, no assertions on UI/QML errors/safety guarantees | CI/manual tests | MEDIUM |
| `docs/ARCHITECTURE.md` | PARTIAL | Out of date vs current runtime wiring | Documentation | MEDIUM |
| `docs/PRODUCT_DOCUMENT.md` | PARTIAL | Product intent ahead of implementation | Documentation | LOW |

### App Entry

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `app/main.cpp` | PARTIAL | High coupling; constructs many modules inline; shared `SystemScanner` accessed from telemetry thread and AI path | Depends on core/ai/modules/ui_backend/data | CRITICAL |

### AI Layer

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `ai/agent_engine.cpp` | PARTIAL | Gemini path active; planned tool execution not fully wired; fallback chain incomplete | Used by `app/main.cpp`, QML via context property | CRITICAL |
| `ai/ai_diagnostics_engine.cpp` | COMPLETE | Strong heuristic reasoning; local-only planner works; no model-backed inference | Used by chatbot/CLI | HIGH |
| `ai/chat_router.cpp` | PARTIAL | Keyword routing only; intent confidence and conflict resolution limited | Used by `AgentEngine` | HIGH |
| `ai/chatbot_interface.cpp` | PARTIAL | Legacy interface duplicates modern agent responsibilities | Legacy path | MEDIUM |
| `ai/context_builder.cpp` | COMPLETE | Provides telemetry/system prompt context | Used by agent/proactive | LOW |
| `ai/gemini_client.cpp` | PARTIAL | Stream parsing is string-based and brittle; no robust SSE parser | Used by `AgentEngine` | CRITICAL |
| `ai/llm_client.cpp` | PARTIAL | Functional request client but currently unused in main runtime path | Standalone/legacy | MEDIUM |
| `ai/ollama_client.cpp` | PARTIAL | Non-streaming + per-call signal wiring risk; not integrated as active fallback chain | Agent dependency | HIGH |
| `ai/proactive_monitor.cpp` | PARTIAL | Basic checks only; not deeply integrated into UI alert pipeline | AI monitoring | MEDIUM |
| `ai/prompt_builder.cpp` | PARTIAL | Exists but not central in active runtime AI pipeline | Legacy/shared utility | MEDIUM |
| `ai/session_memory.cpp` | COMPLETE | Rolling conversation memory works | `AgentEngine` | LOW |
| `ai/system_prediction.cpp` | PARTIAL | Minimal predictive logic; not anomaly-grade | `AiDiagnosticsEngine` | MEDIUM |

### Common / System Integration

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `common/wmi_wrapper.cpp` | PARTIAL | Raw-pointer PIMPL; `executeMethod` hardcoded fallback (`false`) | Standalone helper | HIGH |
| `include/PulseBoostAI/common/wmi_wrapper.hpp` | PARTIAL | API exists but implementation is incomplete for method execution | `common/wmi_wrapper.cpp` | HIGH |
| `include/PulseBoostAI/common/wmi_utils.hpp` | PARTIAL | Inline WMI session duplicates wrapper path; error handling inconsistent | `core/system_scanner.cpp` | HIGH |
| `include/PulseBoostAI/common/windows_utils.hpp` | COMPLETE | Utility surface broadly usable | Widely depended on | LOW |
| `include/PulseBoostAI/common/models.hpp` | PARTIAL | Snapshot schema is smaller than target architecture requirements | Cross-project model hub | HIGH |

### Core Engine

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `core/system_scanner.cpp` | PARTIAL | WMI numeric conversion (`std::stod`) can throw; no guarded fallback; shared mutable counters | Used by telemetry + AI | CRITICAL |
| `core/telemetry_engine.cpp` | PARTIAL | Good worker thread split, but single worker still owns mixed responsibilities and shared scanner access | `app/main.cpp` | CRITICAL |
| `core/telemetry_engine.h` | PARTIAL | Duplicate header exists alongside `include/.../telemetry_engine.hpp` | Legacy duplication risk | HIGH |
| `include/PulseBoostAI/core/telemetry_engine.hpp` | PARTIAL | Current engine contract smaller than requested architecture | Used by main + cpp | HIGH |
| `core/process_manager.cpp` | PARTIAL | `cpuSamples_` mutable map without explicit synchronization | Used by scanner/UI/modules | CRITICAL |
| `core/memory_analyzer.cpp` | COMPLETE | Works with process manager data | Scanner dependency | LOW |
| `core/disk_analyzer.cpp` | COMPLETE | Functional system drive analysis/category output | Scanner/UI/modules | LOW |
| `core/startup_optimizer.cpp` | PARTIAL | Works for scan/toggle, but lacks richer safety/risk modeling | Scanner/UI/modules | HIGH |
| `core/service_manager.cpp` | COMPLETE | Service enumeration/control baseline present | Scanner/modules | MEDIUM |
| `core/registry_optimizer.cpp` | PARTIAL | Basic operations, missing robust rollback integration | Startup + modules | HIGH |
| `core/auto_updater.cpp` | STUB | Explicit MVP placeholder, no manifest/download integrity flow | Unused infra | HIGH |
| `core/crash_reporter.cpp` | PARTIAL | Minidump works, but no upload/reporting pipeline | Infra | MEDIUM |
| `core/license_manager.cpp` | STUB | Mock key validation (`PRO-` prefix) only | Feature gate | HIGH |

### Data Layer

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `data/telemetry_cache.cpp` | COMPLETE | Thread-safe ring cache for recent telemetry | Main/UI/AI | LOW |
| `data/telemetry_logger.cpp` | PARTIAL | CSV parsing uses unchecked `std::stod/std::stoi` and may throw on malformed lines | Main/tests | HIGH |
| `data/optimization_history.cpp` | COMPLETE | Action history persistence works | UI/AI | LOW |

### Modules

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `modules/junk_cleaner.cpp` | PARTIAL | Safe cleanup baseline exists; no full restore-point + rollback orchestration per run | UI/AI | HIGH |
| `modules/game_mode.cpp` | PARTIAL | Priority tuning works; service suspension/restore state machine incomplete | UI/AI | HIGH |
| `modules/developer_mode.cpp` | PARTIAL | Advisory + limited priority boosts only | AI | MEDIUM |
| `modules/network_optimizer.cpp` | PARTIAL | `measureLatency()` returns synthetic value (22ms) on ping success | UI/AI | HIGH |
| `modules/safety_guard.cpp` | PARTIAL | Restore point + log exists; immutable cryptographic audit and rollback manager missing | UI/AI | CRITICAL |
| `modules/duplicate_file_finder.cpp` | COMPLETE | Baseline duplicate scan functional | module utility | LOW |
| `modules/large_file_scanner.cpp` | COMPLETE | Baseline large-file scan functional | module utility | LOW |
| `modules/startup_optimizer.cpp` | STUB | Thin wrapper around core; no module-grade workflow | module utility | MEDIUM |
| `modules/disk_analyzer.cpp` | STUB | Thin wrapper only | module utility | MEDIUM |
| `modules/process_optimizer.cpp` | STUB | Thin wrapper only | module utility | MEDIUM |
| `modules/service_manager.cpp` | STUB | Thin wrapper only | module utility | MEDIUM |

### UI Backend

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `ui_backend/ui_controller.cpp` | PARTIAL | High surface area; contains hardcoded accent colors; direct kill process action without richer safety tiers | QML bridge | CRITICAL |
| `ui_backend/feature_gate.cpp` | PARTIAL | Tier checks are basic; depends on stub license manager | QML settings/gating | HIGH |
| `ui_backend/notification_manager.cpp` | STUB | Console fallback only, no WinRT toast pipeline | UI infra | MEDIUM |

### QML Screens / Components

| File | State | Specific Issues | Dependencies / Depended By | Priority |
|---|---|---|---|---|
| `ui/qml/MainWindow.qml` | PARTIAL | Monolithic layout; runtime module warnings seen historically; mixed style aliases | Root QML entry | CRITICAL |
| `ui/qml/Dashboard.qml` | PARTIAL | Large inline component duplication + hardcoded colors/spacing values | Main screen | CRITICAL |
| `ui/qml/Charts.qml` | PARTIAL | Hardcoded spacing/colors; missing explicit loading/error states | Screen stack | HIGH |
| `ui/qml/ProcessTable.qml` | PARTIAL | Inline component duplication; no dedicated empty/error/loading components | Screen stack | HIGH |
| `ui/qml/StorageMap.qml` | PARTIAL | Treemap UX low value for dominant "Other" category; weak readability in extreme skew | Screen stack | HIGH |
| `ui/qml/AiChat.qml` | PARTIAL | Works, but still includes hardcoded values and inconsistent state handling | Screen stack | HIGH |
| `ui/qml/StartupManager.qml` | BROKEN | Runtime warnings (`undefined` color/string) + `impactColor` reference error | Screen stack | CRITICAL |
| `ui/qml/NetworkOptimizer.qml` | BROKEN | Uses undefined style keys (`surfaceRaised`, `surfaceOverlay`, `border`, `fontHeading`) | Screen stack | CRITICAL |
| `ui/qml/Settings.qml` | BROKEN | Uses undefined style keys | Screen stack | HIGH |
| `ui/qml/HealthHistory.qml` | BROKEN | Placeholder-only; undefined style keys; hardcoded Font Awesome family | Screen stack | HIGH |
| `ui/qml/Onboarding.qml` | BROKEN | Undefined style keys; onboarding logic incomplete | Overlay loader | HIGH |
| `ui/qml/NavButton.qml` | COMPLETE | Usable nav button component | MainWindow | LOW |
| `ui/qml/ToastNotification.qml` | COMPLETE | Works for feedback banner behavior | MainWindow | LOW |
| `ui/qml/ProGate.qml` | BROKEN | Undefined style keys | Potential overlay | HIGH |
| `ui/qml/StreamingText.qml` | COMPLETE | Streaming text effect works | AI chat | LOW |
| `ui/qml/Style.qml` | PARTIAL | Good base theme but missing keys used elsewhere (`fontHeading`, `surfaceRaised`, `surfaceOverlay`, `border`) | All QML | CRITICAL |
| `ui/qml/qmldir` | PARTIAL | Only `Style` singleton exported; no modular singleton map | QML module registration | HIGH |

### Headers (Additional)

All remaining `include/PulseBoostAI/...` headers were scanned; most are `COMPLETE` interface shells with `PARTIAL` status inherited from incomplete implementations. No missing header-level declarations were detected for existing compiled targets.

---

## 0.2 QML Audit

### Hardcoded Hex Colors (must move to `Style.qml` tokens)

- `ui/qml/Dashboard.qml`: `#58a6ff`, `#8a63ff`, `#0e1624`, `#0d1728`, `#dce8ff`
- `ui/qml/Charts.qml`: `#0d1728`
- `ui/qml/AiChat.qml`: `#a5d8ff`
- `ui/qml/Style.qml`: contains palette definitions (expected location)

### Hardcoded Font Families (must move to Style token)

- `ui/qml/HealthHistory.qml`: `font.family: "Font Awesome 6 Free"`

### Hardcoded Spacing / Padding / Radius Density

Counts of hardcoded layout literals detected:

- `ui/qml/Dashboard.qml`: 57
- `ui/qml/ProcessTable.qml`: 40
- `ui/qml/AiChat.qml`: 33
- `ui/qml/StorageMap.qml`: 22
- `ui/qml/Charts.qml`: 21
- `ui/qml/MainWindow.qml`: 17
- `ui/qml/StartupManager.qml`: 15
- `ui/qml/Settings.qml`: 13
- `ui/qml/NetworkOptimizer.qml`: 13
- `ui/qml/Onboarding.qml`: 7
- `ui/qml/NavButton.qml`: 4
- `ui/qml/HealthHistory.qml`: 4

### Duplicated Component Patterns (extract to shared components)

- `Dashboard.qml`: inline `InfoBadge`, `ActionButton`, `MetricTile`, `HealthDial`
- `ProcessTable.qml`: inline `SortHeader` and repeated row cells
- `StorageMap.qml` and `Dashboard.qml`: repeated "card + title + subtitle + bar" patterns
- Multiple screens: repeated top-title + description headers without shared header component

### Missing States

- Missing explicit `loading/error/empty/disabled` states in: `Dashboard.qml`, `Charts.qml`, `StorageMap.qml`, `AiChat.qml`, `NetworkOptimizer.qml`, `StartupManager.qml`, `Settings.qml`
- `HealthHistory.qml` currently placeholder-only, no data/error pipeline

### Missing / Inconsistent Animations

- No standardized transitions for screen load/change states
- Inconsistent hover/press/disabled feedback across controls
- No centralized animation tokens singleton (only per-screen ad hoc animations)

### Runtime QML Errors (from `build/Release/logs/gui_runtime.log`)

- `StartupManager.qml`: `ReferenceError: impactColor is not defined`
- `StartupManager.qml`, `NetworkOptimizer.qml`, `Onboarding.qml`, `HealthHistory.qml`: multiple `Unable to assign [undefined]` warnings
- Historical module load failures for `QtCore`/`Qt.labs.platform` resolved after deployment, but style contract failures remain active

---

## 0.3 C++ Backend Audit

### Thread Safety

- `CRITICAL`: `SystemScanner` is shared between telemetry worker thread and UI/AI call path (`AgentEngine::ask()` can call `scanner_.scan()`), creating race risk on mutable counters and timestamp fields.
- `CRITICAL`: `ProcessManager::cpuSamples_` map mutated without explicit synchronization.
- `OK`: Telemetry signal crossing from worker to UI uses queued connection semantics.

### WMI Calls / Fallbacks

- `PARTIAL`: Two parallel WMI paths (`WmiSession` in `wmi_utils.hpp` and `WmiWrapper`) create maintenance divergence.
- `PARTIAL`: WMI numeric parsing in `system_scanner.cpp` uses `std::stod` without try/catch fallback.
- `STUB`: `WmiWrapper::executeMethod()` returns `false` and is not a full method invocation implementation.

### Memory Management / RAII

- Raw-pointer PIMPL patterns detected in:
  - `common/wmi_wrapper.cpp`
  - `core/auto_updater.cpp`
  - `core/license_manager.cpp`
  - `ui_backend/notification_manager.cpp`
- These are manageable but should move to `std::unique_ptr` for safer ownership semantics.

### Signal / Slot Safety

- `PARTIAL`: `OllamaClient::sendPrompt()` reconnects `QNetworkAccessManager::finished` each call (risk of duplicate callbacks over time if not carefully managed).
- `PARTIAL`: Network clients need stricter reply lifecycle/timeout/error normalization.

### Error Path Coverage

- `data/telemetry_logger.cpp`: CSV parse can throw and currently has no malformed-row handling.
- `ai/gemini_client.cpp`: stream extraction is brittle and can mis-parse fragmented escaped content.
- `modules/network_optimizer.cpp`: latency measurement not real (fixed synthetic success value).
- `core/auto_updater.cpp`, `core/license_manager.cpp`: placeholder behavior not production-safe.

---

## 0.4 Feature Completeness Matrix

### MONITORING

- [x] CPU usage polling (1s intervals) - PDH - **COMPLETE**
- [x] RAM usage polling (1s intervals) - PDH - **COMPLETE**
- [x] Disk I/O polling (3s intervals) - PDH - **COMPLETE**
- [ ] Network I/O polling (3s intervals) - PDH - **PARTIAL**
- [x] Process list polling (5s intervals) - Process API - **COMPLETE**
- [ ] Health score calculation (weighted avg) - Internal - **PARTIAL** (heuristic scoring only)
- [x] Historical telemetry logging - CSV - **COMPLETE**

### AI ENGINE

- [x] Gemini 2.5 Pro client - HTTP - **PARTIAL** (brittle stream parser)
- [x] Ollama fallback client - HTTP - **PARTIAL** (not fully active in provider chain)
- [x] Heuristic offline fallback - Internal - **PARTIAL** (distributed across legacy/local paths)
- [x] Context builder (telemetry snapshot) - Internal - **COMPLETE**
- [x] Session memory (conversation history) - Internal - **COMPLETE**
- [x] Intent router (action detection) - Regex - **PARTIAL**
- [x] Proactive monitor (threshold alerts) - QTimer - **PARTIAL**
- [ ] Custom PulseBoost AI model - ONNX/local - **MISSING**

### OPTIMIZATION MODULES

- [x] Junk cleaner - Win32 API - **PARTIAL**
- [x] Game mode - Process API - **PARTIAL**
- [x] Developer mode - Process API - **PARTIAL**
- [x] Network optimizer (DNS + TCP) - netsh/Registry - **PARTIAL**
- [x] Startup manager (read + toggle) - Registry - **PARTIAL**
- [ ] RAM force-free - EmptyWorkingSet API - **MISSING**
- [ ] Disk defrag trigger - Windows Defrag API - **MISSING**

### SAFETY

- [x] Restore point creation - srclient.dll - **COMPLETE**
- [ ] Registry backup before changes - Registry API - **PARTIAL**
- [x] Safety audit log - File I/O - **COMPLETE**
- [ ] WMI failure fallbacks - Internal - **PARTIAL**
- [ ] Action rollback system - Registry restore - **MISSING**

### UI SCREENS

- [x] Dashboard - QML - **PARTIAL**
- [x] Charts (60s rolling) - QML Canvas - **PARTIAL**
- [x] Process table - QML - **PARTIAL**
- [x] Storage treemap - QML Canvas - **PARTIAL**
- [x] AI Chat - QML - **PARTIAL**
- [x] Startup manager screen - QML - **BROKEN**
- [x] Network optimizer screen - QML - **BROKEN**
- [x] Settings screen - QML - **BROKEN**
- [x] Health history screen - QML - **BROKEN/PARTIAL**
- [x] Onboarding wizard - QML - **BROKEN/PARTIAL**

### INFRASTRUCTURE

- [ ] Auto-updater - HTTP + installer - **STUB**
- [x] Crash reporter - Win32 API - **PARTIAL**
- [ ] License manager - HTTP + local - **STUB**
- [x] Feature gate system - Internal - **PARTIAL**
- [x] System tray integration - Qt System Tray - **COMPLETE**
- [ ] Windows toast notifications - WinRT - **PARTIAL/STUB**
- [x] CLI headless mode - Qt CLI - **COMPLETE**
- [x] Installer script - Inno Setup - **COMPLETE**

---

## Critical Priority Fix Queue (Execution Gate)

1. Resolve QML style contract breakage and undefined properties causing runtime warnings (`StartupManager`, `NetworkOptimizer`, `Settings`, `Onboarding`, `HealthHistory`, `ProGate`).
2. Remove scanner cross-thread race by isolating telemetry scanner state per worker or adding synchronization strategy.
3. Harden AI provider chain and replace brittle Gemini stream parser with structured chunk parser.
4. Replace stub infrastructure (`LicenseManager`, `AutoUpdater`, notification backend) with production implementations.
5. Implement missing safety rollback and enforce high-impact prechecks globally.

---

## Missing Artifacts Against Target Architecture

Major missing top-level items from requested target:

- `CMakePresets.json`
- `src/`-based layered architecture (`app/core/optimization/safety/ai/storage/monetization/ui/system/utils` split)
- ONNX model runtime + `ai_model/` training/export pipeline artifacts
- Dedicated QML module tree (`qml/style`, `qml/layout`, `qml/screens`, `qml/components/...`)
- Unit/integration test suites under `tests/`

Phase 0 audit is complete. Proceed to Phase 1 Step 02 after approval.
