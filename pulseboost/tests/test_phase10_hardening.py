import asyncio
import tempfile
import unittest
from pathlib import Path

from fastapi import FastAPI
from tests.test_client_utils import managed_test_client

from api.routes import router
from core.audit_log import AuditLog
from core.capabilities import CapabilitySnapshot
from core.database import DatabaseService
from core.optimizer import SystemOptimizer
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard
from core.trust_center import TrustCenterService


def build_capabilities():
    return CapabilitySnapshot(
        machine_id="machine-1",
        captured_at=1.0,
        os_name="Windows",
        os_version="test",
        is_windows=True,
        is_admin=True,
        desktop_runtime="webview",
        python_version="3.11",
        has_powershell=True,
        has_wmi=False,
        has_temperature_sensors=False,
        has_gpu_runtime=False,
        supports_electron_runtime=False,
        supports_webview_runtime=True,
        can_manage_services=True,
        can_edit_registry=True,
        can_control_process_priority=True,
        can_control_affinity=True,
        notes=[],
    )


class Phase10HardeningTests(unittest.TestCase):
    def test_expert_mode_route_persists_and_trust_center_reflects_it(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "phase10.db")
            asyncio.run(database.initialize())
            audit_log = AuditLog(database)
            optimizer = SystemOptimizer(
                database=database,
                audit_log=audit_log,
                revert_manager=RevertManager(database, audit_log),
                safety_guard=SafetyGuard(),
                capabilities=build_capabilities(),
            )
            trust = TrustCenterService(
                database=database,
                capabilities=build_capabilities(),
                safety_guard=SafetyGuard(),
                optimizer=optimizer,
            )

            app = FastAPI()
            app.include_router(router)
            app.state.foundation = {"database": database, "audit_log": audit_log, "recovery": {"rationale": "clear"}}
            app.state.trust_center_service = trust

            with managed_test_client(app) as client:
                response = client.post("/api/settings/expert-mode", json={"enabled": True})
                self.assertEqual(response.status_code, 200)
                self.assertTrue(response.json()["enabled"])
                trust_response = client.get("/api/trust-center/status")
                self.assertEqual(trust_response.status_code, 200)
                self.assertTrue(trust_response.json()["expert_mode_state"])
                self.assertFalse(trust_response.json()["dangerous_tweaks_disabled"])


if __name__ == "__main__":
    unittest.main()


