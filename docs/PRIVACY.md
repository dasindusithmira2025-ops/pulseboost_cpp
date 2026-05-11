# Privacy

PulseBoost is designed as a local Windows optimizer. System telemetry, action history, audit logs, snapshots, and proof reports are stored locally by default.

## Local Data

PulseBoost may store:

- System snapshots for restore and proof reports.
- Optimization action history.
- SQLite audit log entries for system-changing actions.
- Quarantined cleanup files.
- Optional AI interaction metadata for local assistant quality.

## AI Mode

Local AI mode keeps recommendations on the device. If cloud AI is configured by the user, prompts may include system-health context needed to answer the request. Cloud mode is optional and can be disabled.

## Safety and Consent

High-risk and advanced actions require explicit confirmation. AI can recommend actions and explain risk, but it must not silently execute high-risk optimizations.

## Secrets

API keys and local secrets must not be committed to the repository or included in release packages. Release validation checks for common secret and generated-file patterns.
