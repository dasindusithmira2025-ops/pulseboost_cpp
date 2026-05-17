from __future__ import annotations

import asyncio
import re
from collections.abc import Callable
from contextlib import suppress
from typing import Any

from core.game_library import InstalledGameScanner
from core.steam_library import SteamLibraryScanner

try:
    import psutil
except ImportError:  # pragma: no cover
    psutil = None

GAME_EXECUTABLE_HINTS = {
    "cs2.exe",
    "apex_legends.exe",
    "riotclientservices.exe",
    "valorant.exe",
    "fortnite.exe",
    "r5apex.exe",
    "cod.exe",
    "overwatch.exe",
    "dota2.exe",
    "csgo.exe",
}

GAME_TOKEN_HINTS = (
    "cs2",
    "apex",
    "valorant",
    "fortnite",
    "r5apex",
    "overwatch",
    "dota2",
    "csgo",
    "leagueoflegends",
    "eldenring",
    "rocketleague",
    "pubg",
)

LAUNCHER_NAMES = {
    "steam.exe",
    "epicgameslauncher.exe",
    "discord.exe",
    "riotclientservices.exe",
    "gameoverlayui.exe",
    "gameoverlayui64.exe",
    "steamwebhelper.exe",
    "gamemanager.exe",
}

PROFILE_BY_PROCESS = {
    "cs2.exe": "fps_competitive",
    "csgo.exe": "fps_competitive",
    "valorant.exe": "fps_competitive",
    "overwatch.exe": "fps_competitive",
    "cod.exe": "fps_competitive",
    "apex_legends.exe": "battle_royale",
    "r5apex.exe": "battle_royale",
    "fortnite.exe": "battle_royale",
    "dota2.exe": "moba",
}


def _empty_detection() -> dict[str, Any]:
    return {
        "detected": False,
        "game_name": None,
        "process": None,
        "profile": None,
    }


def _process_name(process: dict[str, Any]) -> str:
    return str(process.get("name") or "").strip()


def _process_matches(process_name: str) -> bool:
    lower_name = process_name.lower()
    if not lower_name or lower_name in LAUNCHER_NAMES:
        return False
    if lower_name in GAME_EXECUTABLE_HINTS:
        return True
    stem = lower_name[:-4] if lower_name.endswith(".exe") else lower_name
    if not stem:
        return False
    if stem in GAME_TOKEN_HINTS:
        return True
    words = [token for token in re.split(r"[^a-z0-9]+", stem) if token]
    return any(token in words for token in GAME_TOKEN_HINTS)


def _normalize_windows_path(path: str | None) -> str:
    if not path:
        return ""
    return str(path).replace("/", "\\").strip().rstrip("\\").lower()


def _path_within(path: str | None, directory: str | None) -> bool:
    normalized_path = _normalize_windows_path(path)
    normalized_dir = _normalize_windows_path(directory)
    if not normalized_path or not normalized_dir:
        return False
    return normalized_path == normalized_dir or normalized_path.startswith(f"{normalized_dir}\\")


def _canonical_source(raw: str) -> str:
    source = str(raw or "").strip().lower()
    if source.startswith("steam"):
        return "steam"
    if source.startswith("epic"):
        return "epic"
    if source.startswith("gog"):
        return "gog"
    if source.startswith("xbox"):
        return "xbox"
    return source or "catalog"


def _detect_from_installed_catalog(processes: list[dict[str, Any]], installed_games: list[dict[str, Any]]) -> dict[str, Any] | None:
    candidates: list[tuple[float, dict[str, Any], dict[str, Any]]] = []
    for process in processes:
        process_name = _process_name(process)
        if process_name.lower() in LAUNCHER_NAMES:
            continue
        executable_path = process.get("executable_path")
        if not executable_path:
            continue
        for installed_game in installed_games:
            if not _path_within(executable_path, installed_game.get("install_dir")):
                continue
            cpu_score = float(process.get("cpu_percent") or 0.0)
            candidates.append((cpu_score, process, installed_game))

    if not candidates:
        return None

    _, best_process, best_game = max(candidates, key=lambda item: item[0])
    process_name = _process_name(best_process) or "unknown.exe"
    lower_process_name = process_name.lower()
    game_name = str(best_game.get("name") or "").strip() or process_name
    source = _canonical_source(best_game.get("source") or "catalog")
    result: dict[str, Any] = {
        "detected": True,
        "game_name": game_name,
        "process": process_name,
        "profile": PROFILE_BY_PROCESS.get(lower_process_name),
        "name": game_name,
        "pid": best_process.get("pid"),
        "cpu_percent": best_process.get("cpu_percent", 0),
        "executable_path": best_process.get("executable_path") or process_name,
        "source": source,
        "confidence": 0.95 if source in {"steam", "epic", "gog_registry", "xbox_registry"} else 0.9,
        "detected_by": f"{source}_path_match",
    }
    app_id = best_game.get("app_id")
    if app_id is not None:
        result["app_id"] = app_id
    if source == "steam":
        result["steam_app_id"] = app_id
    elif source == "epic":
        result["epic_app_id"] = app_id
    elif source == "gog":
        result["gog_game_id"] = app_id
    elif source == "xbox":
        result["xbox_game_id"] = app_id
    return result


def detect_active_game(
    snapshot,
    session_mode: str,
    *,
    installed_games: list[dict[str, Any]] | None = None,
) -> dict[str, Any]:
    if session_mode != "gaming":
        return _empty_detection()

    processes = getattr(snapshot, "top_processes", []) or []
    catalog_detected = _detect_from_installed_catalog(processes, installed_games or [])
    if catalog_detected:
        return catalog_detected

    candidates: list[dict[str, Any]] = []
    for process in processes:
        process_name = _process_name(process)
        if not _process_matches(process_name):
            continue
        candidates.append(process)

    if not candidates:
        return _empty_detection()

    best = sorted(candidates, key=lambda item: item.get("cpu_percent", 0), reverse=True)[0]
    detected_name = _process_name(best) or "unknown.exe"
    lower_name = detected_name.lower()
    profile = PROFILE_BY_PROCESS.get(lower_name)

    return {
        "detected": True,
        "game_name": detected_name,
        "process": detected_name,
        "profile": profile,
        # Backward-compatible fields for existing services.
        "name": detected_name,
        "pid": best.get("pid"),
        "cpu_percent": best.get("cpu_percent", 0),
        "executable_path": best.get("executable_path") or detected_name,
        "confidence": 0.8,
        "detected_by": "top_process_hints",
    }


async def monitor_game_close(
    callback: Callable[[dict[str, Any]], Any],
    *,
    active_process: str,
    poll_interval_seconds: float = 5.0,
    stop_event: asyncio.Event | None = None,
) -> None:
    if not active_process:
        return

    target = active_process.lower()
    while True:
        if stop_event and stop_event.is_set():
            return
        process_names = _running_process_names()
        if target not in process_names:
            payload = {"event": "game_closed", "process": active_process}
            result = callback(payload)
            if asyncio.iscoroutine(result):
                await result
            return
        await asyncio.sleep(max(0.1, poll_interval_seconds))


def _running_process_names() -> set[str]:
    if not psutil:
        return set()
    names: set[str] = set()
    with suppress(Exception):
        for process in psutil.process_iter(attrs=["name"]):
            name = str(process.info.get("name") or "").strip().lower()
            if name:
                names.add(name)
    return names


class GameDetector:
    def __init__(
        self,
        *,
        steam_scanner: SteamLibraryScanner | None = None,
        installed_game_scanner: InstalledGameScanner | None = None,
    ) -> None:
        self.steam_scanner = steam_scanner
        self.installed_game_scanner = installed_game_scanner

    def detect(self, snapshot, session_mode: str) -> dict[str, Any] | None:
        detected = self.detect_active_game(snapshot, session_mode)
        return detected if detected.get("detected") else None

    def detect_active_game(self, snapshot, session_mode: str) -> dict[str, Any]:
        installed_games: list[dict[str, Any]] = []
        if self.installed_game_scanner:
            try:
                installed_games = self.installed_game_scanner.list_installed_games()
            except Exception:
                installed_games = []
        elif self.steam_scanner:
            # Backward-compatible fallback for tests or callers still injecting only a Steam scanner.
            try:
                installed_games = self.steam_scanner.list_installed_games()
            except Exception:
                installed_games = []
        return detect_active_game(snapshot, session_mode, installed_games=installed_games)

    async def monitor_game_close(
        self,
        callback: Callable[[dict[str, Any]], Any],
        *,
        active_process: str,
        poll_interval_seconds: float = 5.0,
        stop_event: asyncio.Event | None = None,
    ) -> None:
        await monitor_game_close(
            callback,
            active_process=active_process,
            poll_interval_seconds=poll_interval_seconds,
            stop_event=stop_event,
        )
