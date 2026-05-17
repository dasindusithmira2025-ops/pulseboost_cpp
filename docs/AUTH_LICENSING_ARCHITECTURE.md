# PulseBoost Account, Auth, and Licensing Architecture

## Authority Model

PulseBoost is being prepared for a website-owned licensing model.

- Website/backend: future source of truth for users, subscriptions, plans, billing, licenses, device limits, and entitlement flags
- Desktop app: authenticated client that securely stores session material locally, caches the last known entitlement snapshot, and enforces client-side feature gating honestly

The desktop app is not the licensing authority.

## What Is Implemented Now

### Local account foundation

- Signed-out and signed-in desktop states
- Account identity cache
- Plan cache
- Entitlement snapshot cache
- Device activation cache
- Auth/session status endpoints
- Local placeholder sign-in flow for integration and UI validation

### Secure local storage

- Sensitive access and refresh tokens are encrypted through Windows DPAPI before being persisted
- Non-sensitive account, plan, activation, and entitlement metadata is stored in SQLite
- No raw passwords are collected or stored
- No raw secrets are written to plaintext config files or plaintext JSON

### Device activation foundation

- Stable machine hash generation from machine identity and platform characteristics
- Local activation record linked to the current account session
- Activation heartbeat updates through entitlement refresh
- Activation state exposed through API and Settings UI

### Entitlement-based gating

- The desktop shell consumes an entitlement snapshot instead of relying on a single license-key check
- Feature access is exposed as a keyed entitlement map
- UI premium locks and backend route gating now use feature entitlement state
- Existing product flows were preserved where current local functionality already ships

## Secure Storage Strategy

### Sensitive data

- `access_token`
- `refresh_token`

These values are encrypted via Windows DPAPI in `core/secure_storage.py` before being written to the `auth_session_tokens` table.

### Non-sensitive cached metadata

- account identity
- auth session timestamps and source metadata
- plan status
- entitlement snapshot
- device activation metadata

These values are stored in SQLite so the app can recover account posture and last-known licensing state offline.

## Data Model Summary

PulseBoost now persists and exposes these core models:

- `AccountIdentity`
- `AuthSession`
- `DeviceActivation`
- `PlanInfo`
- `FeatureEntitlement`
- `EntitlementSnapshot`

## Entitlement Model

Current feature keys:

- `core_monitoring`
- `safe_optimizations`
- `benchmark_history`
- `premium_benchmark_packs`
- `advanced_gpu_controls`
- `cloud_profile_sync`
- `multi_device_license`
- `enterprise_policy_templates`
- `audit_export`
- `advanced_network_controls`

The backend produces:

- a detailed entitlement snapshot with per-feature reason and source
- a simplified `feature_access` map for direct API/UI gating
- legacy plan-derived feature flags for older orchestrator surfaces that still depend on plan semantics

## Device Activation Model

Device activation is currently local and explicitly marked as pending website authority integration.

Current behavior:

- machine hash is derived locally
- activation record is created or refreshed on sign-in and entitlement refresh
- activation is associated with the current account session
- activation status is surfaced in Settings and API responses

Future website-owned behavior:

- activation issuance and revocation
- per-plan device count limits
- remote revocation state
- server-verified activation heartbeats

## API Foundation Added

PulseBoost now exposes:

- `GET /api/auth/status`
- `POST /api/auth/local-session`
- `POST /api/auth/token-exchange`
- `POST /api/auth/refresh-entitlements`
- `POST /api/auth/sign-out`
- `GET /api/account/identity`
- `GET /api/account/plan`
- `GET /api/account/entitlements`
- `GET /api/account/activation`

The token exchange endpoint is intentionally an honest placeholder until website integration exists.

## What Remains Placeholder

- Real website sign-in and token exchange
- Remote token refresh
- Remote entitlement verification
- Subscription and billing authority
- Device revocation from a website backend
- Cloud profile sync transport
- Premium content delivery from the website/backend

These surfaces are present as hooks and UI placeholders, not fake cloud implementations.
