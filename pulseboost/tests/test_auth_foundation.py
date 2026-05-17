import asyncio
import tempfile
import unittest
from pathlib import Path

import api.routes as routes_module
from fastapi import FastAPI
from tests.test_client_utils import managed_test_client

from api.routes import router
from config import Settings
from core.audit_log import AuditLog
from core.auth_service import AuthService
from core.database import DatabaseService


class FakeCipher:
    def encrypt(self, plaintext: str) -> bytes:
        return f"enc::{plaintext}".encode("utf-8")

    def decrypt(self, ciphertext: bytes) -> str:
        return ciphertext.decode("utf-8").replace("enc::", "", 1)


class MemoryStub:
    async def export_actions_csv(self, limit: int = 200) -> str:
        return f"timestamp,action\n1,export-{limit}\n"


class OrchestratorStub:
    def __init__(self) -> None:
        self.memory = MemoryStub()


class AuthFoundationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.database = DatabaseService(Path(self.temp_dir.name) / "auth.db")
        asyncio.run(self.database.initialize())
        self.audit_log = AuditLog(self.database)
        self.settings = Settings()
        self.settings.db_path = Path(self.temp_dir.name) / "auth.db"
        self.settings.vector_path = Path(self.temp_dir.name) / "vectors"
        self.settings.machine_id = "machine-alpha"
        self.settings.machine_name = "PulseBoost Test Rig"
        self.settings.default_plan = "free"
        self.settings.auth_dev_mode = True
        self.settings.app_version = "2.4.0-test"
        self.service = AuthService(
            database=self.database,
            audit_log=self.audit_log,
            secret_cipher=FakeCipher(),
            settings=self.settings,
        )
        self._original_auth_dev_mode = routes_module.settings.auth_dev_mode
        routes_module.settings.auth_dev_mode = True
        asyncio.run(self.service.initialize())

    def tearDown(self) -> None:
        routes_module.settings.auth_dev_mode = self._original_auth_dev_mode
        self.temp_dir.cleanup()

    def test_local_session_persists_secure_tokens_and_activation(self) -> None:
        status = asyncio.run(
            self.service.sign_in_local_placeholder(
                email="operator@example.com",
                display_name="Operator",
                plan_tier="pro",
            )
        )

        self.assertTrue(status["signed_in"])
        self.assertEqual(status["plan"]["plan_tier"], "pro")
        self.assertTrue(status["feature_access"]["audit_export"])
        self.assertIsNotNone(status["activation"])
        self.assertEqual(len(status["activation"]["machine_hash"]), 64)

        session = asyncio.run(self.service.load_current_local_session())
        tokens = asyncio.run(self.database.get_auth_tokens(status["identity"]["account_id"]))
        self.assertIsNotNone(session)
        self.assertIsNotNone(tokens)
        self.assertTrue(tokens["access_token_blob"].startswith(b"enc::"))
        self.assertNotEqual(tokens["access_token_blob"], session.access_token.encode("utf-8"))

    def test_sign_out_clears_session_and_secure_tokens(self) -> None:
        signed_in = asyncio.run(
            self.service.sign_in_local_placeholder(
                email="signout@example.com",
                display_name="Sign Out",
                plan_tier="team",
            )
        )
        account_id = signed_in["identity"]["account_id"]

        status = asyncio.run(self.service.clear_session(triggered_by="test"))

        self.assertFalse(status["signed_in"])
        self.assertFalse(status["session"]["has_secure_tokens"])
        self.assertIsNone(asyncio.run(self.database.get_active_auth_session()))
        self.assertIsNone(asyncio.run(self.database.get_auth_tokens(account_id)))
        self.assertIsNotNone(status["activation"])
        self.assertIsNone(status["activation"]["account_id"])
        self.assertIsNone(status["activation"]["machine_hash"])
        self.assertFalse(status["activation"]["linked_account"])

    def test_refresh_entitlements_updates_last_verified_and_activation(self) -> None:
        signed_in = asyncio.run(
            self.service.sign_in_local_placeholder(
                email="refresh@example.com",
                display_name="Refresh",
                plan_tier="enterprise",
            )
        )
        before_verified = signed_in["session"]["last_verified_at"]
        before_seen = signed_in["activation"]["last_seen_at"]

        refreshed = asyncio.run(self.service.refresh_entitlement_snapshot(triggered_by="test"))

        self.assertGreaterEqual(refreshed["session"]["last_verified_at"], before_verified)
        self.assertGreaterEqual(refreshed["activation"]["last_seen_at"], before_seen)
        self.assertTrue(refreshed["feature_access"]["enterprise_policy_templates"])

    def test_token_exchange_placeholder_is_honest(self) -> None:
        result = asyncio.run(
            self.service.token_exchange_placeholder(
                provider="website",
                authorization_code="code-123",
            )
        )

        self.assertFalse(result["ok"])
        self.assertFalse(result["implemented"])
        self.assertEqual(result["authority"]["source_of_truth"], "website_backend")

    def test_auth_routes_and_feature_gating(self) -> None:
        app = FastAPI()
        app.include_router(router)
        app.state.auth_service = self.service
        app.state.orchestrator = OrchestratorStub()

        with managed_test_client(app) as client:
            signed_out = client.get("/api/auth/status")
            self.assertEqual(signed_out.status_code, 200)
            self.assertFalse(signed_out.json()["signed_in"])

            export_locked = client.get("/api/actions/export")
            self.assertEqual(export_locked.status_code, 403)

            local_sign_in = client.post(
                "/api/auth/local-session",
                json={
                    "email": "route@example.com",
                    "display_name": "Route User",
                    "plan_tier": "pro",
                },
            )
            self.assertEqual(local_sign_in.status_code, 200)
            self.assertTrue(local_sign_in.json()["signed_in"])

            identity = client.get("/api/account/identity")
            self.assertEqual(identity.status_code, 200)
            self.assertEqual(identity.json()["identity"]["email"], "route@example.com")

            entitlements = client.get("/api/account/entitlements")
            self.assertEqual(entitlements.status_code, 200)
            self.assertTrue(entitlements.json()["feature_access"]["audit_export"])

            activation = client.get("/api/account/activation")
            self.assertEqual(activation.status_code, 200)
            self.assertEqual(activation.json()["activation"]["device_name"], "PulseBoost Test Rig")

            export_ready = client.get("/api/actions/export")
            self.assertEqual(export_ready.status_code, 200)
            self.assertIn("export-200", export_ready.text)

            sign_out = client.post("/api/auth/sign-out")
            self.assertEqual(sign_out.status_code, 200)
            self.assertFalse(sign_out.json()["signed_in"])

    def test_local_session_invalid_email_returns_400(self) -> None:
        app = FastAPI()
        app.include_router(router)
        app.state.auth_service = self.service
        app.state.orchestrator = OrchestratorStub()

        with managed_test_client(app) as client:
            invalid = client.post(
                "/api/auth/local-session",
                json={
                    "email": "not-an-email",
                    "display_name": "Invalid",
                    "plan_tier": "free",
                },
            )
            self.assertEqual(invalid.status_code, 400)
            self.assertIn("valid email address", invalid.json().get("detail", ""))


if __name__ == "__main__":
    unittest.main()


