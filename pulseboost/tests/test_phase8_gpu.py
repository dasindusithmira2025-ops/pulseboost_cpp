import asyncio
import tempfile
import unittest
from pathlib import Path
from unittest.mock import AsyncMock, Mock, patch

from fastapi import FastAPI
from tests.test_client_utils import managed_test_client

from api.routes import router
from core.audit_log import AuditLog
from core.capabilities import CapabilitySnapshot
from core.database import DatabaseService
from core.gpu_controller import GPUController


def build_capabilities(*, has_gpu_runtime=False, can_edit_registry=True):
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
        has_gpu_runtime=has_gpu_runtime,
        supports_electron_runtime=False,
        supports_webview_runtime=True,
        can_manage_services=True,
        can_edit_registry=can_edit_registry,
        can_control_process_priority=True,
        can_control_affinity=True,
        notes=[],
    )


class GPUPhase8Tests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.database = DatabaseService(Path(self.temp_dir.name) / "phase8.db")
        asyncio.run(self.database.initialize())
        self.audit_log = AuditLog(self.database)

    def tearDown(self):
        self.temp_dir.cleanup()

    def build_controller(self, *, vendor="NVIDIA", has_gpu_runtime=False, can_edit_registry=True):
        return GPUController(
            database=self.database,
            audit_log=self.audit_log,
            capabilities=build_capabilities(has_gpu_runtime=has_gpu_runtime, can_edit_registry=can_edit_registry),
            hardware_profile={"gpu_vendor": vendor, "gpu_model": f"{vendor} Test GPU"},
        )

    def test_stats_surface_vendor_and_unsupported_telemetry(self):
        controller = self.build_controller(vendor="NVIDIA", has_gpu_runtime=False)
        stats = asyncio.run(controller.stats())
        self.assertEqual(stats["vendor"], "nvidia")
        self.assertFalse(stats["telemetry_supported"])
        self.assertIn("nvidia-smi", stats["reason"])

    def test_stats_surface_live_nvidia_telemetry_when_runtime_available(self):
        controller = self.build_controller(vendor="NVIDIA", has_gpu_runtime=True)
        completed = Mock()
        completed.stdout = "NVIDIA Test GPU, 591.86, 11, 52, 4096, 40.25, 2400\n"
        with patch("core.gpu_controller.subprocess.run", return_value=completed):
            stats = asyncio.run(controller.stats())
        self.assertTrue(stats["telemetry_supported"])
        self.assertEqual(stats["model"], "NVIDIA Test GPU")
        self.assertEqual(stats["driver_version"], "591.86")
        self.assertEqual(stats["utilization_percent"], 11)
        self.assertEqual(stats["temperature_c"], 52)
        self.assertEqual(stats["memory_used_mb"], 4096)
        self.assertEqual(stats["power_watts"], 40.25)
        self.assertEqual(stats["clock_mhz"], 2400)

    def test_generic_setting_uses_honest_dry_run(self):
        controller = self.build_controller(vendor="NVIDIA", can_edit_registry=True)
        result = asyncio.run(controller.apply_setting("hardware_accelerated_gpu_scheduling", True))
        self.assertTrue(result["success"])
        self.assertTrue(result["dry_run"])
        self.assertFalse(result["applied"])

    def test_vendor_specific_setting_requires_runtime(self):
        controller = self.build_controller(vendor="AMD", has_gpu_runtime=False)
        result = asyncio.run(controller.apply_setting("amd_anti_lag", True, confirm_risky=True))
        self.assertFalse(result["success"])
        self.assertFalse(result["supported"])

    def test_supported_nvidia_setting_reports_dry_run_reason(self):
        controller = self.build_controller(vendor="NVIDIA", has_gpu_runtime=True)
        stats = asyncio.run(controller.stats())
        nvidia_setting = next(item for item in stats["settings"] if item["id"] == "nvidia_low_latency_mode")
        self.assertTrue(nvidia_setting["supported"])
        self.assertIn("dry-run", nvidia_setting["reason"])

    def test_gpu_api_endpoints(self):
        controller = self.build_controller(vendor="NVIDIA", has_gpu_runtime=False)
        app = FastAPI()
        app.include_router(router)
        app.state.gpu_controller = controller
        app.state.auth_service = type(
            "AuthStub",
            (),
            {"current_access_context": staticmethod(AsyncMock(return_value={"feature_access": {"advanced_gpu_controls": True}}))},
        )()
        with managed_test_client(app) as client:
            stats = client.get("/api/gpu/stats")
            self.assertEqual(stats.status_code, 200)
            checklist = client.get("/api/bios/checklist")
            self.assertEqual(checklist.status_code, 200)
            setting = client.post(
                "/api/gpu/settings",
                json={"setting_id": "hardware_accelerated_gpu_scheduling", "value": True, "confirm_risky": False},
            )
            self.assertEqual(setting.status_code, 200)
            self.assertIn("dry_run", setting.json())


if __name__ == "__main__":
    unittest.main()


