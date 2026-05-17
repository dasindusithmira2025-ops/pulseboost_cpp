# PulseBoost Architecture Map

## Current Runtime Topology
- Preferred desktop launcher: [`pulseboost/electron/main.cjs`](d:\PulseBoost Python Project\pulseboost\electron\main.cjs)
- Desktop fallback launcher: [`pulseboost/desktop_app.py`](d:\PulseBoost Python Project\pulseboost\desktop_app.py)
- Desktop start script: [`start_desktop.ps1`](d:\PulseBoost Python Project\start_desktop.ps1) -> [`pulseboost/ui/scripts/launch-desktop.cjs`](d:\PulseBoost Python Project\pulseboost\ui\scripts\launch-desktop.cjs)
- Backend entrypoint: [`serve_app.py`](d:\PulseBoost Python Project\serve_app.py) -> [`pulseboost/api/main.py`](d:\PulseBoost Python Project\pulseboost\api\main.py)
- Frontend shell: [`pulseboost/ui/src/App.jsx`](d:\PulseBoost Python Project\pulseboost\ui\src\App.jsx) plus modular page surfaces under [`pulseboost/ui/src/pages`](d:\PulseBoost Python Project\pulseboost\ui\src\pages)
- Primary persistence: SQLite via [`pulseboost/core/database.py`](d:\PulseBoost Python Project\pulseboost\core\database.py) plus the legacy cognition memory layer where still needed

## Completed Product Areas

### Foundation Platform
- Typed models, DB bootstrap, capability snapshots, platform profiling, safety guard, audit log, revert manager, and session recovery are live
- Startup/shutdown paths record clean-exit state and attempt temporary tweak cleanup

### System Optimizer
- Validated tweak catalog with apply/revert flows
- Registry/service/process-priority/affinity wrappers remain capability-gated and honest about dry-run or unavailable states

### Live Runtime
- Orchestrator remains the central runtime coordinator
- Session tracking, metrics SSE, WebSocket full-state updates, and timeline persistence are integrated

### Proof / Benchmark
- Benchmark runs persist before/after evidence, unsupported metric reasons, tweak sets, and verdicts
- Benchmark Mode is exposed in the existing shell instead of a parallel UI

### Adaptive / Network / GPU
- Adaptive engine is local-only, rule-based, cooldown-limited, and auditable
- Network diagnostics and QoS preview surfaces are integrated with explicit unsupported states
- GPU telemetry/settings and BIOS advisory surfaces are vendor-aware and capability-gated

### Profiles / Trust / Settings
- Game profiles persist with `.pbprofile` export
- Trust Center reports real runtime safety, rollback, recovery, and unavailable-capability state
- Settings exposes expert-mode and adaptive-engine control without bypassing safety rules

## Desktop Runtime State
- Electron now exists as a working preferred shell path with a preload bridge and backend lifecycle management
- PyWebView remains in place as the fallback launcher when Electron is unavailable or intentionally bypassed
- Backend runtime identity is now environment-driven so Trust Center and status surfaces can distinguish Electron from PyWebView honestly
- The current React shell now uses a staged-inspection dashboard, hero-based page headers, split review surfaces, and plan-aware account/settings architecture instead of the older always-visible metrics wall

## Current Module Boundaries
- Keep [`pulseboost/api/main.py`](d:\PulseBoost Python Project\pulseboost\api\main.py) as the FastAPI app root and service wiring point
- Keep [`pulseboost/core/agents/orchestrator.py`](d:\PulseBoost Python Project\pulseboost\core\agents\orchestrator.py) as the runtime coordinator
- Keep [`pulseboost/ui/src/App.jsx`](d:\PulseBoost Python Project\pulseboost\ui\src\App.jsx) as the top-level React shell
- Keep [`pulseboost/ui/src/components/PagePrimitives.jsx`](d:\PulseBoost Python Project\pulseboost\ui\src\components\PagePrimitives.jsx) as the shared UI system for heroes, surfaces, tables, callouts, locks, and review rails
- Keep Windows-specific behavior behind wrappers/adapters so tests remain mockable off-machine

## Remaining Gaps
- GPU writes, QoS writes, and frame-hook-backed FPS capture remain intentionally unimplemented for safety and credibility reasons
- Frontend validation is still build-based; browser automation remains future work
- Installer packaging is documented, but not yet implemented in this repo
- Stitch MCP is now the practical design source of truth for the shipped UI system, and [`docs/DESIGN_SYSTEM.md`](d:\PulseBoost Python Project\docs\DESIGN_SYSTEM.md) mirrors that source in-repo
