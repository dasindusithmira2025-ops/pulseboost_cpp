# PulseBoost

PulseBoost is a premium Windows gaming and PC optimization desktop application focused on transparency, auditability, safe rollback, and measurable performance evidence.

> Status: active development. Installer packaging and some hardware-vendor integrations remain roadmap items; unsupported capabilities are surfaced honestly in the app.

## What PulseBoost Does

- Monitors live CPU, GPU, RAM, network, session, and system state signals.
- Applies supported optimizations only through auditable and reversible service layers.
- Records system actions in an audit trail with plain-English rationale and rollback metadata.
- Shows Trust Center, Benchmark, Network, GPU, Game Profiles, Settings, and Optimization surfaces.
- Uses local-first SQLite persistence and keeps system-level behavior behind Windows-specific adapters.

## Stack

- Backend: Python + FastAPI
- Desktop shell: Electron
- Frontend: React + Vite + Tailwind CSS
- Storage: SQLite
- IPC/runtime bridge: localhost HTTP and safe Electron preload APIs

## Repository Layout

```text
.
|-- pulseboost/
|   |-- api/          FastAPI backend and API routes
|   |-- core/         domain services, safety guards, persistence, metrics
|   |-- electron/     Electron main and preload entry points
|   |-- infra/        supporting infrastructure files
|   |-- prompts/      local assistant/system prompt assets
|   |-- tests/        Python unit and integration tests
|   `-- ui/           React, Vite, Tailwind frontend
|-- docs/             product, architecture, readiness, and API docs
|-- scripts/          development and diagnostics scripts
|-- .github/          CI, issue templates, PR template, Dependabot config
|-- AGENTS.md         repository execution rules
|-- PLANS.md          phase roadmap
|-- PROGRESS.md       implementation log
`-- TESTING.md        validation guide
```

## Quick Start

Install Python 3.11 and Node.js 20, or use the local portable runtimes when present.

```powershell
# Frontend dependencies
cd pulseboost\ui
npm install
cd ..\..

# Run the desktop launcher
.\run_app.ps1
```

The launcher builds the frontend and starts the preferred Electron desktop shell. The backend exposes a local health endpoint on the selected desktop port, normally `http://127.0.0.1:18400` through `http://127.0.0.1:18420`.

## Validation

```powershell
pulseboost\tools\python\python.exe -m compileall pulseboost/api pulseboost/core pulseboost/tests
pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests
cd pulseboost\ui
npm run build
npm run desktop:check
```

When portable runtimes are not present, run the same checks with system `python` and `npm`.

## Documentation

- [Product specification](docs/PULSEBOOST_SPEC.md)
- [Current architecture](docs/PULSEBOOST_TECHNICAL_ARCHITECTURE_CURRENT.md)
- [API reference](docs/PULSEBOOST_API_REFERENCE.md)
- [Feature matrix](docs/PULSEBOOST_FEATURE_MATRIX.md)
- [Known limitations](docs/KNOWN_LIMITATIONS.md)
- [Production readiness](docs/PRODUCTION_READINESS.md)
- [Release checklist](RELEASE_CHECKLIST.md)

## Safety Principles

- Save previous state before system changes.
- Log all applicable actions to the audit trail.
- Keep rollback metadata available where a change can be reverted.
- Never harm protected Windows processes.
- Gate risky GPU, power, service, registry, and network changes behind explicit safeguards.
- Report unsupported states as unavailable instead of pretending success.

## License

PulseBoost is proprietary software. See [LICENSE.md](LICENSE.md).
