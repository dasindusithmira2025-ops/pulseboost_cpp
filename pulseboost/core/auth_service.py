from __future__ import annotations

import hashlib
import platform
import secrets
import socket
import time
import uuid
from typing import Any

from config import Settings
from core.audit_log import AuditLog
from core.cognition.plans import normalize_plan
from core.database import DatabaseService
from core.entitlements import build_entitlement_snapshot, build_plan_info, entitlement_access_map, legacy_plan_features
from core.models import AccountIdentity, AuthSession, DeviceActivation, EntitlementSnapshot, FeatureEntitlement, PlanInfo
from core.secure_storage import SecretCipher


class AuthService:
    def __init__(
        self,
        *,
        database: DatabaseService,
        audit_log: AuditLog,
        secret_cipher: SecretCipher,
        settings: Settings,
    ) -> None:
        self.database = database
        self.audit_log = audit_log
        self.secret_cipher = secret_cipher
        self.settings = settings

    async def initialize(self) -> None:
        active_session = await self.database.get_active_auth_session()
        if (
            active_session
            and not self.settings.auth_dev_mode
            and active_session.get("source") == "local-dev-placeholder"
        ):
            await self.database.clear_auth_session(active_session["account_id"])
            await self.database.upsert_entitlement_snapshot(self._build_signed_out_snapshot().to_dict())
            await self.audit_log.record_event(
                module="auth",
                action="clear_dev_placeholder_session",
                target=active_session["account_id"],
                rationale="Removed a local dev-placeholder auth session because AUTH_DEV_MODE is disabled.",
                validity_tag="VALIDATED",
                triggered_by="startup",
                status="success",
            )

        cached_snapshot = await self.database.get_entitlement_snapshot(None)
        if cached_snapshot is None:
            await self.database.upsert_entitlement_snapshot(
                self._build_signed_out_snapshot().to_dict()
            )

    async def load_current_local_session(self) -> AuthSession | None:
        session_payload = await self.database.get_active_auth_session()
        if not session_payload:
            return None
        tokens = await self.database.get_auth_tokens(session_payload["account_id"])
        if not tokens:
            return None
        return AuthSession(
            account_id=session_payload["account_id"],
            access_token=self.secret_cipher.decrypt(tokens["access_token_blob"]),
            refresh_token=self.secret_cipher.decrypt(tokens["refresh_token_blob"]),
            expires_at=session_payload["expires_at"],
            last_verified_at=session_payload.get("last_verified_at"),
            signed_in=bool(session_payload.get("signed_in", False)),
        )

    async def save_session_securely(
        self,
        *,
        identity: AccountIdentity,
        session: AuthSession,
        plan: PlanInfo,
        entitlement_snapshot: EntitlementSnapshot,
        activation: DeviceActivation,
        source: str,
    ) -> None:
        await self.database.upsert_account_identity(identity.to_dict())
        await self.database.upsert_account_plan(
            {
                "account_id": identity.account_id,
                "plan_tier": plan.plan_tier,
                "status": plan.status,
                "renewal_at": plan.renewal_at,
                "trial_ends_at": plan.trial_ends_at,
            }
        )
        await self.database.upsert_device_activation({**activation.to_dict(), "source": source})
        await self.database.upsert_auth_session(
            {
                "account_id": session.account_id,
                "expires_at": session.expires_at,
                "last_verified_at": session.last_verified_at,
                "signed_in": session.signed_in,
                "source": source,
                "activation_id": activation.activation_id,
            }
        )
        await self.database.upsert_auth_tokens(
            session.account_id,
            self.secret_cipher.encrypt(session.access_token),
            self.secret_cipher.encrypt(session.refresh_token),
        )
        await self.database.upsert_entitlement_snapshot(entitlement_snapshot.to_dict())

    async def get_auth_status(self) -> dict[str, Any]:
        session_payload = await self.database.get_active_auth_session()
        if not session_payload:
            signed_out_snapshot = self._build_signed_out_snapshot()
            await self.database.upsert_entitlement_snapshot(signed_out_snapshot.to_dict())
            latest_activation = await self.database.latest_device_activation()
            signed_out_activation = None
            if latest_activation:
                signed_out_activation = {
                    "activation_id": latest_activation.get("activation_id"),
                    "account_id": None,
                    "machine_hash": None,
                    "device_name": latest_activation.get("device_name"),
                    "app_version": latest_activation.get("app_version"),
                    "activated_at": latest_activation.get("activated_at"),
                    "last_seen_at": latest_activation.get("last_seen_at"),
                    "revoked": bool(latest_activation.get("revoked", False)),
                    "source": latest_activation.get("source", "local-cache"),
                    "linked_account": False,
                }
            return {
                "signed_in": False,
                "identity": None,
                "session": {
                    "signed_in": False,
                    "account_id": None,
                    "expires_at": None,
                    "last_verified_at": None,
                    "source": signed_out_snapshot.source,
                    "has_secure_tokens": False,
                },
                "plan": build_plan_info(signed_out_snapshot.plan_tier, status="signed_out").to_dict(),
                "entitlement_snapshot": signed_out_snapshot.to_dict(),
                "feature_access": entitlement_access_map(signed_out_snapshot),
                "activation": signed_out_activation,
                "authority": self._authority_state(),
                "links": self._links_state(),
                "integration_note": "The desktop app is using local cached entitlement posture until website authority is connected.",
            }

        account_id = session_payload["account_id"]
        identity_payload = await self.database.get_account_identity(account_id)
        plan_payload = await self.database.get_account_plan(account_id)
        entitlement_payload = await self.database.get_entitlement_snapshot(account_id)
        activation_payload = await self.database.get_device_activation(account_id)
        tokens = await self.database.get_auth_tokens(account_id)

        plan = self._plan_from_payload(plan_payload, fallback_plan=entitlement_payload.get("plan_tier") if entitlement_payload else None)
        entitlement_snapshot = self._entitlement_from_payload(
            entitlement_payload,
            fallback_account_id=account_id,
            fallback_plan=plan.plan_tier,
            fallback_source=session_payload.get("source", "local-cache"),
        )
        return {
            "signed_in": True,
            "identity": identity_payload,
            "session": {
                "signed_in": True,
                "account_id": account_id,
                "expires_at": session_payload["expires_at"],
                "last_verified_at": session_payload.get("last_verified_at"),
                "source": session_payload.get("source", "local"),
                "has_secure_tokens": bool(tokens),
            },
            "plan": plan.to_dict(),
            "entitlement_snapshot": entitlement_snapshot.to_dict(),
            "feature_access": entitlement_access_map(entitlement_snapshot),
            "activation": activation_payload,
            "authority": self._authority_state(),
            "links": self._links_state(),
            "integration_note": (
                "Website authority integration is pending. Current account state is stored locally for desktop foundation work."
                if not self.settings.auth_enabled
                else "Website authority hooks are configured for future token exchange work."
            ),
        }

    async def current_access_context(self) -> dict[str, Any]:
        status = await self.get_auth_status()
        snapshot = self._entitlement_from_payload(
            status["entitlement_snapshot"],
            fallback_account_id=status["session"].get("account_id"),
            fallback_plan=status["plan"]["plan_tier"],
            fallback_source=status["session"].get("source") or status["entitlement_snapshot"].get("source", "local"),
        )
        return {
            "signed_in": status["signed_in"],
            "identity": status["identity"],
            "auth_status": status,
            "plan_info": status["plan"],
            "plan_tier": status["plan"]["plan_tier"],
            "entitlement_snapshot": snapshot.to_dict(),
            "feature_access": entitlement_access_map(snapshot),
            "legacy_features": legacy_plan_features(status["plan"]["plan_tier"]),
        }

    async def sign_in_local_placeholder(
        self,
        *,
        email: str,
        display_name: str,
        plan_tier: str,
    ) -> dict[str, Any]:
        normalized_email = email.strip().lower()
        if "@" not in normalized_email:
            raise ValueError("A valid email address is required for the local placeholder sign-in flow.")

        account_id = str(uuid.uuid5(uuid.NAMESPACE_URL, normalized_email))
        created_at = time.time()
        identity = AccountIdentity(
            account_id=account_id,
            email=normalized_email,
            display_name=display_name.strip() or normalized_email.split("@", 1)[0],
            created_at=created_at,
        )
        plan = build_plan_info(normalize_plan(plan_tier), status="active")
        entitlement_snapshot = build_entitlement_snapshot(
            account_id=account_id,
            plan_tier=plan.plan_tier,
            source="local-dev-placeholder",
        )
        activation = await self.create_or_update_local_activation(account_id=account_id, source="local-dev-placeholder")
        session = AuthSession(
            account_id=account_id,
            access_token=secrets.token_urlsafe(32),
            refresh_token=secrets.token_urlsafe(48),
            expires_at=time.time() + 30 * 24 * 3600,
            last_verified_at=time.time(),
            signed_in=True,
        )
        await self.save_session_securely(
            identity=identity,
            session=session,
            plan=plan,
            entitlement_snapshot=entitlement_snapshot,
            activation=activation,
            source="local-dev-placeholder",
        )
        await self.audit_log.record_event(
            module="auth",
            action="local_placeholder_sign_in",
            target=identity.email,
            after_value={
                "account_id": identity.account_id,
                "plan_tier": plan.plan_tier,
                "activation_id": activation.activation_id,
            },
            rationale="Created a local placeholder account session so future website-owned auth and entitlement flows can be integrated without changing desktop foundations.",
            validity_tag="VALIDATED",
            triggered_by="api",
            status="success",
        )
        return await self.get_auth_status()

    async def clear_session(self, *, triggered_by: str = "system") -> dict[str, Any]:
        session_payload = await self.database.get_active_auth_session()
        target = session_payload["account_id"] if session_payload else "signed_out"
        if session_payload:
            await self.database.clear_auth_session(session_payload["account_id"])
        await self.database.upsert_entitlement_snapshot(self._build_signed_out_snapshot().to_dict())
        await self.audit_log.record_event(
            module="auth",
            action="sign_out",
            target=target,
            rationale="Cleared the active local desktop session and removed stored secure tokens.",
            validity_tag="VALIDATED",
            triggered_by=triggered_by,
            status="success",
        )
        return await self.get_auth_status()

    async def refresh_entitlement_snapshot(self, *, triggered_by: str = "system") -> dict[str, Any]:
        status = await self.get_auth_status()
        if not status["signed_in"]:
            snapshot = self._build_signed_out_snapshot()
            await self.database.upsert_entitlement_snapshot(snapshot.to_dict())
            await self.audit_log.record_event(
                module="auth",
                action="refresh_entitlements",
                target="signed_out",
                after_value={"plan_tier": snapshot.plan_tier, "source": snapshot.source},
                rationale="Refreshed the signed-out entitlement snapshot from local fallback configuration.",
                validity_tag="VALIDATED",
                triggered_by=triggered_by,
                status="success",
            )
            return await self.get_auth_status()

        plan = self._plan_from_payload(status["plan"], fallback_plan=self.settings.default_plan)
        refreshed_snapshot = build_entitlement_snapshot(
            account_id=status["session"]["account_id"],
            plan_tier=plan.plan_tier,
            source=status["session"]["source"],
        )
        await self.database.upsert_entitlement_snapshot(refreshed_snapshot.to_dict())
        await self.database.upsert_auth_session(
            {
                "account_id": status["session"]["account_id"],
                "expires_at": status["session"]["expires_at"],
                "last_verified_at": time.time(),
                "signed_in": True,
                "source": status["session"]["source"],
                "activation_id": status["activation"]["activation_id"] if status.get("activation") else None,
            }
        )
        await self.create_or_update_local_activation(
            account_id=status["session"]["account_id"],
            source=status["session"]["source"],
        )
        await self.audit_log.record_event(
            module="auth",
            action="refresh_entitlements",
            target=status["session"]["account_id"],
            after_value={"plan_tier": refreshed_snapshot.plan_tier, "source": refreshed_snapshot.source},
            rationale="Refreshed local entitlement state and activation heartbeat for the current desktop session.",
            validity_tag="VALIDATED",
            triggered_by=triggered_by,
            status="success",
        )
        return await self.get_auth_status()

    async def create_or_update_local_activation(self, *, account_id: str, source: str) -> DeviceActivation:
        existing = await self.database.get_device_activation(account_id)
        now = time.time()
        activation = DeviceActivation(
            activation_id=existing["activation_id"] if existing else self._activation_id(account_id),
            account_id=account_id,
            machine_hash=self._machine_hash(),
            device_name=self.settings.machine_name or socket.gethostname(),
            app_version=self.settings.app_version,
            activated_at=existing["activated_at"] if existing else now,
            last_seen_at=now,
            revoked=bool(existing["revoked"]) if existing else False,
        )
        await self.database.upsert_device_activation({**activation.to_dict(), "source": source})
        return activation

    async def token_exchange_placeholder(
        self,
        *,
        provider: str,
        authorization_code: str,
    ) -> dict[str, Any]:
        _ = authorization_code
        return {
            "ok": False,
            "implemented": False,
            "provider": provider,
            "authority": self._authority_state(),
            "reason": "Website-owned token exchange is not connected yet. The desktop app remains a client, not the licensing authority.",
        }

    def _authority_state(self) -> dict[str, Any]:
        return {
            "source_of_truth": "website_backend",
            "configured": bool(self.settings.website_authority_url),
            "auth_enabled": self.settings.auth_enabled,
            "dev_mode_available": self.settings.auth_dev_mode,
            "base_url": self.settings.website_authority_url or None,
        }

    def _links_state(self) -> dict[str, Any]:
        return {
            "sign_in_url": self.settings.website_signin_url or None,
            "create_account_url": self.settings.website_create_account_url or None,
            "manage_subscription_url": self.settings.website_manage_subscription_url or None,
        }

    def _machine_hash(self) -> str:
        raw = "|".join(
            [
                self.settings.machine_id,
                self.settings.machine_name,
                platform.platform(),
                platform.node(),
                str(uuid.getnode()),
            ]
        )
        return hashlib.sha256(raw.encode("utf-8")).hexdigest()

    def _activation_id(self, account_id: str) -> str:
        return str(uuid.uuid5(uuid.NAMESPACE_DNS, f"{account_id}:{self._machine_hash()}"))

    def _build_signed_out_snapshot(self) -> EntitlementSnapshot:
        return build_entitlement_snapshot(
            account_id=None,
            plan_tier=self.settings.default_plan,
            source="local-default-config",
        )

    async def _load_or_build_signed_out_snapshot(self) -> EntitlementSnapshot:
        payload = await self.database.get_entitlement_snapshot(None)
        return self._entitlement_from_payload(
            payload,
            fallback_account_id=None,
            fallback_plan=self.settings.default_plan,
            fallback_source="local-default-config",
        )

    def _plan_from_payload(self, payload: dict[str, Any] | None, *, fallback_plan: str | None) -> PlanInfo:
        if payload:
            return PlanInfo(
                plan_tier=normalize_plan(payload.get("plan_tier")),
                status=payload.get("status", "active"),
                renewal_at=payload.get("renewal_at"),
                trial_ends_at=payload.get("trial_ends_at"),
            )
        return build_plan_info(fallback_plan, status="active")

    def _entitlement_from_payload(
        self,
        payload: dict[str, Any] | None,
        *,
        fallback_account_id: str | None,
        fallback_plan: str | None,
        fallback_source: str,
    ) -> EntitlementSnapshot:
        if not payload:
            return build_entitlement_snapshot(
                account_id=fallback_account_id,
                plan_tier=fallback_plan,
                source=fallback_source,
            )

        features = {}
        for feature_key, feature_payload in (payload.get("features") or {}).items():
            features[feature_key] = FeatureEntitlement(
                feature_key=feature_payload.get("feature_key", feature_key),
                enabled=bool(feature_payload.get("enabled", False)),
                reason=feature_payload.get("reason", ""),
                source=feature_payload.get("source", payload.get("source", fallback_source)),
            )
        return EntitlementSnapshot(
            account_id=payload.get("account_id"),
            plan_tier=normalize_plan(payload.get("plan_tier", fallback_plan)),
            features=features,
            generated_at=payload.get("generated_at", time.time()),
            source=payload.get("source", fallback_source),
        )
