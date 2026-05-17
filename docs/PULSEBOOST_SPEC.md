# PulseBoost Product Specification

## 1. Product Overview
**PulseBoost** is a premium Windows gaming and PC optimization suite for power users and enthusiasts who want:

- full control
- full transparency
- measurable performance impact
- safe rollback
- real-time adaptive optimization

PulseBoost is designed to outperform black-box “game booster” tools by focusing on trust, proof, deep systems integration, and high-quality user experience.

## 2. Core Product Philosophy
Every optimization must be:

- visible
- explainable
- auditable
- revertible
- hardware-aware
- safely applied

For every change, PulseBoost should show:
- what changed
- before value
- after value
- rationale in plain English
- validity tag
- compatibility note
- whether it can be reverted
- whether it is temporary or persistent

PulseBoost must never feel like a black box.

## 3. Primary Differentiators
### 3.1 Transparency
Unlike black-box optimizers, PulseBoost logs every registry, service, system, network, GPU, and adaptive action.

### 3.2 Real-Time Adaptive Engine
A local-only rule-based engine monitors system conditions and applies safe, visible, revertible changes during gaming sessions.

### 3.3 Protocol-Aware Network Optimization
PulseBoost distinguishes between TCP and UDP game traffic and avoids blindly applying irrelevant tweaks.

### 3.4 Deep GPU & Hardware Awareness
PulseBoost supports vendor-aware GPU metrics and safe setting controls where available, with clear unsupported states where not available.

### 3.5 No Placebo Tweak Philosophy
Each tweak should be labeled with a validity tag:
- `VALIDATED`
- `LEGACY`
- `HARDWARE_SPECIFIC`
- `PLACEBO_RISK`

High-risk or questionable tweaks must not be silently applied.

## 4. Fixed Tech Stack
These are non-negotiable unless explicitly changed by the repo owner:

- Backend: Python
- API: FastAPI
- Frontend: Electron + React + Vite + Tailwind CSS
- Storage: SQLite primary database
- IPC: localhost HTTP and/or safe IPC bridge

## 5. Core Product Areas

### 5.1 Foundation Platform
Build and maintain:
- capability detection
- platform/hardware profiling
- compatibility checks
- safety guard
- revert manager
- session recovery
- event bus
- metrics service
- session manager
- SQLite persistence layer

### 5.2 System Optimizer
Responsibilities:
- registry-backed tweaks
- service start-type management
- process priority handling
- affinity handling
- temporary gaming-session tweaks
- safe restore on exit

Required initial tweak targets:
- Multimedia system profile scheduling
- Game task scheduling
- SysMain control
- Windows Search session control
- timer resolution handling
- CPU core parking-related control where safely supported

### 5.3 Adaptive Engine
Responsibilities:
- 500ms monitoring loop
- rule-based decisions
- cooldowns and rate-limits
- visible and revertible adaptive actions
- per-game historical learning storage

This is not fake cloud AI.
This is a local expert system with learning history.

### 5.4 Network Optimizer
Responsibilities:
- protocol-aware logic
- TCP tweak gating
- UDP-oriented NIC tuning focus
- QoS policy support
- latency diagnostics
- packet loss / jitter / bufferbloat signals
- active NIC awareness

### 5.5 GPU Controller
Responsibilities:
- vendor abstraction
- NVIDIA metrics via supported runtime where available
- AMD metrics/settings abstraction where available
- registry-backed generic graphics settings
- BIOS advisory checklist (read-only guidance)
- safe bounds and explicit confirmations for risky changes

### 5.6 Audit & Transparency Layer
Responsibilities:
- append-only audit logging
- revert metadata
- per-entry revert
- export JSON/CSV
- filter by module/date/session/source/status
- UI timeline support

### 5.7 Proof / Benchmark Engine
Responsibilities:
- baseline capture
- optimized capture
- before/after comparison
- verdict generation:
  - `HELPED`
  - `NO_MEASURABLE_IMPACT`
  - `REGRESSION`
  - `UNSTABLE`

This is essential to defeating placebo tweak criticism.

### 5.8 Frontend Experience
PulseBoost must look premium, compact, technical, and trustworthy.

Visual design:
- background: `#0f1117`
- surface: `#1a1d27`
- accent: `#6366f1`

Navigation:
- Dashboard
- Optimizations
- Network
- GPU
- Audit Log
- Game Profiles
- Benchmark
- Trust Center
- Settings

Required frontend principles:
- fast
- minimal clutter
- data-dense
- consistent status badges
- consistent loading/empty/error states
- no fake charts in final implementation
- no dead controls

## 6. Safety Requirements
These are non-negotiable:

1. Never apply a tweak without first saving current state.
2. Recover from unclean exit by reverting temporary active changes.
3. Validate registry paths before writing.
4. Detect admin requirements and handle gracefully.
5. Never suspend/deprioritize critical processes.
6. Service handling must be non-destructive.
7. GPU clock/power modifications must enforce safe ranges and confirmations.
8. Every risky action must be clearly explained.
9. PowerShell policy changes must be Process-scoped only.
10. Uninstall/revert path must restore original values where PulseBoost changed them.
11. Unsupported features must show honest unavailable state.
12. Adaptive actions must be visible and revertible.

## 7. Core Domain Models
The codebase should use strong typed models for at minimum:

### TweakObject
Fields:
- id
- name
- category
- registry_path
- key
- before_value
- after_value
- rationale
- validity
- impact
- compatibility_note
- hardware_requirements
- temporary
- requires_admin
- applied
- timestamp
- apply_fn
- revert_fn

### AuditEntry
Fields:
- id
- timestamp
- module
- action
- target
- before_value
- after_value
- rationale
- validity_tag
- triggered_by
- reverted
- revert_timestamp
- session_id
- status

### SessionRecord
Fields:
- id
- game_name
- executable_path
- started_at
- ended_at
- baseline_metrics_snapshot
- final_metrics_snapshot
- tweaks_applied_count
- adaptive_actions_count
- stability_score
- clean_exit

### HardwareProfile
Fields:
- CPU details
- hybrid CPU support
- GPU vendor/model
- RAM
- motherboard/BIOS advisory data
- NIC details
- Windows version/build
- supported capabilities

### BenchmarkResult
Fields:
- benchmark id
- workload/game name
- baseline fps
- optimized fps
- 1% low delta
- frame-time variance delta
- ping delta
- jitter delta
- notes
- verdict

## 8. Backend / API Requirements
FastAPI endpoints should include at minimum:

- `GET /api/status`
- `GET /api/metrics/live` (SSE)
- `GET /api/tweaks`
- `POST /api/tweaks/{id}/apply`
- `POST /api/tweaks/{id}/revert`
- `POST /api/tweaks/apply-all`
- `GET /api/audit`
- `POST /api/audit/{id}/revert`
- `GET /api/network/diagnostics`
- `GET /api/gpu/stats`
- `POST /api/gpu/settings`
- `GET /api/games`
- `GET /api/games/{id}/profile`
- `POST /api/games/{id}/profile`
- `GET /api/bios/checklist`
- `POST /api/adaptive/toggle`
- `POST /api/benchmark/run`
- `GET /api/benchmark/results`
- `GET /api/trust-center/status`

## 9. Database Requirements
SQLite should be the single source of truth.

Minimum tables:
- tweaks
- tweak_states
- audit_entries
- sessions
- session_actions
- hardware_profiles
- game_profiles
- benchmarks
- revert_snapshots
- app_state
- capabilities_cache
- notifications

## 10. Frontend Page Requirements

### Dashboard
Show:
- live CPU/GPU/RAM/network metrics
- active game banner
- active tweaks count
- adaptive engine status
- session timer
- recent actions
- FPS graph if available

### Optimizations
Each tweak card should show:
- toggle
- name
- description
- validity badge
- exact path/values
- before/after
- rationale
- impact rating
- compatibility note
- revert button

### Network
Show:
- latency graph
- packet loss
- jitter
- NIC info
- protocol-aware warnings
- diagnostics panels
- network tweak controls

### GPU
Show:
- live GPU stats
- supported settings
- BIOS checklist
- shader cache tools
- capability visibility

### Audit Log
Show:
- timeline
- filters
- per-entry revert
- export actions

### Game Profiles
Show:
- detected installed games
- recommended tweaks
- historical gains
- stability score
- import/export `.pbprofile`

### Benchmark
Show:
- before/after charts
- verdict summary
- evidence and deltas

### Trust Center
Show:
- admin status
- crash recovery status
- last clean exit
- protected process rules
- dangerous tweaks disabled
- expert mode state
- rollback readiness
- capability overview

### Settings
Show:
- expert mode
- process priority defaults
- adaptive engine toggle
- session behavior
- notification controls
- privacy/telemetry controls if applicable

## 11. UX Standard
PulseBoost must feel:
- premium
- stable
- technical
- trustworthy
- not bloated
- not childish
- not deceptive

Use:
- consistent wording
- clear warnings
- compact layouts
- polished charts
- smooth, restrained interaction design

## 12. Definition of Done
PulseBoost is considered substantially complete when:
- core foundation is stable
- tweaks are safely auditable/revertible
- sessions work reliably
- live metrics work
- audit log is first-class
- benchmark mode proves value
- adaptive engine safely reacts
- network/GPU modules expose honest capability-aware behavior
- trust center and recovery flows are solid
- UI feels coherent and professional
- docs and tests are maintained