# PulseBoost Page Map (Current UI)

Source of truth:
- `pulseboost/ui/src/App.jsx`
- `pulseboost/ui/src/components/Sidebar.jsx`
- `pulseboost/ui/src/pages/*.jsx`
- `pulseboost/ui/src/api/client.js`

Navigation pages in runtime shell:
- `dashboard`
- `pulsecore`
- `network`
- `gpu`
- `audit`
- `benchmark`
- `profiles`
- `trust`
- `settings`
- `account`

## 1. Global Shell

### App shell (`App.jsx`)
- Purpose:
  - Main desktop shell and page coordinator.
- Route/screen name:
  - Internal page-state switch (not URL router).
- Data dependencies:
  - status, state, metrics, auth, feature access, trust, tweaks, benchmark data, game data.
- Main actions:
  - page refresh, data/log folder open, window controls, shortcut handlers.
- Current status:
  - Working.
- Premium/locked states:
  - Status badges and plan labels shown globally.
- Unsupported handling:
  - Errors surfaced in top-level error banner.

### Sidebar (`Sidebar.jsx`)
- Purpose:
  - Navigation, health indicator, account quick access.
- Data dependencies:
  - `activePage`, `healthScore`, account identity, plan label, signed-in state.
- Main actions:
  - Navigate pages, collapse/expand, jump to settings/account.
- Current status:
  - Working.

### Onboarding overlay (`OnboardingOverlay.jsx`)
- Purpose:
  - First-run onboarding sequence.
- Trigger:
  - localStorage key `pulseboost.onboarding.completed.v2`.
- Status:
  - Working.

## 2. Page-by-Page Mapping

## Dashboard
- Screen key: `dashboard`
- Component: `DashboardPage.jsx`
- Purpose:
  - High-level machine/trust/recommendation overview.
- Data dependencies:
  - `activeSession`, `connected`, `healthScore`, `metrics`, `trustCenter`, `visibleOptimizations`.
- Main actions:
  - Jump to PulseCore, AI chat focus, audit page.
- Backend/API dependencies:
  - `/api/status`, `/api/state`, `/api/metrics`, `/api/trust-center/status`.
- Current status:
  - Working.
- Premium/locked states:
  - none directly enforced in this page.
- Unsupported handling:
  - missing trust timestamps/capabilities shown as `Unavailable`/fallback text.

## PulseCore (Optimizations + AI)
- Screen key: `pulsecore`
- Component: `PulseCorePage.jsx`
- Purpose:
  - Core optimization operations, boost cycle, tweak timeline, smart suggestions, AI chat.
- Data dependencies:
  - tweak catalog, audit entries, health history, metrics, gpu stats, smart suggestions, active session, foundation hardware/profile status.
- Main actions:
  - Apply/revert tweak
  - Revert all temporary tweaks
  - Run Boost Now flow
  - Trigger suggestion actions
  - Chat with PulseAI
- Backend/API dependencies:
  - `/api/tweaks`
  - `/api/tweaks/{id}/apply`
  - `/api/tweaks/{id}/revert`
  - `/api/optimizations`, `/api/optimizations/decision`
  - `/api/suggestions`
  - `/api/health/history`
  - `/ws` for chat tokens
- Current status:
  - Working.
- Premium/locked states:
  - indirect through feature access and optimization routes.
- Unsupported handling:
  - tweak errors shown inline; blocked actions surface server messages.

## Network
- Screen key: `network`
- Component: `NetworkPage.jsx`
- Purpose:
  - Network diagnostics, protocol posture, qos control surface.
- Data dependencies:
  - diagnostics payload (`summary`, `targets`, `nic_capabilities`, `qos`, `protocol_profile`), feature access.
- Main actions:
  - Run diagnostic refresh
  - Apply Low Latency / Balanced QoS profiles
- Backend/API dependencies:
  - `/api/network/diagnostics`
  - `/api/network/qos`
- Current status:
  - Diagnostics: Working (capability-limited)
  - QoS apply: Dry-run
- Premium/locked states:
  - QoS action buttons disabled unless `advanced_network_controls` true.
- Unsupported handling:
  - Explicit `UNSUPPORTED`/`DRY_RUN` badges and backend notes displayed.

## GPU
- Screen key: `gpu`
- Component: `GpuPage.jsx`
- Purpose:
  - GPU telemetry, optimization settings surface, driver links, BIOS advisory.
- Data dependencies:
  - `gpuStats`, `biosChecklist`, feature access.
- Main actions:
  - Apply/review setting
  - Open vendor download links
- Backend/API dependencies:
  - `/api/gpu/stats`
  - `/api/gpu/settings`
  - `/api/bios/checklist`
- Current status:
  - Telemetry: Working (runtime-dependent)
  - Settings apply: Dry-run
  - BIOS advisory: Working
- Premium/locked states:
  - settings apply buttons disabled unless `advanced_gpu_controls` true.
- Unsupported handling:
  - telemetry support note and reason text shown when unavailable.

## Audit Log
- Screen key: `audit`
- Component: `AuditPage.jsx`
- Purpose:
  - Full audit timeline/list with details and revert entry action.
- Data dependencies:
  - `auditEntries`, feature access.
- Main actions:
  - Search and inspect entry details
  - Revert entry (if snapshot-backed)
  - Export audit CSV
- Backend/API dependencies:
  - `/api/audit`
  - `/api/audit/{id}/revert`
  - `/api/actions/export`
- Current status:
  - Working.
- Premium/locked states:
  - Export button disabled when `audit_export` locked.
- Unsupported handling:
  - non-revertible rows disable revert action.

## Benchmark
- Screen key: `benchmark`
- Component: `BenchmarkPage.jsx`
- Purpose:
  - Run benchmark captures, inspect latest/history, compare proof sets, export results.
- Data dependencies:
  - benchmark results, selected benchmark, tweak catalog, benchmark form state.
- Main actions:
  - Run benchmark
  - Select history row
  - Export benchmark report
  - Configure compare workload/duration/tweak set
- Backend/API dependencies:
  - `/api/benchmark/run`
  - `/api/benchmark/results`
  - `/api/benchmark/results/{id}`
  - `/api/benchmark/results/{id}/export`
- Current status:
  - Working (capability-limited).
- Premium/locked states:
  - benchmark history and run route usage rely on entitlement gate; app routes to Account page when locked.
- Unsupported handling:
  - UI notes explicitly mention unsupported metrics remain explicit.

## Game Profiles
- Screen key: `profiles`
- Component: `ProfilesPage.jsx`
- Purpose:
  - Manage per-game optimization profiles and import/export.
- Data dependencies:
  - game catalog, selected game profile, feature access.
- Main actions:
  - Create profile
  - Edit profile fields
  - Save profile
  - Import/export `.pbprofile`
  - Activate/deactivate profile selection
- Backend/API dependencies:
  - `/api/games`
  - `/api/games/{id}/profile` (GET/POST)
  - `/api/games/{id}/export`
  - `/api/games/import`
- Current status:
  - Working.
- Premium/locked states:
  - Community tab visually marked premium.
- Unsupported handling:
  - Community feature explicitly says unavailable in current desktop build when entitlement might allow.

## Trust Center
- Screen key: `trust`
- Component: `TrustPage.jsx`
- Purpose:
  - Safety posture and rollback command center.
- Data dependencies:
  - trust payload (`rollback_readiness`, `touch_matrix`, `permission_audit`, `expert_mode_state`).
- Main actions:
  - Revert all temporary changes
  - Undo category changes
  - Toggle expert mode
- Backend/API dependencies:
  - `/api/trust-center/status`
  - `/api/trust-center/rollback-all`
  - `/api/trust-center/undo/{category}`
  - `/api/settings/expert-mode`
- Current status:
  - Working.
- Premium/locked states:
  - none primary; this is trust/safety surface.
- Unsupported handling:
  - unsupported categories cannot be undone; route returns clear 400.

## Settings
- Screen key: `settings`
- Component: `SettingsPage.jsx`
- Purpose:
  - App preferences, adaptive toggle, safety/expert controls, data actions, shortcut reference.
- Data dependencies:
  - settings preferences payload, adaptive state, expert mode, data action state.
- Main actions:
  - toggle preferences
  - toggle expert mode
  - trigger data actions (export/import/reset/clear history)
- Backend/API dependencies:
  - `/api/settings`
  - `/api/settings/preferences`
  - `/api/settings/expert-mode`
  - `/api/settings/data-action`
  - `/api/adaptive/toggle`
- Current status:
  - Working.
- Premium/locked states:
  - none primary.
- Unsupported handling:
  - invalid imports are surfaced via backend error and local message state.

## Account
- Screen key: `account`
- Component: `AccountPage.jsx`
- Purpose:
  - Identity/session/plan/entitlement view and auth entry points.
- Data dependencies:
  - `authStatus`, `featureAccess`, account links, current plan.
- Main actions:
  - local placeholder sign-in (dev mode only)
  - website sign-in/create-account/manage-subscription link open
  - refresh entitlements
  - sign out
  - export report action
- Backend/API dependencies:
  - `/api/auth/status`
  - `/api/auth/local-session`
  - `/api/auth/refresh-entitlements`
  - `/api/auth/sign-out`
  - `/api/account/identity`
  - `/api/account/plan`
  - `/api/account/entitlements`
  - `/api/account/activation`
  - `/api/settings/data-action` (`export_report`)
- Current status:
  - Local account foundation: Working
  - Website authority: Future-dependent
- Premium/locked states:
  - feature rows marked included/locked from `featureAccess` map.
- Unsupported handling:
  - website actions disabled when authority URLs not configured.

## 3. Cross-Page Runtime Signals

Shared status patterns surfaced across UI:
- plan and sign-in badges in shell header
- `featureAccess` controls premium locks and reroutes
- dry-run and unsupported tags displayed by feature pages
- global error banner for API failures
- page-specific fallback text for missing telemetry/capability data

## 4. Desktop and IPC Surface Used by Pages

`window.pulseboostDesktop` methods used in UI:
- `getMeta`
- `openLogs`
- `openDataDir`
- `openExternal`
- `restartGuardian`
- `minimizeWindow`
- `toggleMaximizeWindow`
- `closeWindow`
- `onShortcut`

Shortcut actions consumed by `App.jsx`:
- `boost_now`
- `focus_chat`
- `go_pulsecore`
- `dismiss_dialog`

## 5. Page-State Risks and Notes

- Navigation is state-driven, not URL-routed; deep-linking support is minimal.
- Several destructive/important actions still use native `window.confirm` dialogs.
- Data refresh cadence differs by page (dashboard/pulsecore more frequent), which is good for performance but requires awareness during demos.
