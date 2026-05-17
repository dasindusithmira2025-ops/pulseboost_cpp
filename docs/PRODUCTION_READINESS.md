# Production Readiness Checklist

## Core Runtime
- [x] Foundation DB/bootstrap exists
- [x] Capability detection persists to SQLite
- [x] Audit log is append-only and queryable
- [x] Revert snapshots persist
- [x] Startup recovery path exists
- [x] Temporary tweaks can be restored after unclean exit
- [x] Clean shutdown attempts temporary tweak cleanup

## Product Areas
- [x] Optimizer core with validated tweak catalog
- [x] Session engine and live metrics
- [x] Benchmark/proof engine
- [x] Adaptive engine V1
- [x] Network diagnostics surface
- [x] GPU/BIOS surface
- [x] Game profiles
- [x] Trust Center
- [x] Settings surface for expert mode and adaptive control

## Safety
- [x] Protected process rules enforced
- [x] Unsupported capabilities are explicit
- [x] Dangerous/high-risk surfaces remain gated or dry-run only
- [x] Electron desktop shell path added without removing the existing fallback launcher

## Validation
- [x] Python unit test suite passes
- [x] Frontend production build passes
- [x] Backend compile checks pass
- [x] Electron launcher files parse and the desktop launcher script resolves the runtime path
- [ ] Browser automation / UI E2E coverage
- [ ] Production-safe QoS writer
- [ ] Production-safe GPU settings writer
- [ ] Real frame-hook benchmark integration
- [ ] Installer packaging implementation

## V1 Scope
- Ship Electron as the preferred desktop shell with PyWebView retained as fallback
- Ship safe, auditable optimizer flows with real rollback metadata and honest unsupported states
- Ship benchmark mode with persisted verdicts and unsupported-metric disclosure instead of fabricated FPS data
- Ship network/GPU/profile/trust/settings surfaces that clearly separate supported, dry-run, and unavailable behavior
- Ship release documentation that explains current safety posture and remaining limitations plainly

## Post-V1 Roadmap
- Frame-hook or supported in-game telemetry integration for FPS, 1% low, and frame-time evidence
- Production-safe QoS and GPU setting writers with additional confirmation and capability gating
- Browser automation coverage for the desktop shell and frontend pages
- Installer/uninstaller implementation that preserves the documented revert story

## Ship Notes
- PulseBoost is coherent, auditable, revert-aware, and technically honest in the current repo state.
- Remaining gaps are documented and surfaced as unavailable or dry-run rather than hidden.
