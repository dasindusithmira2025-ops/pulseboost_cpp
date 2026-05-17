import asyncio
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import AsyncMock, patch

from fastapi import FastAPI
from tests.test_client_utils import managed_test_client

from api.routes import router
from core.audit_log import AuditLog
from core.capabilities import CapabilitySnapshot
from core.database import DatabaseService
from core.network_optimizer import NetworkOptimizer


def build_capabilities(*, is_admin=True):
    return CapabilitySnapshot(
        machine_id="machine-1",
        captured_at=1.0,
        os_name="Windows",
        os_version="test",
        is_windows=True,
        is_admin=is_admin,
        desktop_runtime="webview",
        python_version="3.11",
        has_powershell=True,
        has_wmi=False,
        has_temperature_sensors=False,
        has_gpu_runtime=False,
        supports_electron_runtime=False,
        supports_webview_runtime=True,
        can_manage_services=is_admin,
        can_edit_registry=is_admin,
        can_control_process_priority=True,
        can_control_affinity=True,
        notes=[],
    )


class NetworkPhase7Tests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.database = DatabaseService(Path(self.temp_dir.name) / "phase7.db")
        asyncio.run(self.database.initialize())
        self.audit_log = AuditLog(self.database)

    def tearDown(self):
        self.temp_dir.cleanup()

    def build_optimizer(self, *, is_admin=True):
        return NetworkOptimizer(
            database=self.database,
            audit_log=self.audit_log,
            capabilities=build_capabilities(is_admin=is_admin),
        )

    def test_protocol_profile_prefers_udp_for_active_game(self):
        optimizer = self.build_optimizer()
        profile = optimizer._protocol_profile("gaming", {"game_name": "Valorant"})
        self.assertEqual(profile["detected_transport"], "udp")

    def test_diagnostics_persist(self):
        optimizer = self.build_optimizer()
        fake_public = {
            "label": "Public target",
            "supported": True,
            "latency_ms": 18.5,
            "jitter_ms": 1.4,
            "packet_loss_percent": 0.0,
            "attempts": 3,
            "transport": "tcp_connect",
        }
        with patch.object(optimizer, "_probe_target", new=AsyncMock(return_value=fake_public)):
            payload = asyncio.run(optimizer.diagnostics(session_mode="normal", active_session=None))
        self.assertEqual(payload["summary"]["bufferbloat_grade"], "A")
        latest = asyncio.run(self.database.latest_network_diagnostics())
        self.assertIsNotNone(latest)
        self.assertEqual(latest["summary"]["latency_ms"], 18.5)

    def test_qos_apply_is_honest_dry_run(self):
        optimizer = self.build_optimizer(is_admin=True)
        result = asyncio.run(optimizer.apply_qos_profile("Low Latency", protocol="udp"))
        self.assertTrue(result["success"])
        self.assertFalse(result["applied"])
        self.assertTrue(result["dry_run"])

    def test_probe_target_timeout_is_bounded(self):
        optimizer = self.build_optimizer()

        def slow_probe(*_args, **_kwargs):
            time.sleep(0.2)
            return None

        with patch.object(optimizer, "_probe_once", side_effect=slow_probe):
            result = asyncio.run(
                asyncio.wait_for(
                    optimizer._probe_target("127.0.0.1", 65535, "Slow probe", attempts=1, timeout_seconds=0.05),
                    timeout=1.0,
                )
            )
        self.assertFalse(result["supported"])

    def test_network_api_behavior(self):
        optimizer = self.build_optimizer()
        fake_public = {
            "label": "Public target",
            "supported": True,
            "latency_ms": 22.0,
            "jitter_ms": 2.5,
            "packet_loss_percent": 0.0,
            "attempts": 3,
            "transport": "tcp_connect",
        }
        app = FastAPI()
        app.include_router(router)
        app.state.network_optimizer = optimizer
        app.state.orchestrator = type("OrchestratorStub", (), {"current_state": {"session_mode": "normal", "active_session": None}})()
        app.state.auth_service = type(
            "AuthStub",
            (),
            {"current_access_context": staticmethod(AsyncMock(return_value={"feature_access": {"advanced_network_controls": True}}))},
        )()
        with patch.object(optimizer, "_probe_target", new=AsyncMock(return_value=fake_public)):
            with managed_test_client(app) as client:
                diagnostics = client.get("/api/network/diagnostics")
                self.assertEqual(diagnostics.status_code, 200)
                qos = client.post("/api/network/qos", json={"profile_name": "Low Latency", "protocol": "udp"})
                self.assertEqual(qos.status_code, 200)
                self.assertIn("dry_run", qos.json())


if __name__ == "__main__":
    unittest.main()


