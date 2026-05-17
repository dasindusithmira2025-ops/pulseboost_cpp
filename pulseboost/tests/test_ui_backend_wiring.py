import importlib
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import psutil
from tests.test_client_utils import managed_test_client

import config


class UiBackendWiringTests(unittest.TestCase):
    def test_settings_preferences_and_tweak_lifecycle_routes(self):
        with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as temp_dir:
            temp_path = Path(temp_dir)
            env = {
                "DB_PATH": str(temp_path / "ui-wiring.db"),
                "VECTOR_PATH": str(temp_path / "vectors"),
                "DESKTOP_RUNTIME": "electron",
            }
            with patch.dict(os.environ, env, clear=False):
                config.get_settings.cache_clear()
                import api.main as api_main

                api_main = importlib.reload(api_main)
                current_process = psutil.Process(os.getpid())

                with managed_test_client(api_main.app) as client:
                    settings_response = client.post(
                        "/api/settings/preferences",
                        json={
                            "preferences": {
                                "startWithWindows": False,
                                "updateChannel": "beta",
                                "backgroundMonitoring": False,
                            }
                        },
                    )
                    self.assertEqual(settings_response.status_code, 200)
                    settings_payload = client.get("/api/settings").json()
                    self.assertFalse(settings_payload["preferences"]["startWithWindows"])
                    self.assertEqual(settings_payload["preferences"]["updateChannel"], "beta")
                    self.assertFalse(settings_payload["preferences"]["backgroundMonitoring"])

                    apply_response = client.post(
                        "/api/tweaks/background_affinity_limit/apply",
                        json={
                            "params": {
                                "pid": current_process.pid,
                                "name": current_process.name(),
                                "cores": [0],
                            }
                        },
                    )
                    self.assertEqual(apply_response.status_code, 200)
                    applied_catalog = client.get("/api/tweaks").json()
                    applied_item = next(item for item in applied_catalog if item["id"] == "background_affinity_limit")
                    self.assertTrue(applied_item["applied"])
                    self.assertIsNotNone(applied_item["active_snapshot_id"])

                    audit_entries = client.get("/api/audit?limit=20").json()
                    revertible_entry = next(
                        entry for entry in audit_entries if (entry.get("after_value") or {}).get("revert_snapshot_id")
                    )
                    revert_response = client.post(f"/api/audit/{revertible_entry['id']}/revert")
                    self.assertEqual(revert_response.status_code, 200)

                    reverted_catalog = client.get("/api/tweaks").json()
                    reverted_item = next(item for item in reverted_catalog if item["id"] == "background_affinity_limit")
                    self.assertFalse(reverted_item["applied"])

                    client.post(
                        "/api/tweaks/background_affinity_limit/apply",
                        json={
                            "params": {
                                "pid": current_process.pid,
                                "name": current_process.name(),
                                "cores": [0],
                            }
                        },
                    )
                    rollback_response = client.post("/api/trust-center/rollback-all")
                    self.assertEqual(rollback_response.status_code, 200)
                    self.assertGreaterEqual(rollback_response.json()["restored_count"], 1)

                config.get_settings.cache_clear()

    def test_ui_pages_do_not_ship_known_fake_literals(self):
        pulseboost_root = Path(__file__).resolve().parents[1]
        page_files = [
            pulseboost_root / "ui" / "src" / "pages" / "NetworkPage.jsx",
            pulseboost_root / "ui" / "src" / "pages" / "GpuPage.jsx",
            pulseboost_root / "ui" / "src" / "pages" / "BenchmarkPage.jsx",
            pulseboost_root / "ui" / "src" / "pages" / "ProfilesPage.jsx",
            pulseboost_root / "ui" / "src" / "pages" / "AccountPage.jsx",
            pulseboost_root / "ui" / "src" / "pages" / "PulseCorePage.jsx",
        ]
        fake_literals = [
            "412 / 38 Mbps",
            "1,240 RPM",
            "65°C",
            "XXXX-XXXX-XXXX-XXXX",
            "Previous system state snapshot",
            "Expected: 2",
        ]

        combined_text = "\n".join(path.read_text(encoding="utf-8") for path in page_files)
        for literal in fake_literals:
            self.assertNotIn(literal, combined_text)


if __name__ == "__main__":
    unittest.main()


