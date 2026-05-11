# Investor Demo Script

## Setup

1. Start the Qt/QML app from the release build.
2. Open Action Center.
3. Confirm the sidebar includes Action Center, Audit Log, Restore, AI, Backup, Optimizations, and Settings.
4. Keep the manual confirmation checkbox off for the first pass.

## Safety Proof

1. In Action Center, click `Dry Run` on Safe Junk Cleanup.
2. Show risk level, required confirmation, backup/restore availability, expected effect, dry-run result, actual result, and audit link.
3. Open Audit Log and show the dry-run entry.
4. Return to Action Center.

## Before / After Proof

1. Enable manual confirmation only for a low-risk or demo-safe action.
2. Execute Safe Junk Cleanup or Flush DNS.
3. Show the Before / After Proof panel:
   - CPU pressure
   - RAM pressure
   - disk usage
   - startup item count
   - heavy process list summary
   - recoverable storage estimate
   - boot/startup estimate
4. Explain that proof captures are local and used for trust, not exaggerated claims.

## Restore Story

1. Open Restore / Undo Center.
2. Show PulseBoost snapshots, quarantined files, rollback entries, and restore controls.
3. Create a snapshot.
4. Explain that advanced actions require backups where applicable.

## AI Advisory Mode

1. Open AI.
2. Ask: `What should I optimize first?`
3. Show that AI recommends, explains, and prepares action context.
4. Confirm that AI does not silently execute high-risk actions.

## Close

Emphasize that Phase 2 is reliability and proof-of-work focused: no new optimizer categories, no Tauri production path, strict CLI JSON, SQLite audit logging, and release validation.
