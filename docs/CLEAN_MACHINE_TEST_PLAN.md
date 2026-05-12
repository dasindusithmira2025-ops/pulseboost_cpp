# Clean Machine Test Plan

Use a clean Windows 10 or Windows 11 VM with no local PulseBoost development tools, build outputs, databases, or logs.

## Install And Launch

1. Copy the alpha installer to the VM.
2. Install PulseBoost AI into the default `Program Files` directory.
3. Launch PulseBoost AI from the Start Menu shortcut.
4. Launch PulseBoost AI from the desktop shortcut if that installer task was selected.
5. Confirm the app opens without missing DLL, plugin, or QML errors.

## Functional Smoke Test

1. Run a safe scan.
2. Open every top-level screen in the production shell.
3. Run a dry-run cleanup.
4. Verify the confirmation dialog appears before any non-dry-run action.
5. Create a restore point if Windows allows it on the VM.
6. Open Audit Log and confirm actions are visible.
7. Open Restore Center and confirm restore or rollback visibility is present.
8. Run:

```powershell
& "C:\Program Files\PulseBoost AI\PulseBoostAI.exe" --validate-qml
```

## Uninstall Test

1. Uninstall PulseBoost AI from Windows Settings or the Start Menu uninstall shortcut.
2. Verify the install directory is removed.
3. Verify shortcuts are removed.
4. Verify no unwanted files remain in the install directory.
5. Document any remaining app data under the user profile or `ProgramData` if it is intentionally retained for audit history, restore metadata, or local settings.

## Failure Capture

For any failure, capture the VM Windows version, installer filename, exact action, screenshot if applicable, and any relevant Event Viewer or application log details.
