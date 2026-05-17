# Installer And Revert Architecture

## Runtime Layout
- Desktop shell: Python + WebView launcher
- Backend: FastAPI on localhost
- Frontend: built Vite assets served by FastAPI
- Persistence: single SQLite database at `data/memory.db`

## Installer Responsibilities
- Install the desktop launcher and backend runtime together
- Create the application data directory with write permission for the current user
- Register a Start Menu shortcut and optional desktop shortcut
- Avoid changing PowerShell execution policy globally
- Preserve the database, exported profiles, and audit history during in-place upgrades unless the user explicitly chooses a reset

## Uninstaller Responsibilities
- Stop the running PulseBoost process cleanly before removing files
- Offer to keep or delete user data
- If deleting user data, remove the SQLite DB, vector store, logs, and exported temporary assets only after process shutdown
- Remove shortcuts and any optional startup entries created by the installer

## Revert Story
- Registry/service/process-affinity/priority changes must capture pre-change state before apply
- Temporary tweaks must be restored on clean shutdown when possible
- If the app exits uncleanly, startup recovery must review and restore temporary tweaks before reuse
- Audit entries remain append-only and should survive uninstall when the user keeps app data

## Release Constraints
- The supported runtime for V1 remains Python + WebView
- Electron remains a tracked architectural target, but migrating runtimes should be handled as a dedicated project rather than folded into installer work
- GPU writes, QoS writes, and frame-hook benchmark capture are not installer blockers because they are intentionally shipped as dry-run or unavailable states today
