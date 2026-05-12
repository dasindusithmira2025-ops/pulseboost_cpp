# PulseBoost AI — Figma Make / React Export to Qt/QML Handoff

Source inspected: `Replicate Existing Design.zip`

## 1. Decision

The uploaded design export should be used as a **visual and structural reference**, not as production application code.

The export is a Vite + React + Shadcn-style project with:

- `src/app/components/Root.tsx` — top bar, sidebar, navigation shell
- `src/app/components/screens/` — 11 generated product screens
- `src/app/components/shared/` — reusable `HealthScore`, `MetricCard`, `RiskBadge`
- `src/styles/theme.css` — core color/design tokens
- `src/imports/pasted_text/pulseboost-ai-design.md` — original UI design prompt
- `package.json` — web dependencies such as React, Vite, Radix, Lucide, Recharts, MUI

Your production application is **C++20 + Qt/QML**, so do not copy the web implementation directly into the product. Rebuild the design natively in QML using the existing PulseBoost CPP backend and Phase 2 safety behavior.

---

## 2. Extracted Design Tokens

These tokens are taken from the generated theme and should be recreated as a QML theme singleton or constants file.

### Core Colors

| Token | Value | Usage |
|---|---:|---|
| Background | `#0a0e1a` | Main window background |
| Foreground | `#e8eaed` | Primary text |
| Card | `#13172b` | Card backgrounds |
| Secondary / Muted | `#1e2338` | Secondary cards, hover states, panels |
| Muted Text | `#8b92a8` | Helper text, labels |
| Primary Accent | `#00d9ff` | Active navigation, primary buttons, focus ring |
| Primary Foreground | `#0a0e1a` | Text on cyan buttons |
| Border | `rgba(255,255,255,0.1)` | Card and panel borders |
| Sidebar | `#0f1220` | Left navigation background |
| Sidebar Accent | `#1e2338` | Version card / hover states |
| Sidebar Border | `rgba(255,255,255,0.08)` | Sidebar divider |

### Risk Colors

| Risk | Value | QML token name |
|---|---:|---|
| Safe | `#00ff88` | `riskSafe` |
| Low | `#00d9ff` | `riskLow` |
| Medium | `#ffb800` | `riskMedium` |
| High | `#ff4757` | `riskHigh` |
| Manual | `#a78bfa` | `riskManual` |

### Chart Colors

| Chart Token | Value |
|---|---:|
| Chart 1 | `#00d9ff` |
| Chart 2 | `#00ff88` |
| Chart 3 | `#ffb800` |
| Chart 4 | `#ff4757` |
| Chart 5 | `#a78bfa` |

### Typography

Base size: `16px`.

Recommended QML sizes:

| Role | Size | Weight |
|---|---:|---:|
| Page title | 24 | 600 |
| Section title | 20 | 600 |
| Card title | 18 | 500 |
| Body | 16 | 400 |
| Small label | 14 | 400/500 |
| Tiny metadata | 12 | 400 |
| Metric value | 28–36 | 600/700 |

### Radius / Spacing

Generated radius token: `0.5rem`, equal to roughly `8px`.

Recommended QML scale:

| Token | Value |
|---|---:|
| Radius small | 4 |
| Radius medium | 6 |
| Radius large | 8 |
| Radius XL | 12 |
| Gap XS | 4 |
| Gap SM | 8 |
| Gap MD | 12 |
| Gap LG | 16 |
| Gap XL | 24 |
| Sidebar width | 256 |
| Top bar height | 56 |

---

## 3. Navigation Structure

Generated navigation list from `Root.tsx`:

1. Overview
2. Action Center
3. AI Advisor
4. Before / After Proof
5. Audit Log
6. Restore Center
7. Processes
8. Startup Apps
9. Storage Cleanup
10. Network Tools
11. Settings

Qt/QML equivalent:

- Use `ApplicationWindow` or root `Rectangle`.
- Use a `RowLayout` for sidebar + main content.
- Use a fixed `AppSidebar.qml` width of `256`.
- Use `StackView`, `Loader`, or existing `ContentArea.qml` to swap screens.
- Keep screen state centralized through existing `UiController`.

---

## 4. Core QML Components to Build

Create:

```text
ui/qml/theme/Theme.qml
ui/qml/components/AppSidebar.qml
ui/qml/components/TopStatusBar.qml
ui/qml/components/MetricCard.qml
ui/qml/components/HealthScoreRing.qml
ui/qml/components/RiskBadge.qml
ui/qml/components/AppCard.qml
ui/qml/components/ActionCard.qml
ui/qml/components/ActionDetailPanel.qml
ui/qml/components/ConfirmationDialog.qml
ui/qml/components/DryRunResultDialog.qml
ui/qml/components/AuditTable.qml
ui/qml/components/AuditDetailDrawer.qml
ui/qml/components/RestoreCard.qml
ui/qml/components/ProofComparisonCard.qml
ui/qml/components/AiRecommendationCard.qml
```

### Theme.qml

Expose all design tokens:

```qml
pragma Singleton
import QtQuick

QtObject {
    readonly property color background: "#0a0e1a"
    readonly property color foreground: "#e8eaed"
    readonly property color card: "#13172b"
    readonly property color secondary: "#1e2338"
    readonly property color mutedText: "#8b92a8"
    readonly property color primary: "#00d9ff"
    readonly property color primaryForeground: "#0a0e1a"
    readonly property color border: "#1AFFFFFF"
    readonly property color sidebar: "#0f1220"
    readonly property color sidebarAccent: "#1e2338"
    readonly property color riskSafe: "#00ff88"
    readonly property color riskLow: "#00d9ff"
    readonly property color riskMedium: "#ffb800"
    readonly property color riskHigh: "#ff4757"
    readonly property color riskManual: "#a78bfa"
    readonly property int radius: 8
    readonly property int topBarHeight: 56
    readonly property int sidebarWidth: 256
}
```

### AppSidebar.qml

Properties:

- `currentPage: string`
- `onNavigate(pageId)` signal
- `versionText: string`

States:

- active item: cyan background, dark text, subtle glow
- inactive item: muted/foreground text with hover background

Implementation notes:

- Use `ListView` or `Repeater` over a model.
- Icons can use existing icon font/assets, simple SVGs, or text glyph fallback.
- Footer shows version card: `Version 2.5.0 Pro` in generated design. Replace this later with real app version.

### TopStatusBar.qml

Content:

- Shield icon
- `PulseBoost AI`
- green pulsing indicator: `System Protected`
- last scan time
- optional `Run Safe Scan` button

### RiskBadge.qml

Props:

- `level: string` — `safe`, `low`, `medium`, `high`, `manual`
- `size: string` — `sm`, `md`

Behavior:

- rounded pill
- small colored dot
- text label:
  - Safe
  - Low Risk
  - Medium Risk
  - High Risk
  - Manual Review

### MetricCard.qml

Props:

- `title`
- `value`
- `subtitle`
- `trend`
- `iconName`
- `iconColor`

Layout:

- card background `Theme.card`
- border `Theme.border`
- left: text content
- right: icon badge in `primary` tint
- hover border: primary with low opacity

### HealthScoreRing.qml

Rebuild the React SVG ring using QML `Canvas` or `Shape`.

Props:

- `score: int`
- `size: string`

Colors:

- `>= 90` safe green
- `>= 70` cyan
- `>= 50` amber
- `< 50` red

### AppCard.qml

Base reusable card:

- `color: Theme.card`
- `border.color: Theme.border`
- `radius: Theme.radius`
- optional hover/focus states
- padding controlled by `default property alias content`

---

## 5. Screen Mapping

### Overview.qml

Use:

- `TopStatusBar`
- `HealthScoreRing`
- `MetricCard` grid
- `AppCard` for Recommended Safe Actions
- `AppCard` for Safety Status
- AI summary panel

Data source:

- existing telemetry snapshot from `UiController`
- recommended safe actions from Phase 2 action metadata

### ActionCenter.qml

Use:

- filters: Safe / Manual / Advanced / High Risk
- `ActionCard` list
- `ActionDetailPanel`
- `RiskBadge`
- `DryRunResultDialog`
- `ConfirmationDialog`

Critical UX:

- Apply button disabled until dry-run passes.
- High-risk actions must show `Manual Review Required`.
- AI must only prepare recommendations, not silently execute.

### AIAdvisor.qml

Use:

- chat-style diagnostic panel
- `AiRecommendationCard`
- safety banner: “AI Advisory Mode: AI can recommend actions, but high-risk actions require your confirmation.”

Critical UX:

- AI can call `prepareDryRun`, not `executeHighRiskAction` directly.

### BeforeAfterProof.qml / ProofReport.qml

Use:

- `ProofComparisonCard`
- simple line/bar charts
- timeline: Scan → Dry Run → Restore Point → Apply → Verified Result
- Export Report button

### AuditLog.qml

Use:

- `AuditTable`
- `AuditDetailDrawer`
- filters: Today, This Week, High Risk, Rollback Available, Failed Actions

Data source:

- SQLite audit log reads already added in Phase 2.

### RestoreCenter.qml

Use:

- `RestoreCard`
- timeline of available restore options
- confirmation modal for restore actions

Data source:

- snapshots, quarantine, rollback audit entries

### Processes.qml

Use:

- table: CPU, RAM, disk, publisher, path, safety status
- `Analyze Process`
- `End Process` as high-risk, confirmation-required

### StartupApps.qml

Use:

- startup impact score
- app list with Disable / Delay / Ignore
- rollback indicators

### StorageCleanup.qml

Use:

- category cards: temp files, browser cache, recycle bin, logs, quarantine
- preview before cleanup
- default action: Move to Quarantine

### NetworkTools.qml

Use:

- diagnostic cards
- DNS / latency / adapter / TCP status
- global changes marked Advanced / Manual Review Required

### Settings.qml

Use:

- AI mode: Local only / Cloud optional / Disabled
- safety gates
- audit logging toggle
- quarantine duration
- restore point behavior
- theme controls
- privacy section

---

## 6. Codex Implementation Prompt

Paste this into Codex after Phase 2 is committed:

```text
Use the uploaded Figma Make export `Replicate Existing Design.zip` as visual reference only.

Do not import or ship the React/Vite code. Rebuild the design natively inside the existing `pulseboost_cpp` Qt/QML app.

Context:
- The export contains a React/Vite/Shadcn-style desktop UI.
- Key files in the export:
  - src/app/components/Root.tsx
  - src/app/components/screens/*.tsx
  - src/app/components/shared/HealthScore.tsx
  - src/app/components/shared/MetricCard.tsx
  - src/app/components/shared/RiskBadge.tsx
  - src/styles/theme.css
- Production target is C++20 + Qt/QML.
- Existing Phase 2 work already added ActionCenter.qml, AuditLog.qml, RestoreCenter.qml, before/after proof reports, confirmation gates, SQLite audit reads, restore inventory, and AI advisory-first behavior.

Goal:
Polish the existing Qt/QML UI to match the generated design system while preserving all backend behavior.

Rules:
1. Do not add new optimizer features.
2. Do not use React, Vite, Tailwind, Radix, MUI, Recharts, or web runtime code in the production app.
3. Do not break existing C++ backend or UiController APIs.
4. Keep safety-first UX: dry-run before apply, confirmation for medium/high risk, rollback/audit visibility.
5. High-risk actions must never look casual.
6. Use QML reusable components and theme tokens.

Tasks:
1. Add a QML theme singleton or theme constants file using the tokens:
   - background #0a0e1a
   - foreground #e8eaed
   - card #13172b
   - secondary #1e2338
   - mutedText #8b92a8
   - primary #00d9ff
   - riskSafe #00ff88
   - riskLow #00d9ff
   - riskMedium #ffb800
   - riskHigh #ff4757
   - riskManual #a78bfa
   - sidebar #0f1220
   - radius 8
   - sidebarWidth 256
   - topBarHeight 56

2. Create reusable QML components:
   - AppSidebar.qml
   - TopStatusBar.qml
   - AppCard.qml
   - MetricCard.qml
   - HealthScoreRing.qml
   - RiskBadge.qml
   - ActionCard.qml
   - ActionDetailPanel.qml
   - ConfirmationDialog.qml
   - DryRunResultDialog.qml
   - AuditTable.qml
   - AuditDetailDrawer.qml
   - RestoreCard.qml
   - ProofComparisonCard.qml
   - AiRecommendationCard.qml

3. Refactor existing screens to use these components:
   - Overview.qml
   - ActionCenter.qml
   - AIAdvisor.qml
   - BeforeAfterProof.qml or ProofReport.qml
   - AuditLog.qml
   - RestoreCenter.qml
   - Processes.qml
   - StartupApps.qml
   - StorageCleanup.qml
   - NetworkTools.qml
   - Settings.qml

4. Match the generated layout:
   - 56px top status bar
   - 256px sidebar
   - dark navy background
   - cyan active nav item
   - card-based content
   - risk badges
   - readable tables
   - investor-demo proof/report sections

5. Update qml.qrc or CMake QML resource registration for every new QML file.

6. Run validation:
   - cmake --build --preset build-release
   - ctest --test-dir build -C Release --output-on-failure
   - .\scripts\test.ps1 -Configuration Release
   - .\scripts\release-validate.ps1 -Configuration Release

7. Do not commit until all checks pass.
```

---

## 7. Production Warning

The generated export is useful, but it includes web dependencies that should not enter the final Windows optimizer product:

- React
- Vite
- Tailwind
- Radix UI
- Shadcn wrappers
- MUI
- Recharts
- browser routing

Your production code should stay native:

```text
C++ backend + Qt/QML UI + SQLite audit + safety gates + local telemetry
```

---

## 8. Recommended Commit Plan

After implementing the UI polish in QML:

```bash
git checkout -b ui/figma-inspired-qml-polish
git add ui/qml CMakeLists.txt qml.qrc docs
git diff --cached --check
git commit -m "Polish Qt/QML UI from Figma design system"
git push origin ui/figma-inspired-qml-polish
```

Do not merge until all validation passes and the GUI is manually checked.
