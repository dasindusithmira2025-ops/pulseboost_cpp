from __future__ import annotations

import json
import os
import re
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

from core.steam_library import SteamLibraryScanner


EPIC_MANIFEST_ENV_KEYS = ("PULSEBOOST_EPIC_MANIFEST_DIR",)
GOG_ROOT_ENV_KEYS = ("PULSEBOOST_GOG_ROOTS", "PULSEBOOST_GOG_ROOT")
XBOX_ROOT_ENV_KEYS = ("PULSEBOOST_XBOX_ROOTS", "PULSEBOOST_XBOX_ROOT")
MANUAL_ROOT_ENV_KEYS = ("PULSEBOOST_GAME_ROOTS",)

DEFAULT_EPIC_MANIFEST_DIRS = (
    Path(r"C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests"),
)
DEFAULT_GOG_ROOTS = (
    Path(r"C:\GOG Games"),
    Path(r"C:\Program Files (x86)\GOG Galaxy\Games"),
    Path(r"C:\Program Files\GOG Galaxy\Games"),
)
DEFAULT_XBOX_ROOTS = (
    Path(r"C:\XboxGames"),
    Path(r"D:\XboxGames"),
    Path(r"E:\XboxGames"),
)
DEFAULT_MANUAL_ROOTS = (
    Path(r"C:\Games"),
    Path(r"D:\Games"),
    Path(r"E:\Games"),
    Path(r"C:\Program Files\Games"),
    Path(r"C:\Program Files (x86)\Games"),
    Path(r"C:\Program Files\Epic Games"),
)


@dataclass(slots=True)
class InstalledGame:
    name: str
    install_dir: str
    source: str
    app_id: str | None = None
    executable_path: str | None = None
    manifest_path: str | None = None
    launcher: str | None = None
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


class InstalledGameScanner:
    def __init__(
        self,
        *,
        steam_scanner: SteamLibraryScanner | None = None,
        refresh_interval_seconds: float = 300.0,
        epic_manifest_dirs: list[str | Path] | None = None,
        gog_root_candidates: list[str | Path] | None = None,
        xbox_root_candidates: list[str | Path] | None = None,
        manual_root_candidates: list[str | Path] | None = None,
    ) -> None:
        self.steam_scanner = steam_scanner
        self.refresh_interval_seconds = max(0.0, float(refresh_interval_seconds))
        self._explicit_epic_manifest_dirs = [Path(item) for item in (epic_manifest_dirs or [])]
        self._explicit_gog_roots = [Path(item) for item in (gog_root_candidates or [])]
        self._explicit_xbox_roots = [Path(item) for item in (xbox_root_candidates or [])]
        self._explicit_manual_roots = [Path(item) for item in (manual_root_candidates or [])]
        self._cached_games: list[InstalledGame] = []
        self._cached_at: float = 0.0

    def list_installed_games(self, *, force_refresh: bool = False) -> list[dict[str, Any]]:
        now = time.time()
        cache_valid = (
            not force_refresh
            and self._cached_games
            and (now - self._cached_at) < self.refresh_interval_seconds
        )
        if cache_valid:
            return [item.to_dict() for item in self._cached_games]

        games = self._scan_games()
        deduplicated: dict[str, InstalledGame] = {}
        for game in games:
            key = self._game_key(game)
            existing = deduplicated.get(key)
            if existing is None:
                deduplicated[key] = game
                continue
            # Prefer entries with a direct executable for session detection precision.
            if not existing.executable_path and game.executable_path:
                deduplicated[key] = game
        self._cached_games = sorted(deduplicated.values(), key=lambda item: item.name.lower())
        self._cached_at = now
        return [item.to_dict() for item in self._cached_games]

    def _scan_games(self) -> list[InstalledGame]:
        games: list[InstalledGame] = []
        games.extend(self._scan_steam())
        games.extend(self._scan_epic())
        games.extend(self._scan_gog_registry())
        games.extend(self._scan_xbox_registry())
        games.extend(self._scan_roots_as_games(self._discover_gog_roots(), source="gog_path", launcher="gog"))
        games.extend(self._scan_roots_as_games(self._discover_xbox_roots(), source="xbox_path", launcher="xbox"))
        games.extend(self._scan_roots_as_games(self._discover_manual_roots(), source="manual_path", launcher="manual"))
        return games

    def _scan_steam(self) -> list[InstalledGame]:
        if not self.steam_scanner:
            return []
        try:
            payload = self.steam_scanner.list_installed_games()
        except Exception:
            return []

        games: list[InstalledGame] = []
        for item in payload:
            if not isinstance(item, dict):
                continue
            name = str(item.get("name") or "").strip()
            install_dir = str(item.get("install_dir") or "").strip()
            if not name or not install_dir:
                continue
            app_id = str(item.get("app_id") or "").strip() or None
            games.append(
                InstalledGame(
                    name=name,
                    install_dir=install_dir,
                    source="steam",
                    app_id=app_id,
                    executable_path=str(item.get("executable_path") or "").strip() or None,
                    manifest_path=str(item.get("manifest_path") or "").strip() or None,
                    launcher="steam",
                    metadata={"steam_app_id": app_id} if app_id else {},
                )
            )
        return games

    def _scan_epic(self) -> list[InstalledGame]:
        games: list[InstalledGame] = []
        for manifest_dir in self._discover_epic_manifest_dirs():
            if not manifest_dir.exists() or not manifest_dir.is_dir():
                continue
            try:
                manifest_paths = list(manifest_dir.glob("*.item"))
            except OSError:
                continue
            for manifest_path in manifest_paths:
                payload = self._read_json(manifest_path)
                if not payload:
                    continue
                name = str(payload.get("DisplayName") or payload.get("MainGameAppName") or payload.get("AppName") or "").strip()
                install_location = str(payload.get("InstallLocation") or "").strip()
                if not name or not install_location:
                    continue
                install_dir = Path(install_location)
                launch_executable = str(payload.get("LaunchExecutable") or "").strip()
                executable_path = self._resolve_executable(install_dir, launch_executable)
                app_id = str(payload.get("AppName") or payload.get("CatalogItemId") or payload.get("NamespaceId") or "").strip() or None
                games.append(
                    InstalledGame(
                        name=name,
                        install_dir=str(install_dir),
                        source="epic",
                        app_id=app_id,
                        executable_path=str(executable_path) if executable_path else None,
                        manifest_path=str(manifest_path),
                        launcher="epic",
                        metadata={
                            "epic_namespace": payload.get("NamespaceId"),
                            "epic_catalog_item_id": payload.get("CatalogItemId"),
                        },
                    )
                )
        return games

    def _scan_gog_registry(self) -> list[InstalledGame]:
        entries = self._enumerate_registry_games(
            lookups=(
                (r"HKLM", r"SOFTWARE\WOW6432Node\GOG.com\Games"),
                (r"HKLM", r"SOFTWARE\GOG.com\Games"),
                (r"HKCU", r"Software\GOG.com\Games"),
            ),
            source="gog_registry",
            launcher="gog",
            name_keys=("gamename", "name", "title"),
            install_keys=("path", "workingdir", "installdir", "installpath"),
            executable_keys=("exe", "launchcommand", "executable"),
            id_keys=("gameid", "id"),
        )
        return entries

    def _scan_xbox_registry(self) -> list[InstalledGame]:
        entries = self._enumerate_registry_games(
            lookups=(
                (r"HKCU", r"Software\Microsoft\XboxGames"),
                (r"HKLM", r"SOFTWARE\Microsoft\XboxGames"),
            ),
            source="xbox_registry",
            launcher="xbox",
            name_keys=("title", "name", "displayname"),
            install_keys=("installlocation", "path", "rootfolder"),
            executable_keys=("executable", "launchpath", "exe"),
            id_keys=("titleid", "storeid", "id"),
        )
        return entries

    def _scan_roots_as_games(self, roots: list[Path], *, source: str, launcher: str) -> list[InstalledGame]:
        games: list[InstalledGame] = []
        for root in roots:
            if not root.exists() or not root.is_dir():
                continue
            for game_dir in self._candidate_game_dirs(root):
                executable = self._find_primary_executable(game_dir)
                if executable is None:
                    continue
                game_name = self._friendly_directory_name(game_dir.name)
                if not game_name:
                    continue
                games.append(
                    InstalledGame(
                        name=game_name,
                        install_dir=str(game_dir),
                        source=source,
                        executable_path=str(executable),
                        launcher=launcher,
                        metadata={"scan_root": str(root)},
                    )
                )
        return games

    def _enumerate_registry_games(
        self,
        *,
        lookups: tuple[tuple[str, str], ...],
        source: str,
        launcher: str,
        name_keys: tuple[str, ...],
        install_keys: tuple[str, ...],
        executable_keys: tuple[str, ...],
        id_keys: tuple[str, ...],
    ) -> list[InstalledGame]:
        try:
            import winreg  # type: ignore
        except Exception:
            return []

        hive_map: dict[str, int] = {
            "HKLM": winreg.HKEY_LOCAL_MACHINE,
            "HKCU": winreg.HKEY_CURRENT_USER,
        }
        games: list[InstalledGame] = []
        for hive_name, key_path in lookups:
            hive = hive_map.get(hive_name)
            if hive is None:
                continue
            try:
                with winreg.OpenKey(hive, key_path) as root_key:
                    index = 0
                    while True:
                        try:
                            subkey_name = winreg.EnumKey(root_key, index)
                        except OSError:
                            break
                        index += 1
                        try:
                            with winreg.OpenKey(root_key, subkey_name) as sub_key:
                                values = self._read_registry_values(sub_key)
                        except OSError:
                            continue

                        install_dir_raw = self._first_non_empty(values, install_keys)
                        if not install_dir_raw:
                            continue
                        install_dir = Path(str(install_dir_raw))
                        name = self._first_non_empty(values, name_keys) or self._friendly_directory_name(subkey_name)
                        if not name:
                            continue
                        raw_executable = self._first_non_empty(values, executable_keys)
                        executable_path = self._resolve_executable(install_dir, str(raw_executable or ""))
                        app_id = self._first_non_empty(values, id_keys) or subkey_name
                        games.append(
                            InstalledGame(
                                name=str(name),
                                install_dir=str(install_dir),
                                source=source,
                                app_id=str(app_id),
                                executable_path=str(executable_path) if executable_path else None,
                                manifest_path=f"registry:{hive_name}\\{key_path}\\{subkey_name}",
                                launcher=launcher,
                            )
                        )
            except OSError:
                continue
        return games

    def _discover_epic_manifest_dirs(self) -> list[Path]:
        if self._explicit_epic_manifest_dirs:
            return self._dedupe_paths(self._explicit_epic_manifest_dirs)
        candidates = list(self._paths_from_env(EPIC_MANIFEST_ENV_KEYS))
        candidates.extend(DEFAULT_EPIC_MANIFEST_DIRS)
        return self._dedupe_paths(candidates)

    def _discover_gog_roots(self) -> list[Path]:
        if self._explicit_gog_roots:
            return self._dedupe_paths(self._explicit_gog_roots)
        candidates = list(self._paths_from_env(GOG_ROOT_ENV_KEYS))
        candidates.extend(DEFAULT_GOG_ROOTS)
        return self._dedupe_paths(candidates)

    def _discover_xbox_roots(self) -> list[Path]:
        if self._explicit_xbox_roots:
            return self._dedupe_paths(self._explicit_xbox_roots)
        candidates = list(self._paths_from_env(XBOX_ROOT_ENV_KEYS))
        candidates.extend(self._xbox_roots_from_registry())
        candidates.extend(DEFAULT_XBOX_ROOTS)
        return self._dedupe_paths(candidates)

    def _discover_manual_roots(self) -> list[Path]:
        if self._explicit_manual_roots:
            return self._dedupe_paths(self._explicit_manual_roots)
        candidates = list(self._paths_from_env(MANUAL_ROOT_ENV_KEYS))
        candidates.extend(DEFAULT_MANUAL_ROOTS)
        return self._dedupe_paths(candidates)

    def _xbox_roots_from_registry(self) -> list[Path]:
        try:
            import winreg  # type: ignore
        except Exception:
            return []

        roots: list[Path] = []
        lookups = (
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Microsoft\GamingServices", "AppInstallDir"),
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Microsoft\GamingServices", "InstallLocation"),
            (winreg.HKEY_CURRENT_USER, r"Software\Microsoft\XboxGames", "AppInstallDir"),
        )
        for hive, key_path, value_name in lookups:
            try:
                with winreg.OpenKey(hive, key_path) as key:
                    raw_value, _ = winreg.QueryValueEx(key, value_name)
            except OSError:
                continue
            value = str(raw_value or "").strip()
            if not value:
                continue
            roots.append(Path(value))
        return roots

    def _find_primary_executable(self, install_dir: Path) -> Path | None:
        if not install_dir.exists() or not install_dir.is_dir():
            return None

        ignored_exact = {
            "dxsetup.exe",
            "vcredist_x64.exe",
            "vcredist_x86.exe",
            "eacsetup.exe",
            "easyanticheat_setup.exe",
        }
        demote_tokens = ("launcher", "crash", "report", "updater", "helper")
        search_dirs = (
            install_dir,
            install_dir / "content",
            install_dir / "binaries",
            install_dir / "bin",
            install_dir / "game",
            install_dir / "win64",
            install_dir / "x64",
            install_dir / "shipping",
        )

        candidates: list[tuple[int, Path]] = []
        for search_dir in self._dedupe_paths([item for item in search_dirs if item.exists()]):
            if not search_dir.is_dir():
                continue
            try:
                executables = list(search_dir.glob("*.exe"))
            except OSError:
                continue
            for executable in executables:
                name = executable.name.lower()
                if name in ignored_exact or name.startswith("unins") or name.startswith("setup"):
                    continue
                try:
                    size = int(executable.stat().st_size)
                except OSError:
                    size = 0
                score = size
                root_token = re.sub(r"[^a-z0-9]+", "", install_dir.name.lower())
                exe_token = re.sub(r"[^a-z0-9]+", "", executable.stem.lower())
                if root_token and root_token in exe_token:
                    score += 3_000_000
                if any(token in exe_token for token in demote_tokens):
                    score -= 5_000_000
                if "shipping" in exe_token or "game" in exe_token:
                    score += 1_000_000
                candidates.append((score, executable))

        if not candidates:
            return None
        candidates.sort(key=lambda item: item[0], reverse=True)
        return candidates[0][1]

    def _resolve_executable(self, install_dir: Path, launch_executable: str) -> Path | None:
        launch_path = str(launch_executable or "").strip()
        if launch_path:
            candidate = Path(launch_path)
            if candidate.is_absolute():
                if candidate.exists():
                    return candidate
            else:
                joined = install_dir / candidate
                if joined.exists():
                    return joined
        return self._find_primary_executable(install_dir)

    def _candidate_game_dirs(self, root: Path) -> list[Path]:
        candidates: list[Path] = []
        try:
            root_has_exe = any(root.glob("*.exe"))
        except OSError:
            root_has_exe = False
        if root_has_exe:
            candidates.append(root)

        max_directories = 150
        scanned = 0
        try:
            children = sorted(root.iterdir(), key=lambda item: item.name.lower())
        except OSError:
            return candidates
        for child in children:
            if scanned >= max_directories:
                break
            if not child.is_dir():
                continue
            scanned += 1
            if child.name.startswith("."):
                continue
            candidates.append(child)
        return candidates

    @staticmethod
    def _friendly_directory_name(raw: str) -> str:
        cleaned = str(raw or "").strip()
        if not cleaned:
            return ""
        cleaned = re.sub(r"[_\-]+", " ", cleaned)
        cleaned = re.sub(r"\s+", " ", cleaned).strip()
        return cleaned

    @staticmethod
    def _read_json(path: Path) -> dict[str, Any]:
        try:
            raw = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            return {}
        try:
            payload = json.loads(raw)
        except json.JSONDecodeError:
            return {}
        return payload if isinstance(payload, dict) else {}

    @staticmethod
    def _read_registry_values(key: Any) -> dict[str, Any]:
        try:
            import winreg  # type: ignore
        except Exception:
            return {}
        values: dict[str, Any] = {}
        index = 0
        while True:
            try:
                name, value, _ = winreg.EnumValue(key, index)
            except OSError:
                break
            values[str(name).lower()] = value
            index += 1
        return values

    @staticmethod
    def _first_non_empty(values: dict[str, Any], keys: tuple[str, ...]) -> Any | None:
        for key in keys:
            value = values.get(key)
            text = str(value or "").strip()
            if text:
                return value
        return None

    @staticmethod
    def _paths_from_env(keys: tuple[str, ...]) -> list[Path]:
        paths: list[Path] = []
        for key in keys:
            raw = str(os.environ.get(key) or "").strip()
            if not raw:
                continue
            for token in [item.strip() for item in raw.split(";")]:
                if token:
                    paths.append(Path(token))
        return paths

    @staticmethod
    def _game_key(game: InstalledGame) -> str:
        app_id = str(game.app_id or "").strip().lower()
        install_dir = str(game.install_dir or "").strip().replace("/", "\\").rstrip("\\").lower()
        name = str(game.name or "").strip().lower()
        if app_id:
            return f"{game.source}:{app_id}"
        if install_dir:
            return f"{game.source}:{install_dir}"
        return f"{game.source}:{name}"

    @staticmethod
    def _dedupe_paths(paths: list[Path]) -> list[Path]:
        result: list[Path] = []
        seen: set[str] = set()
        for path in paths:
            normalized = str(path).replace("/", "\\").rstrip("\\").lower()
            if not normalized or normalized in seen:
                continue
            seen.add(normalized)
            result.append(path)
        return result
