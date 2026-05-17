from __future__ import annotations

import sys
import types
import unittest
from importlib.machinery import ModuleSpec
from unittest.mock import patch

fake_webview = types.ModuleType("webview")
fake_webview.Window = object
fake_webview.create_window = lambda *args, **kwargs: None
fake_webview.start = lambda *args, **kwargs: None
fake_webview.__spec__ = ModuleSpec("webview", loader=None)
sys.modules.setdefault("webview", fake_webview)

import desktop_app
from api.main import resolved_desktop_runtime


class DesktopBridgeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.previous_window = desktop_app.desktop_window
        desktop_app.desktop_window = None

    def tearDown(self) -> None:
        desktop_app.desktop_window = self.previous_window

    def test_bridge_meta_exposes_desktop_runtime_fields(self) -> None:
        bridge = desktop_app.DesktopBridge()

        meta = bridge.get_meta()

        self.assertTrue(meta["isDesktop"])
        self.assertEqual(meta["runtime"], "pywebview")
        self.assertTrue(meta["windowControls"])
        self.assertIn("dataDir", meta)
        self.assertIn("logDir", meta)
        self.assertIn("version", meta)

    def test_runtime_resolution_defaults_to_pywebview_variants(self) -> None:
        with patch.dict("os.environ", {"DESKTOP_RUNTIME": "desktop-webview"}, clear=False):
            self.assertEqual(resolved_desktop_runtime(), "pywebview")

    def test_bridge_window_actions_fail_closed_without_window(self) -> None:
        bridge = desktop_app.DesktopBridge()

        self.assertFalse(bridge.minimize_window())
        self.assertEqual(
            bridge.toggle_maximize_window(),
            {"ok": False, "maximized": False},
        )
        self.assertFalse(bridge.close_window())


if __name__ == "__main__":
    unittest.main()
