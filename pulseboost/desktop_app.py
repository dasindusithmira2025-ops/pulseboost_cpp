from __future__ import annotations

import atexit
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
import webbrowser
from pathlib import Path

import webview


PULSEBOOST_ROOT = Path(__file__).resolve().parent
WORKSPACE_ROOT = PULSEBOOST_ROOT.parent
PYTHON_EXE = PULSEBOOST_ROOT / "tools" / "python" / "python.exe"
LOG_DIR = PULSEBOOST_ROOT / "run"
DATA_DIR = WORKSPACE_ROOT / "data"
BACKEND_PORT = 18400
BACKEND_URL = f"http://127.0.0.1:{BACKEND_PORT}"
APP_VERSION = "0.2.0"

backend_process: subprocess.Popen | None = None
desktop_window: webview.Window | None = None
backend_stdout_handle = None
backend_stderr_handle = None
_app_log_handle = open(WORKSPACE_ROOT / "pulseboost.log", "a", encoding="utf-8")
atexit.register(_app_log_handle.close)


def wait_for_backend(timeout_seconds: int = 30) -> None:
    deadline = time.time() + timeout_seconds
    last_error: Exception | None = None
    while time.time() < deadline:
        try:
            with urllib.request.urlopen(f"{BACKEND_URL}/healthz", timeout=2) as response:
                if response.status == 200:
                    return
        except (OSError, urllib.error.URLError) as error:
            last_error = error
            time.sleep(0.4)
    if last_error:
        raise RuntimeError("Guardian backend did not become ready in time.") from last_error
    raise RuntimeError("Guardian backend did not become ready in time.")


def start_backend() -> None:
    global backend_process, backend_stdout_handle, backend_stderr_handle

    if backend_process and backend_process.poll() is None:
        wait_for_backend()
        return

    close_backend_log_handles()
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    backend_stdout_handle = open(LOG_DIR / "desktop-backend.out.log", "a", encoding="utf-8")
    backend_stderr_handle = open(LOG_DIR / "desktop-backend.err.log", "a", encoding="utf-8")

    backend_process = subprocess.Popen(
        [
            str(PYTHON_EXE),
            "-m",
            "uvicorn",
            "serve_app:app",
            "--host",
            "127.0.0.1",
            "--port",
            str(BACKEND_PORT),
        ],
        cwd=str(WORKSPACE_ROOT),
        env={**os.environ, "DESKTOP_RUNTIME": "pywebview"},
        stdout=backend_stdout_handle,
        stderr=backend_stderr_handle,
    )
    wait_for_backend()


def stop_backend() -> None:
    global backend_process

    if not backend_process:
        close_backend_log_handles()
        return
    if backend_process.poll() is not None:
        backend_process = None
        close_backend_log_handles()
        return

    backend_process.terminate()
    try:
        backend_process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        backend_process.kill()
        backend_process.wait(timeout=5)
    finally:
        backend_process = None
        close_backend_log_handles()


def close_backend_log_handles() -> None:
    global backend_stdout_handle, backend_stderr_handle

    if backend_stdout_handle:
        try:
            backend_stdout_handle.flush()
        except Exception:
            pass
        try:
            backend_stdout_handle.close()
        except Exception:
            pass
        backend_stdout_handle = None

    if backend_stderr_handle:
        try:
            backend_stderr_handle.flush()
        except Exception:
            pass
        try:
            backend_stderr_handle.close()
        except Exception:
            pass
        backend_stderr_handle = None


def restart_backend() -> None:
    stop_backend()
    start_backend()


class DesktopBridge:
    def __init__(self) -> None:
        self._is_maximized = False

    def get_meta(self) -> dict:
        return {
            "isDesktop": True,
            "backendUrl": BACKEND_URL,
            "logDir": str(LOG_DIR),
            "dataDir": str(DATA_DIR),
            "runtime": "pywebview",
            "version": APP_VERSION,
            "windowControls": True,
        }

    def open_logs(self) -> bool:
        LOG_DIR.mkdir(parents=True, exist_ok=True)
        os.startfile(str(LOG_DIR))
        return True

    def open_data_dir(self) -> bool:
        DATA_DIR.mkdir(parents=True, exist_ok=True)
        os.startfile(str(DATA_DIR))
        return True

    def open_external(self, url: str) -> bool:
        if not url:
            return False
        webbrowser.open(url)
        return True

    def restart_guardian(self) -> dict:
        restart_backend()
        return {"ok": True}

    def minimize_window(self) -> bool:
        if desktop_window is None:
            return False
        desktop_window.minimize()
        return True

    def toggle_maximize_window(self) -> dict[str, bool]:
        if desktop_window is None:
            return {"ok": False, "maximized": self._is_maximized}
        if self._is_maximized:
            desktop_window.restore()
        else:
            desktop_window.maximize()
        self._is_maximized = not self._is_maximized
        return {"ok": True, "maximized": self._is_maximized}

    def close_window(self) -> bool:
        if desktop_window is None:
            return False
        desktop_window.destroy()
        return True


def inject_bridge(window: webview.Window) -> None:
    script = """
        window.pulseboostDesktop = {
          isDesktop: true,
          getMeta: function () { return window.pywebview.api.get_meta(); },
          openLogs: function () { return window.pywebview.api.open_logs(); },
          openDataDir: function () { return window.pywebview.api.open_data_dir(); },
          openExternal: function (url) { return window.pywebview.api.open_external(url); },
          restartGuardian: function () { return window.pywebview.api.restart_guardian(); },
          minimizeWindow: function () { return window.pywebview.api.minimize_window(); },
          toggleMaximizeWindow: function () { return window.pywebview.api.toggle_maximize_window(); },
          closeWindow: function () { return window.pywebview.api.close_window(); }
        };
        window.dispatchEvent(new Event("pulseboost-desktop-ready"));
    """
    for _ in range(50):
        try:
            window.evaluate_js(script)
            return
        except Exception:
            time.sleep(0.2)


def main() -> None:
    global desktop_window

    if not PYTHON_EXE.exists():
        raise FileNotFoundError(f"Portable Python runtime was not found at {PYTHON_EXE}")

    start_backend()
    atexit.register(stop_backend)
    atexit.register(close_backend_log_handles)

    bridge = DesktopBridge()
    desktop_window = webview.create_window(
        "PulseBoost",
        BACKEND_URL,
        js_api=bridge,
        width=1600,
        height=980,
        min_size=(1200, 760),
        background_color="#0f1117",
        confirm_close=True,
    )
    desktop_window.events.closed += stop_backend
    webview.start(inject_bridge, desktop_window, debug=False)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"[PulseBoost Desktop] {error}", file=sys.stderr)
        raise
