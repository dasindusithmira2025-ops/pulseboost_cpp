import unittest

from core.cognition.suggestions import generate_smart_suggestions


class SuggestionsEngineTests(unittest.TestCase):
    def test_cpu_idle_and_ram_pressure_rules(self) -> None:
        suggestions = generate_smart_suggestions(
            metrics={"cpu_total": 42.0, "ram_percent": 86.0},
            session_mode="normal",
            thermal_state={},
            active_session={},
        )
        suggestion_ids = {item["id"] for item in suggestions}
        self.assertIn("cpu_idle_high", suggestion_ids)
        self.assertIn("ram_pressure", suggestion_ids)

    def test_missing_game_profile_rule(self) -> None:
        suggestions = generate_smart_suggestions(
            metrics={"cpu_total": 12.0, "ram_percent": 48.0},
            session_mode="gaming",
            thermal_state={},
            active_session={"game_name": "valorant.exe", "profile_name": ""},
        )
        suggestion_ids = {item["id"] for item in suggestions}
        self.assertIn("missing_game_profile", suggestion_ids)

    def test_suggestion_output_is_limited(self) -> None:
        suggestions = generate_smart_suggestions(
            metrics={
                "cpu_total": 45.0,
                "ram_percent": 91.0,
                "gpu_temp": 90.0,
                "ram_speed_mhz": 3200.0,
                "ram_speed_spec_mhz": 6000.0,
                "pagefile_drive": "C:",
                "system_drive_is_ssd": True,
            },
            session_mode="normal",
            thermal_state={},
            active_session={"game_name": "cs2.exe", "profile_name": ""},
        )
        self.assertLessEqual(len(suggestions), 3)


if __name__ == "__main__":
    unittest.main()
