# PulseBoost Safety Model

PulseBoost treats Qt/QML as the production UI and routes system-changing work through an explicit safety model.

## Action Policy

Every optimization action has a descriptor:

- `actionId`: stable identifier used by UI, CLI, audit log, and tests.
- `riskLevel`: `read_only`, `low`, `moderate`, `high`, or `critical`.
- `dryRunSupported`: whether the action can preview impact without changing state.
- `confirmationRequired`: whether live execution requires explicit user confirmation.
- `backupRequired`: whether a restore point, registry backup, network backup, or equivalent guard is required.
- `rollbackAvailable`: whether PulseBoost can expose a revert path.
- `auditRequired`: whether the action must be written to SQLite.
- `advancedOnly`: whether the action is hidden behind advanced/manual execution.

The implementation lives in `SafetyPolicy` and is shared by CLI and UI workflows.

## Execution Rules

Read-only actions can run without confirmation. Low-risk actions should still surface audit metadata. Moderate and higher actions require clear UI language and are logged. High and critical actions require explicit confirmation, and advanced-only actions cannot run through advisory AI prompts.

## Storage Cleanup

Junk cleanup is non-destructive by default. PulseBoost quarantines or recycles files instead of permanently deleting them. Permanent deletion is high risk and requires advanced confirmation.

## Network and RAM

Global `netsh` and Winsock changes are advanced-only. PulseBoost creates a network backup before applying supported network tuning and exposes a revert action. RAM working-set trimming is manual/advanced and is not marketed as a guaranteed performance boost.

## Audit Log

System-changing actions are written to SQLite table `action_audit_log`:

- `created_at`
- `action_type`
- `status`
- `summary`
- `risk_level`
- `dry_run`
- `request_json`
- `result_json`

The Audit Log screen reads this table directly so investor demos can prove what happened.
