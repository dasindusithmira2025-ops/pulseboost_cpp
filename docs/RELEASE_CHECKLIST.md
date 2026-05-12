# Release Checklist

Run before packaging:

```powershell
.\scripts\release-validate.ps1 -Configuration Release
.\scripts\package-validate.ps1 -Configuration Release
```

## Required Gates

- Release build completes.
- Safety tests pass through CTest.
- CLI smoke tests return strict JSON with `ok: true`.
- Qt/QML production screens are included in `qml.qrc`.
- Deprecated `tauri_ui` paths are not included in release resources or tracked production source.
- No generated build output is committed.
- No local SQLite databases, logs, JSONL history, `__pycache__`, `.pyc`, secrets, or generated runtime files are committed.
- Action Center can dry-run supported actions.
- Audit Log loads SQLite `action_audit_log` entries.
- Restore / Undo Center shows snapshots, quarantine inventory, and rollback entries.
- AI assistant remains advisory-first for high-risk actions.

## Demo Build Notes

Use the Qt/QML app as the only production UI. The release should be built from source using CMake presets and validated with the script above. Do not package stale local runtime state.

## Known Validation Warnings

No known validation warnings after Phase 2.7. CLI/headless commands now create a Qt application object before SQLite-backed safety/audit code can run, and SQLite connections are closed with query objects out of scope before `removeDatabase()`.

## QML Release Scope

Production top-level screens in the current shell:

- `Home`
- `ActionCenter`
- `AiChat`
- `HealthHistory` as before/after proof
- `AuditLog`
- `RestoreCenter`
- `ProcessManager`
- `StartupManager`
- `StorageAnalyzer`
- `NetworkMonitor`
- `Settings`

Production advanced/internal screens retained in `qml.qrc`:

- `Tools`
- `Optimizations`
- `BoostUp`
- `Backup`
- `Dashboard`
- `Charts`
- `GameMode`
- `RamOptimizer`
- `Temps`
- `Games`
- `DriverManager`
- `SecurityScanner`
- `Onboarding`

These retained screens must not be treated as deprecated Tauri/web surfaces. Any screen promoted back to top-level navigation needs safety-copy review for dry-run, confirmation, rollback, and audit visibility.

## Phase 2.8 Installer And Package Validation

- Package validation: run `scripts/package-validate.ps1 -Configuration Release` after the Release build and Qt deployment.
- Installer validation: confirm `PulseBoost_Installer.iss` packages `PulseBoostAI.exe`, Qt runtime DLLs, required plugin directories, and QML runtime resources only.
- Clean-machine validation: follow `docs/CLEAN_MACHINE_TEST_PLAN.md` on a Windows 10 or Windows 11 VM before sharing an alpha build.
- Signing preparation: follow `docs/CODE_SIGNING.md`; do not use or commit real certificates, passwords, or signing secrets until the owner provides the signing channel.
- Uninstall validation: verify the app directory and shortcuts are removed, with only documented user app data retained.
- Release artifact naming: use `PulseBoostAI_Setup.exe` for the Inno Setup output and include the version in any externally shared filename, for example `PulseBoostAI_Setup_1.0.0_alpha.exe`.
- Known limitations: builds are unsigned until code signing is provisioned; SmartScreen reputation is expected to be limited for early alpha packages; clean-machine validation must confirm VM-specific restore point availability.
