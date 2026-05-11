# PulseBoost AI

PulseBoost AI is a Windows-native system optimization desktop application built with C++20, Win32 telemetry, and a Qt Quick QML interface. It focuses on real diagnostics, safe remediation, and AI-assisted reasoning instead of synthetic "RAM booster" behavior.

## What it includes

- worker-thread telemetry for CPU, RAM, disk, GPU, network, startup items, services, and heavy processes
- a modern QML dashboard with health score, live trends, storage treemap, and process controls
- an embedded on-device AI chat agent with telemetry-driven diagnostics and action planning
- local action execution for safe modules (cleanup, startup audit, game mode, developer profile, restore point)
- safe optimization modules for junk cleanup, restore point creation, game mode, and startup analysis
- CSV logging for telemetry and optimization history

## Project layout

```text
PulseBoostAI/
|-- app/
|-- ai_model/
|-- ai/
|-- core/
|-- data/
|-- docs/
|-- include/PulseBoostAI/
|-- modules/
|-- ui/qml/
|-- ui_backend/
`-- CMakeLists.txt
```

## Prerequisites

- Windows 10 or Windows 11
- Visual Studio 2022
- CMake 3.20 or newer
- Qt 6.6.x with MSVC support and Qt Quick / Qt Quick Controls

## Dependency installation

Use the provided installer script:

```powershell
.\scripts\install-deps.ps1
```

The script installs or verifies:

- Git
- CMake
- Python 3.x
- Visual Studio 2022 C++ toolchain
- Qt 6.6.3 MSVC runtime

## Build

```powershell
.\scripts\build.ps1 -QtPath "C:\Qt\6.6.3\msvc2019_64"
```

Or with CMake presets:

```powershell
cmake --preset release
cmake --build --preset build-release
```

Equivalent manual configuration:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_PREFIX_PATH="C:\Qt\6.6.3\msvc2019_64"

cmake --build build --config Release --parallel
```

## Run

Deploy the Qt runtime if needed:

```powershell
& "C:\Qt\6.6.3\msvc2019_64\bin\windeployqt.exe" --release --qmldir ".\ui\qml" ".\build\Release\PulseBoostAI.exe"
```

Launch the application:

```powershell
.\build\Release\PulseBoostAI.exe
```

## Local AI engine

The default agent is fully local and does not require external AI services.

Pipeline:

```text
User prompt -> Intent + risk scoring -> Telemetry trend analysis -> Plan generation -> Safe action execution
```

The local engine evaluates:
- CPU/RAM/disk/startup/process pressure scores
- historical trend slopes from telemetry cache
- imperative intent (for safe auto-execution)
- safety requirements (restore point before high-impact actions)

## PulseModel training pipeline

A full local training/export pipeline is available under `ai_model/`:

```powershell
python ai_model/train/collect_training_data.py
python ai_model/train/prepare_dataset.py
python ai_model/train/fine_tune.py
python ai_model/train/evaluate_model.py
python ai_model/train/export_onnx.py
```

## CLI modes

The executable also supports simple non-GUI smoke operations:

```powershell
.\build\Release\PulseBoostAI.exe --scan
.\build\Release\PulseBoostAI.exe --clean
.\build\Release\PulseBoostAI.exe --chat analyze performance
.\build\Release\PulseBoostAI.exe --self-test
```

## Smoke tests

```powershell
.\scripts\test.ps1
```

## Logs

Runtime and verification output is written to `logs/`:

- `logs/gui_runtime.log`
- `logs/telemetry.csv`
- `logs/optimization_history.csv`
- `logs/self_test.log`

## Current validation

The current refactored baseline has been verified with:

- successful Release build
- successful CLI smoke tests
- clean QML startup with the root window loading successfully

## Safety model

- cleanup actions target safe disposable temp and cache locations
- restore-point creation is available before higher-impact operations
- optimization history is persisted for traceability
- AI actions are routed through explicit modules instead of hidden system tweaks
