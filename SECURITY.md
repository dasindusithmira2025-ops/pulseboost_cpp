# Security Policy

PulseBoost is a Windows system utility. Security and rollback behavior are part of the product contract.

## Reporting

Do not disclose exploitable vulnerabilities publicly before maintainers have had time to investigate.

Report security issues privately to the repository owner through GitHub security advisories or a private maintainer channel.

## Safety Rules

- Critical Windows processes must never be suspended, deprioritized, or terminated.
- Registry, service, power, GPU, and network changes must save previous state first.
- Risky actions must require explicit confirmation or expert mode.
- Unsupported capability states must be reported as unavailable, not faked as successful.
- Secrets and local runtime databases must not be committed.

## Supported Scope

Security review currently covers the source tree, FastAPI backend, Electron shell, React frontend, SQLite persistence, and Windows integration adapters.
