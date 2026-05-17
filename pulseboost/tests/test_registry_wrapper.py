import asyncio
import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch

from core.models import CapabilitySnapshot
from core.registry_wrapper import SafeRegistry


def build_capabilities() -> CapabilitySnapshot:
    return CapabilitySnapshot(
        machine_id="machine-1",
        captured_at=1.0,
        os_name="Windows",
        os_version="test",
        is_windows=True,
        is_admin=True,
        desktop_runtime="electron",
        python_version="3.11",
        has_powershell=True,
        has_wmi=False,
        has_temperature_sensors=False,
        has_gpu_runtime=False,
        supports_electron_runtime=True,
        supports_webview_runtime=True,
        can_manage_services=True,
        can_edit_registry=True,
        can_control_process_priority=True,
        can_control_affinity=True,
        notes=[],
    )


class _DummyKey:
    def __enter__(self):
        return object()

    def __exit__(self, exc_type, exc, tb):
        return False


class RegistryWrapperTests(unittest.TestCase):
    def setUp(self) -> None:
        self.registry = SafeRegistry(
            audit_log=None,
            revert_manager=None,
            capabilities=build_capabilities(),
        )

    def test_read_value_missing_key_returns_supported_none_payload(self) -> None:
        fake_winreg = SimpleNamespace(
            KEY_READ=0,
            OpenKey=lambda *args, **kwargs: _DummyKey(),
            QueryValueEx=lambda *args, **kwargs: (_ for _ in ()).throw(FileNotFoundError()),
        )
        with patch("core.registry_wrapper.winreg", fake_winreg), patch.object(
            SafeRegistry,
            "_supported",
            return_value=True,
        ), patch.object(
            SafeRegistry,
            "_parse_path",
            return_value=("HKCU", "Software\\Test"),
        ):
            result = self.registry.read_value("HKCU\\Software\\Test", "MissingValue")
        self.assertTrue(result["supported"])
        self.assertIsNone(result["value"])
        self.assertIsNone(result["type"])

    def test_restore_missing_value_tolerates_missing_delete(self) -> None:
        fake_database = SimpleNamespace(
            get_revert_snapshot=AsyncMock(
                return_value={
                    "before_value": {
                        "path": "HKCU\\Software\\Test",
                        "key": "MissingValue",
                        "value": None,
                        "type": None,
                    }
                }
            )
        )
        fake_revert_manager = SimpleNamespace(
            database=fake_database,
            mark_restored=AsyncMock(return_value=None),
        )
        registry = SafeRegistry(
            audit_log=None,
            revert_manager=fake_revert_manager,
            capabilities=build_capabilities(),
        )
        registry.settings.executor_dry_run = False

        fake_winreg = SimpleNamespace(
            KEY_SET_VALUE=0,
            CreateKeyEx=lambda *args, **kwargs: _DummyKey(),
            DeleteValue=lambda *args, **kwargs: (_ for _ in ()).throw(FileNotFoundError()),
            SetValueEx=lambda *args, **kwargs: None,
        )
        with patch("core.registry_wrapper.winreg", fake_winreg), patch.object(
            SafeRegistry,
            "_parse_path",
            return_value=("HKCU", "Software\\Test"),
        ):
            result = asyncio.run(registry.restore("snapshot-1", triggered_by="test"))

        self.assertTrue(result["success"])
        fake_revert_manager.mark_restored.assert_awaited_once_with("snapshot-1", status="restored")


if __name__ == "__main__":
    unittest.main()
