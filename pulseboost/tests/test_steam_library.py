import tempfile
import unittest
from pathlib import Path

from core.steam_library import SteamLibraryScanner


class SteamLibraryScannerTests(unittest.TestCase):
    def test_scanner_reads_library_manifests(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir) / "Steam"
            steamapps = root / "steamapps"
            common = steamapps / "common"
            game_dir = common / "Dota 2 beta"
            game_dir.mkdir(parents=True, exist_ok=True)
            (game_dir / "game.exe").write_bytes(b"fake-executable")

            (steamapps / "appmanifest_570.acf").write_text(
                '"AppState"\n{\n    "appid"      "570"\n    "name"       "Dota 2"\n    "installdir" "Dota 2 beta"\n}\n',
                encoding="utf-8",
            )

            scanner = SteamLibraryScanner(steam_root_candidates=[root], refresh_interval_seconds=60.0)
            games = scanner.list_installed_games(force_refresh=True)

            self.assertEqual(len(games), 1)
            self.assertEqual(games[0]["app_id"], "570")
            self.assertEqual(games[0]["name"], "Dota 2")
            self.assertTrue(str(games[0]["install_dir"]).endswith("Dota 2 beta"))
            self.assertTrue(str(games[0]["executable_path"]).lower().endswith("game.exe"))


if __name__ == "__main__":
    unittest.main()
