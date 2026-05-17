# Contributing to PulseBoost

PulseBoost is a safety-first Windows optimization suite. Changes must preserve transparency, auditability, and rollback behavior.

## Development Rules

- Keep the existing stack: Python, FastAPI, SQLite, Electron, React, Vite, and Tailwind CSS.
- Do not add fake success states or placeholder controls.
- Keep Windows-specific system calls behind adapters.
- Add or update tests for meaningful behavior changes.
- Update documentation when behavior, architecture, or release status changes.

## Local Validation

```powershell
pulseboost\tools\python\python.exe -m compileall pulseboost/api pulseboost/core pulseboost/tests
pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests
cd pulseboost\ui
npm run build
npm run desktop:check
```

When portable runtimes are not present, install Python 3.11 and Node.js 20 locally, then run the same commands with `python` and `npm`.

## Pull Requests

Every PR should explain:

- what changed
- why it changed
- what safety or rollback behavior is affected
- which checks were run
- remaining risks or unsupported states
