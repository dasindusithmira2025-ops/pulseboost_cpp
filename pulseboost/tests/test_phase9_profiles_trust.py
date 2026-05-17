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
from core.game_profile_service import GameProfileService
from core.optimizer import SystemOptimizer
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard
from core.trust_center import TrustCenterService


class FakeSteamScanner:
    def __init__(self, games: list[dict] | None = None) -> None:
        self.games = games or []

    def list_installed_games(self, *, force_refresh: bool = False) -> list[dict]:
        return list(self.games)


class FakeInstalledGameScanner:
    def __init__(self, games: list[dict] | None = None) -> None:
        self.games = games or []

    def list_installed_games(self, *, force_refresh: bool = False) -> list[dict]:
        return list(self.games)


def build_capabilities(*, is_admin=False):
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


class Phase9ProfilesTrustTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.database = DatabaseService(Path(self.temp_dir.name) / "phase9.db")
        asyncio.run(self.database.initialize())
        self.audit_log = AuditLog(self.database)
        self.revert_manager = RevertManager(self.database, self.audit_log)
        self.optimizer = SystemOptimizer(
            database=self.database,
            audit_log=self.audit_log,
            revert_manager=self.revert_manager,
            safety_guard=SafetyGuard(),
            capabilities=build_capabilities(is_admin=False),
        )

    def tearDown(self):
        self.temp_dir.cleanup()

    def test_profile_persistence_and_export(self):
        service = GameProfileService(database=self.database, optimizer=self.optimizer)
        saved = asyncio.run(
            service.save_profile(
                "valorant",
                {
                    "game_id": "valorant",
                    "game_name": "Valorant",
                    "recommended_tweaks": ["search_indexer_priority"],
                    "notes": "Keep indexing throttled during matches.",
                },
            )
        )
        self.assertEqual(saved["game_name"], "Valorant")
        exported = asyncio.run(service.export_profile("valorant"))
        self.assertIn('"format": "pbprofile.v1"', exported)

    def test_list_games_includes_steam_manifest_catalog(self):
        service = GameProfileService(
            database=self.database,
            optimizer=self.optimizer,
            steam_scanner=FakeSteamScanner(
                games=[
                    {
                        "app_id": "730",
                        "name": "Counter-Strike 2",
                        "install_dir": r"C:\SteamLibrary\steamapps\common\Counter-Strike Global Offensive",
                        "executable_path": r"C:\SteamLibrary\steamapps\common\Counter-Strike Global Offensive\game\bin\win64\cs2.exe",
                    }
                ]
            ),
        )
        games = asyncio.run(service.list_games())
        self.assertTrue(any(item["game_name"] == "Counter-Strike 2" for item in games))
        cs2 = next(item for item in games if item["game_name"] == "Counter-Strike 2")
        self.assertEqual(cs2.get("steam_app_id"), "730")

    def test_list_games_includes_epic_catalog(self):
        service = GameProfileService(
            database=self.database,
            optimizer=self.optimizer,
            installed_game_scanner=FakeInstalledGameScanner(
                games=[
                    {
                        "app_id": "Fortnite",
                        "name": "Fortnite",
                        "source": "epic",
                        "install_dir": r"C:\Program Files\Epic Games\Fortnite",
                        "executable_path": r"C:\Program Files\Epic Games\Fortnite\FortniteClient-Win64-Shipping.exe",
                    }
                ]
            ),
        )
        games = asyncio.run(service.list_games())
        self.assertTrue(any(item["game_name"] == "Fortnite" for item in games))
        fortnite = next(item for item in games if item["game_name"] == "Fortnite")
        self.assertEqual(fortnite.get("source"), "epic")
        self.assertEqual(fortnite.get("epic_app_id"), "Fortnite")

    def test_trust_center_status_reflects_recovery_and_safeguards(self):
        trust = TrustCenterService(
            database=self.database,
            capabilities=build_capabilities(is_admin=False),
            safety_guard=SafetyGuard(),
            optimizer=self.optimizer,
        )
        asyncio.run(self.database.set_app_state("runtime.recovery", {"status": "pending", "rationale": "unclean exit"}))
        asyncio.run(self.database.set_app_state("runtime.last_clean_exit", {"clean_exit": True}))
        status = asyncio.run(trust.status({"recovery": {"rationale": "unclean exit"}}))
        self.assertFalse(status["admin_status"])
        self.assertTrue(status["dangerous_tweaks_disabled"])
        self.assertIn("Administrator privileges are unavailable.", status["unsupported_capabilities"])

    def test_trust_center_api_and_profile_api(self):
        profile_service = GameProfileService(database=self.database, optimizer=self.optimizer)
        trust_service = TrustCenterService(
            database=self.database,
            capabilities=build_capabilities(is_admin=False),
            safety_guard=SafetyGuard(),
            optimizer=self.optimizer,
        )
        app = FastAPI()
        app.include_router(router)
        app.state.game_profile_service = profile_service
        app.state.trust_center_service = trust_service
        app.state.foundation = {"recovery": {"rationale": "No unclean previous session was detected."}}
        with managed_test_client(app) as client:
            save_response = client.post(
                "/api/games/valorant/profile",
                json={"game_name": "Valorant", "recommended_tweaks": ["search_indexer_priority"], "notes": "profile api test"},
            )
            self.assertEqual(save_response.status_code, 200)
            trust_response = client.get("/api/trust-center/status")
            self.assertEqual(trust_response.status_code, 200)
            export_response = client.get("/api/games/valorant/export")
            self.assertEqual(export_response.status_code, 200)
            self.assertIn("pbprofile.v1", export_response.text)


if __name__ == "__main__":
    unittest.main()


