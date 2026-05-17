import json
import tempfile
import unittest
from pathlib import Path

from core.game_library import InstalledGameScanner


class InstalledGameScannerTests(unittest.TestCase):
    def test_scanner_reads_epic_manifest_catalog(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            manifest_dir = root / "EpicManifests"
            game_dir = root / "Epic" / "Fortnite"
            game_dir.mkdir(parents=True, exist_ok=True)
            (game_dir / "FortniteClient-Win64-Shipping.exe").write_bytes(b"exe")

            payload = {
                "DisplayName": "Fortnite",
                "AppName": "Fortnite",
                "InstallLocation": str(game_dir),
                "LaunchExecutable": "FortniteClient-Win64-Shipping.exe",
            }
            manifest_dir.mkdir(parents=True, exist_ok=True)
            (manifest_dir / "fortnite.item").write_text(json.dumps(payload), encoding="utf-8")

            scanner = InstalledGameScanner(
                refresh_interval_seconds=60.0,
                epic_manifest_dirs=[manifest_dir],
                gog_root_candidates=[],
                xbox_root_candidates=[],
                manual_root_candidates=[],
            )
            games = scanner.list_installed_games(force_refresh=True)
            self.assertTrue(any(item["name"] == "Fortnite" for item in games))
            fortnite = next(item for item in games if item["name"] == "Fortnite")
            self.assertEqual(fortnite["source"], "epic")
            self.assertEqual(fortnite["app_id"], "Fortnite")
            self.assertTrue(str(fortnite.get("executable_path") or "").lower().endswith("fortniteclient-win64-shipping.exe"))

    def test_scanner_reads_manual_game_roots(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir) / "Games"
            game_dir = root / "Custom Racer"
            game_dir.mkdir(parents=True, exist_ok=True)
            (game_dir / "customracer.exe").write_bytes(b"executable")

            scanner = InstalledGameScanner(
                refresh_interval_seconds=60.0,
                epic_manifest_dirs=[],
                gog_root_candidates=[],
                xbox_root_candidates=[],
                manual_root_candidates=[root],
            )
            games = scanner.list_installed_games(force_refresh=True)
            self.assertTrue(any(item["name"] == "Custom Racer" for item in games))
            racer = next(item for item in games if item["name"] == "Custom Racer")
            self.assertEqual(racer["source"], "manual_path")
            self.assertTrue(str(racer.get("install_dir") or "").endswith("Custom Racer"))


if __name__ == "__main__":
    unittest.main()
