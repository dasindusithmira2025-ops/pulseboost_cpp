import asyncio
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from core.game_detection import GameDetector, monitor_game_close
from core.steam_library import SteamLibraryScanner


class FakeInstalledGameScanner:
    def __init__(self, games: list[dict] | None = None) -> None:
        self.games = games or []

    def list_installed_games(self, *, force_refresh: bool = False) -> list[dict]:
        return list(self.games)


class GameDetectionTests(unittest.TestCase):
    def test_detect_active_game_returns_unified_shape(self) -> None:
        snapshot = SimpleNamespace(
            top_processes=[
                {"name": "cs2.exe", "pid": 4242, "cpu_percent": 55.0, "executable_path": "C:\\Games\\CS2\\cs2.exe"},
                {"name": "discord.exe", "pid": 9000, "cpu_percent": 4.0},
            ]
        )

        detector = GameDetector()
        detected = detector.detect_active_game(snapshot, "gaming")

        self.assertTrue(detected["detected"])
        self.assertEqual(detected["game_name"], "cs2.exe")
        self.assertEqual(detected["process"], "cs2.exe")
        self.assertEqual(detected["profile"], "fps_competitive")
        # Backward-compatible fields consumed by existing services.
        self.assertEqual(detected["name"], "cs2.exe")
        self.assertEqual(detected["pid"], 4242)

    def test_detect_active_game_normal_mode_returns_not_detected(self) -> None:
        snapshot = SimpleNamespace(top_processes=[{"name": "valorant.exe", "pid": 12, "cpu_percent": 72.0}])
        detector = GameDetector()
        detected = detector.detect_active_game(snapshot, "normal")
        self.assertEqual(
            detected,
            {
                "detected": False,
                "game_name": None,
                "process": None,
                "profile": None,
            },
        )

    def test_detect_active_game_does_not_treat_code_as_cod(self) -> None:
        snapshot = SimpleNamespace(
            top_processes=[
                {"name": "Code.exe", "pid": 1324, "cpu_percent": 30.0, "executable_path": r"C:\Program Files\Microsoft VS Code\Code.exe"},
                {"name": "explorer.exe", "pid": 103, "cpu_percent": 1.0},
            ]
        )
        detector = GameDetector()
        detected = detector.detect_active_game(snapshot, "gaming")
        self.assertFalse(detected["detected"])

    def test_detect_active_game_uses_steam_install_path_match(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            steam_root = Path(temp_dir) / "Steam"
            steamapps = steam_root / "steamapps"
            common = steamapps / "common"
            game_dir = common / "Counter-Strike Global Offensive"
            game_bin = game_dir / "game" / "bin" / "win64"
            game_bin.mkdir(parents=True, exist_ok=True)
            (steamapps / "libraryfolders.vdf").write_text(
                '"libraryfolders"\n{\n    "0"\n    {\n        "path"  "C:\\\\Fake"\n    }\n}\n',
                encoding="utf-8",
            )
            (steamapps / "appmanifest_730.acf").write_text(
                '"AppState"\n{\n    "appid"      "730"\n    "name"       "Counter-Strike 2"\n    "installdir" "Counter-Strike Global Offensive"\n}\n',
                encoding="utf-8",
            )

            scanner = SteamLibraryScanner(steam_root_candidates=[steam_root], refresh_interval_seconds=9999)
            detector = GameDetector(steam_scanner=scanner)
            snapshot = SimpleNamespace(
                top_processes=[
                    {
                        "name": "cs2.exe",
                        "pid": 4242,
                        "cpu_percent": 60.0,
                        "executable_path": str(game_bin / "cs2.exe"),
                    }
                ]
            )

            detected = detector.detect_active_game(snapshot, "gaming")
            self.assertTrue(detected["detected"])
            self.assertEqual(detected["game_name"], "Counter-Strike 2")
            self.assertEqual(detected["steam_app_id"], "730")
            self.assertEqual(detected["detected_by"], "steam_path_match")

    def test_detect_active_game_uses_epic_install_path_match(self) -> None:
        scanner = FakeInstalledGameScanner(
            games=[
                {
                    "name": "Fortnite",
                    "source": "epic",
                    "app_id": "Fortnite",
                    "install_dir": r"C:\Program Files\Epic Games\Fortnite",
                    "executable_path": r"C:\Program Files\Epic Games\Fortnite\FortniteClient-Win64-Shipping.exe",
                }
            ]
        )
        detector = GameDetector(installed_game_scanner=scanner)
        snapshot = SimpleNamespace(
            top_processes=[
                {
                    "name": "FortniteClient-Win64-Shipping.exe",
                    "pid": 4455,
                    "cpu_percent": 44.0,
                    "executable_path": r"C:\Program Files\Epic Games\Fortnite\FortniteClient-Win64-Shipping.exe",
                }
            ]
        )

        detected = detector.detect_active_game(snapshot, "gaming")
        self.assertTrue(detected["detected"])
        self.assertEqual(detected["game_name"], "Fortnite")
        self.assertEqual(detected["source"], "epic")
        self.assertEqual(detected["epic_app_id"], "Fortnite")
        self.assertEqual(detected["detected_by"], "epic_path_match")

    def test_monitor_game_close_emits_event_when_process_disappears(self) -> None:
        events: list[dict] = []

        def callback(payload: dict) -> None:
            events.append(payload)

        with patch(
            "core.game_detection._running_process_names",
            side_effect=[{"valorant.exe"}, set()],
        ):
            asyncio.run(
                monitor_game_close(
                    callback,
                    active_process="valorant.exe",
                    poll_interval_seconds=0,
                )
            )

        self.assertEqual(len(events), 1)
        self.assertEqual(events[0]["event"], "game_closed")
        self.assertEqual(events[0]["process"], "valorant.exe")


if __name__ == "__main__":
    unittest.main()
