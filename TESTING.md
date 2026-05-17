# Testing Guide

## Primary Validation Commands

### Backend Compile Check
```powershell
pulseboost\tools\python\python.exe -m compileall pulseboost/api pulseboost/core pulseboost/tests
```

### Python Test Suite
```powershell
pulseboost\tools\python\python.exe -m unittest discover -s pulseboost\tests
```

### Frontend Production Build
```powershell
cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run build"
```

### Desktop Runtime Check
```powershell
cmd.exe /c "set PATH=d:\PulseBoost Python Project\pulseboost\tools\node-tmp\node-v20.15.1-win-x64;%PATH%&& npm.cmd run desktop:check"
```

## Focused Test Files
- `pulseboost/tests/test_foundation.py`
- `pulseboost/tests/test_optimizer_core.py`
- `pulseboost/tests/test_phase3.py`
- `pulseboost/tests/test_phase5_benchmark.py`
- `pulseboost/tests/test_phase6_adaptive.py`
- `pulseboost/tests/test_phase7_network.py`
- `pulseboost/tests/test_phase8_gpu.py`
- `pulseboost/tests/test_phase9_profiles_trust.py`
- `pulseboost/tests/test_phase10_hardening.py`
- `pulseboost/tests/test_desktop_shell.py`

## Validation Notes
- Frontend validation is currently build-based; there is no browser-automation suite yet
- `TestClient` runs may emit `anyio` resource warnings without functional failures
- GPU/QoS/frame-hook gaps are expected to appear as unavailable or dry-run states, not test failures
- `npm install` is now required for the preferred Electron shell path because Electron is a local dev dependency in `pulseboost/ui/package.json`

## Pre-Release Minimum Bar
- Compile checks pass
- Full Python test suite passes
- Frontend build passes
- Trust Center and Settings wording is reviewed against current limitations
- Known limitations remain documented honestly
