# PulseBoost Design System

## Source Of Truth
- Primary in-repo source for the latest rebuild: local Stitch export under `data/stitch/pulseboost_final_mcp/`
  - `design-system.md`
  - `manifest.json`
  - downloaded screen HTML under `code/`
  - downloaded screen imagery under `images/`
- Primary design project: Stitch `PulseBoost Desktop UI/UX Prototype`
- Stitch project id: `projects/1416118724942608660`
- Design system asset: `PulseBoost Kinetic` (`assets/73568073445342e28cc8a0f668e71347`)
- Current environment note: this chat session could not enumerate the Stitch MCP server directly through the resource helper, but the connected Stitch project mapping, screen ids, and theme brief are already captured in-repo from the authenticated Stitch workflow and were used as the source of truth for this pass.
- Repo-local fallback: this document mirrors the Stitch structure and remains the implementation reference when direct server mutation or enumeration is not practical in-session.

## Stitch Structure
- Design system: `PulseBoost Kinetic`
  - colors, typography, tonal surfaces, compact density, buttons, inputs, charts, tables, and status badge logic
- Core screens:
  - `Dashboard - PulseBoost` -> `2080f7bf73ea4f2eab4506ff89881aa1`
  - `Optimizations - PulseBoost` -> `1a086b95a849408fb731d8f3970aa6bf`
  - `Network - PulseBoost` -> `96d25af7dee84a87badaf758cdc54af7`
  - `GPU - PulseBoost` -> `833d22c766ee424191199d54bf4d3a1f`
  - `Audit Log - PulseBoost` -> `5a239f182790432ea7669920ef9ffd74`
  - `Benchmark - PulseBoost` -> `b9d9a991102749ca8f1a3dac69f37128`
  - `Game Profiles - PulseBoost` -> `83e68d5558374f229fda6aaeedf5c619`
  - `Trust Center - PulseBoost` -> `d77dfeec11b04909b85c4b49a82ce2e5`
  - `Settings - PulseBoost` -> `f7fd8b687aab48b4bab88ae2a88088b0`
  - `PulseBoost System Optimizer` -> `fb6f15cb99474e99a26672bb88206974`
- Shell direction pulled from Stitch theme guidance:
  - "The Sovereign Observatory" creative direction
  - tonal architecture instead of visible section borders
  - compact, industrial density
  - dark observability surfaces with indigo accent emphasis

## Product Direction
PulseBoost should feel like a premium Windows workstation for performance tuning, observability, and safe rollback.

The shell should communicate:
- density over decoration
- trust over hype
- control over mystery
- evidence over marketing
- safety over aggression
- fewer things visible at once
- staged drill-downs instead of dashboard walls
- monetization-ready polish without fake billing

## Core Tokens

### Color
- App background: `#0f1117`
- Base surface: `#1a1d27`
- Raised surface: `#202433`
- Deep surface: `#151925`
- Accent: `#6366f1`
- Accent soft: `#818cf8`
- Text primary: `#edf1ff`
- Text secondary: `#a7afc7`
- Text muted: `#7b839c`
- Border: `rgba(167, 175, 199, 0.16)`
- Success: `#34d399`
- Warning: `#f59e0b`
- Danger: `#f87171`
- Info: `#38bdf8`

### Typography
- UI body: `"Inter", "Segoe UI Variable Text", "Segoe UI", sans-serif`
- Display/headings: `"Inter", "Segoe UI Variable Display", "Segoe UI", sans-serif`
- Mono/data: `"JetBrains Mono", "Cascadia Code", monospace`

### Density
- Desktop-first spacing rhythm: `8 / 12 / 16 / 20 / 24`
- Default card padding: `18px`
- Tight card padding: `14px`
- Sidebar width target: `272px`
- Content max width: none; prefer wide workstation layouts

## Surfaces
- Sidebar: dark, layered, persistent, operational
- Title bar: compact, desktop-grade, includes page title and contextual actions
- Content cards: crisp panels with restrained elevation
- Tables: bordered, high-legibility, low ornament
- Drawers/detail panels: right-side contextual inspection surfaces

## Status Badge System
- `VALIDATED`: success tone
- `LEGACY`: warning tone
- `HARDWARE_SPECIFIC`: info tone
- `PLACEBO_RISK`: danger tone
- `ACTIVE`: accent tone
- `REVERTED`: muted info tone
- `FAILED`: danger tone
- `AI_APPLIED`: accent tone
- `TEMPORARY`: info tone
- `REQUIRES_ADMIN`: warning tone
- `UNSUPPORTED`: muted warning tone
- `EXPERT_ONLY`: warning tone
- `DRY_RUN`: muted tone
- `LIVE`: accent tone
- `SAFE_DEFAULT`: success tone
- `RECOVERY_MODE`: warning tone

## Shell Structure
- Narrow left sidebar with grouped navigation, machine summary, and plan posture
- Compact workspace header with runtime actions and desktop controls
- Thin context strip for session, trust, and plan state
- One primary page workspace at a time; page detail lives in split layouts and review rails rather than in stacked dashboards
- Toast stack for action outcomes
- Modal confirmations for risky or revert actions
- First-launch, recovery, unsupported-capability, and locked-premium states remain explicit in-product

## Page Direction

### Dashboard
- Mission-control hero at the top with four high-signal machine and session metrics
- Key telemetry is reduced to a small pressure set instead of a wall of cards
- Recommendations remain visible, but process review and session history move into staged deep-inspection tabs
- Copilot remains present but calmer and more utility-like
- No large internal monitoring wall on the landing view

### Optimizations
- Catalog becomes a real control surface
- Each tweak shows rationale, exact state, risk/admin/temporary markers, and before/after values
- Right-side detail panel explains the selected tweak
- Plan-aware bundle surfaces are shown as locked rather than faked

### Network
- Overview metrics first, then target diagnostics, then NIC capability surface
- QoS remains explicit when dry-run

### GPU
- Telemetry and settings clearly separated
- Advisory-only firmware/BIOS guidance visually distinct from live controls

### Audit
- Strongest trust page after Trust Center
- Event list plus detail panel
- Export and revert-oriented language stays explicit

### Benchmark
- Setup, verdict, and history each get their own clear surface
- Unsupported metrics are treated as evidence, not absence

### Game Profiles
- Catalog + profile editor + import/export workflow
- Historical gains and recommendation basis emphasized

### Trust Center
- High-confidence safety dashboard
- Admin, rollback, recovery, safeguards, unsupported capabilities, and expert-mode posture all visible immediately

### Settings
- Group safe defaults, session behavior, adaptive control, and expert mode separately
- Runtime constraints remain visible, including Electron preferred runtime and PyWebView fallback
- Add account or plan posture and premium-ready gates without inventing billing behavior

## Interaction Rules
- Prefer inline explanation before dangerous actions
- Use in-app modal confirmations instead of native browser confirms
- Keep motion subtle: fade, rise, and panel reveal only
- Unsupported and dry-run states must remain first-class and readable
- Prefer staged disclosure over long landing pages; deep operational detail belongs behind tabs, drawers, or dedicated surfaces

## Replacement Scope
The hard reset implementation fully replaced:
- `App.jsx`
- `styles/globals.css`
- all page files under `ui/src/pages`
- `PagePrimitives.jsx`
- `AlertBanner.jsx`
- `AuditLog.jsx`
- `ConfirmDialog.jsx`
- `AIPanel.jsx`
- `StatusBadge.jsx`

The old dashboard-era component set was removed:
- `EfficiencyRing.jsx`
- `HealthRing.jsx`
- `MetricCard.jsx`
- `OptimizationFeed.jsx`
- `PillarSidebar.jsx`
- `ProcessList.jsx`
- `StreamChart.jsx`
- `Timeline.jsx`

## Implementation Priorities
1. Shell first: grouped sidebar, plan-aware account rail, top bar, context strip, and denser workstation spacing
2. Shared primitives: page hero, section headers, notes, segmented inspection controls, status badges, tables, toasts, and dialogs
3. Signature pages: Dashboard, Audit Log, Benchmark, Trust Center
4. Operational pages: Optimizations, Network, GPU, Profiles, Settings
5. Validation: keep backend/API/runtime behavior unchanged while build and test coverage stay green
