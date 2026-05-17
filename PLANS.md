# PulseBoost Execution Plan

This file defines the implementation roadmap Codex must follow.

---

## Phase 0 — Repo Assessment and Execution Plan
### Tasks
- Inspect current repo structure
- Map current files to target architecture
- Identify completed vs incomplete parts
- Create/update `PROGRESS.md`
- Create/update architecture notes if needed
- Review dependencies and likely blockers
- Preserve working code wherever possible

### Acceptance Criteria
- Clear repo assessment summary exists
- Normalized architecture map exists
- Missing pieces are identified
- Phase order and execution assumptions are documented

---

## Phase 1 — Foundation Platform
### Build
- `core/models.py`
- `core/database.py`
- `core/event_bus.py`
- `core/capabilities.py`
- `core/platform_info.py`
- `core/compatibility.py`
- `core/safety_guard.py`
- `core/revert_manager.py`
- `core/session_recovery.py`
- `core/audit_log.py`

### Also build
- app state tracking
- clean exit / unclean exit handling
- DB bootstrap and migrations
- repository abstractions
- capability snapshot persistence

### Acceptance Criteria
- app boots and initializes DB
- capabilities can be detected/persisted
- audit entries persist
- revert snapshots persist
- crash recovery decision path exists
- tests pass for foundation modules

---

## Phase 2 — System Optimizer Core
### Build
- `core/optimizer.py`
- safe registry wrapper
- safe service wrapper
- process priority manager
- affinity manager
- game detection integration hooks
- temporary tweak lifecycle

### Initial implementation scope
Only implement the highest-confidence validated tweaks first.
Do not flood the app with questionable tweaks.

### Acceptance Criteria
- tweaks apply/revert safely
- service start types can change/revert safely
- every action logs to audit
- compatibility checks are enforced
- recovery path works

---

## Phase 3 — Session Engine + Live Metrics + API Base
### Build
- `core/metrics.py`
- `core/game_detector.py`
- `core/session_manager.py`
- FastAPI skeleton/routers
- `GET /api/status`
- `GET /api/metrics/live`
- `GET /api/tweaks`
- `GET /api/audit`

### Acceptance Criteria
- live SSE metrics stream works
- sessions start/end around game activity
- tweak and audit endpoints function
- integration tests pass

---

## Phase 4 — Frontend Foundation
### Build / Refactor
- layout system
- shared UI primitives
- typed API client
- state store
- Dashboard page
- Optimizations page
- Audit Log page

### Acceptance Criteria
- Electron and backend communicate cleanly
- pages load real backend data
- no fake final data
- core flows feel polished

---

## Phase 5 — Proof Engine / Benchmark Mode
### Build
- `core/benchmark_engine.py`
- benchmark endpoints
- benchmark UI
- evidence/impact visualization

### Acceptance Criteria
- baseline and comparison results persist
- benchmark verdicts render in UI
- benchmark data ties into profiles and evidence flows

---

## Phase 6 — Adaptive Engine V1
### Build
- `core/adaptive_engine.py`
- rule engine
- action cooldowns
- visible action pipeline
- notification support

### Acceptance Criteria
- adaptive rules trigger safely
- actions are logged, visible, revertible
- no action spam
- no unstable loops

---

## Phase 7 — Network Optimizer
### Build
- `core/network_optimizer.py`
- diagnostics service
- NIC tuning abstraction
- protocol-aware logic
- QoS support layer
- network UI panel

### Acceptance Criteria
- protocol-aware status is visible
- diagnostics work
- tweaks are auditable/revertible
- unsupported NIC capabilities show clearly

---

## Phase 8 — GPU Controller + BIOS Advisory
### Build
- `core/gpu_controller.py`
- vendor abstractions
- GPU metrics/settings API
- BIOS advisory service
- GPU UI panel

### Acceptance Criteria
- metrics are visible
- supported settings can be controlled safely
- unsupported vendor/runtime cases surface clearly
- no fake writes

---

## Phase 9 — Game Profiles + Trust Center
### Build
- game profile storage
- profile recommendation logic
- `.pbprofile` import/export
- Trust Center page
- recovery visibility
- expert mode gates

### Acceptance Criteria
- per-game history works
- Trust Center is accurate and useful
- rollback readiness is visible

---

## Phase 10 — Polish, Hardening, Installer Readiness
### Tasks
- review all error states
- tighten UX wording
- harden startup/shutdown
- improve recovery edge cases
- finalize settings behavior
- prepare installer/uninstaller architecture
- ensure revert story is documented

### Acceptance Criteria
- app feels coherent and premium
- no obvious broken flows
- production readiness checklist exists
- docs updated

---

## Delivery Rules
After each phase:
1. summarize changes
2. list changed files
3. list tests run/results
4. list risks/tradeoffs
5. update `PROGRESS.md`
6. continue automatically unless truly blocked