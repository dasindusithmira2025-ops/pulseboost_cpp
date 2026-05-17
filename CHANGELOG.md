# Changelog

## 2026-04-08 - Local Stitch Export Source-Of-Truth UI Rebuild

### Added

- Local-Stitch-driven rewrites for Trust Center, Network, GPU, Game Profiles, and Settings using the exported screen/code assets under `data/stitch/pulseboost_final_mcp/`

### Changed

- Finished replacing the remaining page layer around the new desktop shell and design tokens instead of carrying forward older mixed page compositions
- Tightened Trust Center into a clearer safety and unsupported-capability review surface
- Rebuilt Network, GPU, and Profiles into calmer evidence-first layouts that stay honest when telemetry or control paths are unsupported
- Reworked Settings into a grouped commercial desktop settings area with category navigation, plan posture, expert gating, and runtime-boundary review

### Known Follow-up Work

- Frontend validation is still build-based rather than desktop E2E
- Billing and licensing remain architecture-ready only
- Python test output still includes existing `anyio` resource warnings

## 2026-04-08 - Hard Reset UI Replacement Pass

### Added

- New shared UI system centered on page heroes, split review surfaces, structured tables, premium locks, and calmer callouts
- New premium desktop shell with grouped navigation, plan posture, and reduced first-screen clutter

### Changed

- Replaced the top-level UI architecture in place instead of extending the previous dashboard-heavy layer
- Rebuilt Dashboard, Optimizations, Network, GPU, Audit Log, Benchmark, Game Profiles, Trust Center, and Settings around a new calmer hierarchy
- Simplified the landing page into machine state, top telemetry, recommendation focus, and trust posture
- Reduced the frontend bundle materially by removing the older dashboard-era component layer

### Removed

- `EfficiencyRing.jsx`
- `HealthRing.jsx`
- `MetricCard.jsx`
- `OptimizationFeed.jsx`
- `PillarSidebar.jsx`
- `ProcessList.jsx`
- `StreamChart.jsx`
- `Timeline.jsx`

### Known Follow-up Work

- Browser automation is still missing
- Billing and licensing remain UI-architecture ready only
- Electron GUI smoke coverage is still manual

## 2026-04-08 - Stitch-Led Dashboard Simplification And Product-Ready Shell Pass

### Added

- Shared page-hero, segmented inspection, and premium-gate primitives for the rebuilt shell
- Plan-aware account surfaces in the sidebar and Settings so premium feature positioning can exist honestly without fake billing

### Changed

- Rebuilt Dashboard around a hero plus staged deep-inspection model so process tables, timeline review, and session history no longer overwhelm the first view
- Refit Optimizations, Benchmark, Audit Log, Trust Center, Settings, Network, GPU, and Game Profiles around the same calmer hierarchy and status language
- Tightened the visual system with stronger page-level hierarchy, premium account styling, and segmented detail views

### Known Follow-up Work

- Bundle chunk warning still remains
- Frontend verification is still build-based rather than E2E
- Commercial billing or licensing flows are still not implemented, only architecturally supported in the UI

## 2026-04-08 - Stitch-Driven Desktop Product Refactor

### Added

- Official Stitch-driven design source mapping in `docs/DESIGN_SYSTEM.md`
- Preferred Electron desktop shell with preload bridge, backend lifecycle management, and PyWebView fallback launcher
- Runtime identity abstraction so backend, Trust Center, and shell surfaces report Electron versus PyWebView honestly

### Changed

- Replaced the top-level React shell with a denser workstation frame, stronger sidebar, desktop-style chrome, context ribbon, and in-shell first-launch or recovery notices
- Updated the visual system toward Stitch's darker tonal hierarchy, tighter density, Inter-first typography stack, and calmer observability presentation
- Reframed Settings and shell microcopy around real runtime state, unsupported capabilities, and desktop behavior instead of the older WebView-only story

### Known Follow-up Work

- Browser automation remains missing
- Installer packaging is still roadmap work
- Bundle chunk size warning still remains

## 2026-04-08 - Full UI Refactor Pass

### Added

- Official Figma redesign source file `PulseBoost AAA Redesign` (`c3fnoUlR1cbDTCtiAGX5lw`)
- Repo-documented Figma structure and implementation priorities in `docs/DESIGN_SYSTEM.md`
- Shared desktop UI primitives for panel notes, signal meters, richer page headers, and denser workspace context surfaces

### Changed

- Rebuilt the app shell around grouped navigation, contextual top-bar actions, a runtime context strip, and a stronger desktop workstation frame
- Refactored Dashboard, Optimizations, Network, GPU, Audit Log, Benchmark, Game Profiles, Trust Center, and Settings into a more coherent premium desktop layout system
- Reworked charts, audit surfaces, copilot framing, page drawers, and supporting microcopy to emphasize trust, technical clarity, and evidence-first UX

### Known Follow-up Work

- Figma MCP Starter-plan rate limiting still restricts large repeated design mutations inside one session
- Frontend validation remains build-based rather than E2E
- Electron migration, GPU writers, QoS writers, and frame-hook benchmark capture remain separate roadmap items

## 2026-04-08 - Desktop Redesign And Productization Pass

### Added

- Repo-local design system source of truth in `docs/DESIGN_SYSTEM.md`
- Modular desktop page architecture under `pulseboost/ui/src/pages`
- In-app confirmation dialog flow and `.pbprofile` import support
- Desktop bridge metadata/window controls test coverage in `pulseboost/tests/test_desktop_shell.py`

### Changed

- Reworked the frontend shell into a denser desktop workstation layout with persistent sidebar, workspace top bar, calmer charts, and unified status language
- Extended the PyWebView bridge with data-directory access and window control hooks while preserving the working runtime

## 2026-04-07 - V1 Completion And Stabilization

### Added

- Foundation platform modules for capabilities, safety, recovery, audit, rollback, and persistence
- Validated optimizer core with tweak apply/revert flows and temporary tweak tracking
- Session engine, metrics SSE, benchmark/proof engine, adaptive engine, network diagnostics, GPU/BIOS surfaces, game profiles, Trust Center, and Settings
- Release-oriented documentation: installer architecture, known limitations, production readiness, testing guide, and release checklist

### Changed

- Preserved the working Python + WebView desktop runtime instead of forcing a risky Electron rewrite into the current ship path
- Hardened startup/shutdown cleanup and recovery reporting
- Stabilized the React shell so all completed pages are reachable from navigation and reflect real backend state
- Tightened UX wording around unavailable features, dry-run controls, Trust Center, and Settings

### Fixed

- Reconciled `PROGRESS.md` so phase status blocks now match the completed changelog
- Fixed hidden-page frontend wiring issues caused by missing imports/state in the app shell
- Improved badge consistency plus loading/empty/error treatment across completed frontend surfaces

### Known Follow-up Work

- Electron runtime parity
- Frame-hook benchmark capture
- Production-safe QoS and GPU writers
- Browser automation coverage
- Installer implementation
