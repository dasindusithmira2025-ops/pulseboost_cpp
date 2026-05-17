# PulseBoost

PulseBoost is a desktop machine guardian and optimization engine.
The monitoring engine is still FastAPI plus React internally, but the intended runtime is now a native desktop window powered by Python and WebView that boots the guardian backend for you, hunts inefficiency, and keeps the browser out of the main workflow.

## What is included

- Live machine metrics sampled every 3 seconds
- Smoothed 0 to 100 health score
- Baseline-aware anomaly detection
- Predictive pressure signals for CPU, RAM, and disk
- Suggestion and action preview flow for safe remediation
- Proactive waste hunting across CPU, RAM, disk, network, and thermal state
- Efficiency scoring and optimization feed
- Waste-ranked process intelligence and learned maintenance windows
- SQLite history with retention rollups
- AI chat panel with Anthropic fallback support
- Desktop shell that launches the guardian and opens PulseBoost in a native window

## Run PulseBoost as a desktop app

1. Make sure the portable Python runtime exists under `pulseboost\tools\python`.
2. Make sure UI dependencies are installed in `pulseboost\ui`.
3. Start the desktop app from the repo root:

```powershell
powershell -File start_desktop.ps1
```

The launcher builds the React UI, starts the FastAPI guardian through the bundled Python runtime, and opens a native desktop window.

## Account and licensing foundation

PulseBoost now includes a local account and entitlement foundation that is intentionally designed for a future website-owned auth and billing authority.

- Signed-out and signed-in desktop states
- Secure local session persistence for tokens using Windows-backed encryption
- Cached account, plan, entitlement, and device activation metadata in SQLite
- Entitlement-based feature gating for premium surfaces
- Honest local placeholder sign-in for desktop integration testing while website auth is still pending

See `docs/AUTH_LICENSING_ARCHITECTURE.md` for the detailed authority model, local storage strategy, activation design, and future website integration plan.

## Direct development commands

Backend only:

```powershell
pulseboost\tools\python\python.exe -m uvicorn serve_app:app --host 127.0.0.1 --port 8000
```

Frontend build:

```powershell
pulseboost\tools\node-tmp\node-v20.15.1-win-x64\npm.cmd run build --prefix pulseboost\ui
```

Desktop shell from the UI project:

```powershell
npm run desktop
```

## Project layout

- `api`: HTTP and WebSocket API
- `core`: collector, cognition, memory, planner, reasoner, executor
- `prompts`: versioned system prompts
- `ui`: React dashboard
- `desktop_app.py`: native desktop window bootstrap and backend lifecycle
- `tools`: portable runtimes used by the local app

## Notes

- The desktop shell uses `serve_app.py` so the window loads the same built UI the backend serves.
- Auto-heal stays conservative by default. Moderate and risky actions still require confirmation.
- Auth and billing remain scaffolding, not finished provider integrations.
