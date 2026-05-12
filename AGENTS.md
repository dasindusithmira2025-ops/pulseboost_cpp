# AGENTS.md - PulseBoost AI Codex Instructions

## Project Identity

PulseBoost AI is a serious Windows-native PC optimization, diagnostics, safety, and AI-advisory desktop application.

Production stack:

- C++20
- Qt 6 Quick/QML
- Win32 / WMI / PDH / Windows system APIs
- SQLite / local persistence
- CMake + Visual Studio 2022
- Local-first AI diagnostics and action routing

The canonical production UI is **Qt/QML under `ui/qml/`**.

This is **not** a Tauri/web-shell product anymore. Tauri, React, Vite, Shadcn exports, Figma Make ZIPs, and design-reference files are reference-only unless a task explicitly says otherwise.

## Prime Directive

Make the product safer, more reliable, more testable, and more release-ready.

Do not chase random new features.

Always preserve:

- safety policy
- dry-run behavior
- confirmation gates
- audit logging
- rollback/restore visibility
- local-first privacy
- AI advisory-first behavior
- real backend-connected UI

## Current Product Priorities

Highest priority work:

1. Release validation cleanup
2. Qt/SQLite warning cleanup
3. Packaging hygiene
4. QML resource correctness
5. Safety-critical UI correctness
6. CI/release pipeline
7. Installer validation
8. Code signing preparation
9. Investor-demo reliability

Lower priority work:

- new optimizer engines
- new AI features
- new visual redesigns
- broad feature expansion
- speculative system tweaks

## Cost-Efficient Working Rules

Before editing, do the smallest useful inspection.

Prefer:

```powershell
git status --short
git diff --stat
git diff --name-only
rg "search term" path/
```
