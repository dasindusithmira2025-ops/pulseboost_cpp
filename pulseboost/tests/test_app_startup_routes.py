import importlib
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tests.test_client_utils import managed_test_client

import config


class AppStartupRouteTests(unittest.TestCase):
    def test_main_app_binds_desktop_services_for_route_access(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            env = {
                "DB_PATH": str(temp_path / "startup.db"),
                "VECTOR_PATH": str(temp_path / "vectors"),
                "DESKTOP_RUNTIME": "electron",
            }
            with patch.dict(os.environ, env, clear=False):
                config.get_settings.cache_clear()
                import api.main as api_main

                api_main = importlib.reload(api_main)

                with managed_test_client(api_main.app) as client:
                    self.assertEqual(client.get("/healthz").status_code, 200)
                    self.assertEqual(client.get("/api/status").status_code, 200)
                    self.assertEqual(client.get("/api/auth/status").status_code, 200)
                    self.assertEqual(client.get("/api/network/diagnostics").status_code, 200)
                    self.assertEqual(client.get("/api/gpu/stats").status_code, 200)
                    self.assertEqual(client.get("/api/games").status_code, 200)
                    self.assertEqual(client.get("/api/trust-center/status").status_code, 200)
                    export_response = client.post("/api/settings/data-action", json={"action": "export_all_data", "payload": {}})
                    self.assertEqual(export_response.status_code, 200)
                    self.assertIn("empty_state", export_response.json())
                    clear_response = client.post("/api/settings/data-action", json={"action": "clear_benchmark_history", "payload": {}})
                    self.assertEqual(clear_response.status_code, 200)
                    reset_response = client.post("/api/settings/data-action", json={"action": "reset_all_settings", "payload": {}})
                    self.assertEqual(reset_response.status_code, 200)
                    import_response = client.post(
                        "/api/settings/data-action",
                        json={"action": "import_settings", "payload": {"preferences": {"checkUpdates": False}}},
                    )
                    self.assertEqual(import_response.status_code, 200)

                config.get_settings.cache_clear()


if __name__ == "__main__":
    unittest.main()


