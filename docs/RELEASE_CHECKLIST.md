# Release Checklist

Run before packaging:

```powershell
.\scripts\release-validate.ps1 -Configuration Release
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
