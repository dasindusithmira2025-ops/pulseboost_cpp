# PulseBoost AI - Remaining Native GA Gaps

Last updated: April 3, 2026
Scope: shipping native `ui/qml/` + Qt/C++ shell

## Landed recently

- Native Qt/QML is now the shipping shell and the 7-screen product IA is in place:
  - `Home`
  - `Optimizations`
  - `Boost-Up`
  - `Games`
  - `AI`
  - `Backup`
  - `Settings`
- The native shell was rebuilt around:
  - `ui/qml/layout/AppWindow.qml`
  - `ui/qml/layout/Sidebar.qml`
  - `ui/qml/layout/TopBar.qml`
  - `ui/qml/layout/ContentArea.qml`
  - `ui/qml/layout/StatusBar.qml`
- Heavy native shell visuals were reduced and shared style/components were flattened in:
  - `ui/qml/style/Style.qml`
  - `ui/qml/style/Animations.qml`
  - `ui/qml/components/foundation/GlassPanel.qml`
  - `ui/qml/components/controls/GlowButton.qml`
- New native product surfaces now exist in:
  - `ui/qml/screens/Home.qml`
  - `ui/qml/screens/Optimizations.qml`
  - `ui/qml/screens/BoostUp.qml`
  - `ui/qml/screens/Games.qml`
  - `ui/qml/screens/Backup.qml`
  - `ui/qml/screens/AiChat.qml`
  - `ui/qml/screens/Settings.qml`
- Advanced specialist tools were removed from top-level nav and retained under `Settings > Advanced` through:
  - `ui/qml/screens/Tools.qml`
- `UiController` now exposes newer product features directly to the native shell, including:
  - pulse score
  - benchmark delta
  - advisor items
  - optimization presets
  - tweak list/apply/revert
  - detected games and game-session actions
  - AI preference state
- `app/main.cpp` now wires the newer product engines into the native GUI path:
  - `TweakEngine`
  - `PulseBench`
  - `SystemAdvisor`
  - `GameOptimizer`
- The native release now builds and launches successfully from:
  - `build_run\Release\PulseBoostAI.exe`
- Product docs now identify Qt/QML as canonical and Tauri as deprecated reference-only.

## Still open

- `modules/tweak_engine.cpp`
  The native shell now exposes optimization presets and tweak flows, but the real tweak catalog is still a first production slice rather than the full desired coverage.

- `core/game_optimizer.cpp` and `ui/qml/screens/Games.qml`
  Detection, optimize, launch-and-optimize, and revert are surfaced natively. Remaining work is deeper per-title orchestration, launcher-specific handling, and stronger auto-revert/session-completion behavior.

- `core/pulse_bench.cpp`
  Benchmarking exists and benchmark delta is surfaced into the native shell, but proof-of-work and benchmark storytelling still need a more polished native presentation.

- `core/system_advisor.cpp` and `ui/qml/screens/Home.qml`
  Advisor content is now folded into the native product surfaces. Remaining work is deeper hardware and BIOS-level guidance like XMP/Resizable BAR/DirectStorage where detection is feasible.

- `ui/qml/screens/BoostUp.qml`
  Core boost actions are surfaced natively, but broader repair/cleanup coverage and richer post-action proof summaries are still open.

- `ui/qml/screens/Backup.qml`
  Snapshot and recovery framing are now first-class in the native shell. Remaining work is deeper persisted history and richer restore/report visualization.

- `ui/qml/screens/AiChat.qml` and native AI bindings
  AI mode and guidance are surfaced in the native shell, but the full native chat-stream parity and richer action metadata can still be improved.

- `ui/qml/screens/Settings.qml`
  Settings now owns shipping configuration and advanced tools, but further convergence of update/license/diagnostics detail is still open.

- Tauri deprecation hardening
  Tauri is now deprecated in docs and product direction, but the repo still contains the full reference surface. Final submission cleanup can further reduce confusion around non-shipping paths.

- GA polish
  Final pass work still includes:
  - additional native-shell empty/error/loading state polish
  - broader proof-of-work messaging
  - deeper native convergence of secondary utilities
  - final release packaging and submission hardening
