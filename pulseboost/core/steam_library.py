from __future__ import annotations

import os
import re
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


STEAM_ENV_KEYS = ("PULSEBOOST_STEAM_PATH", "STEAM_PATH")
DEFAULT_STEAM_ROOTS = (
    Path(r"C:\Program Files (x86)\Steam"),
    Path(r"C:\Program Files\Steam"),
)


@dataclass(slots=True)
class SteamGame:
    app_id: str
    name: str
    install_dir: str
    manifest_path: str
    executable_path: str | None = None
    source: str = "steam_manifest"

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


class SteamLibraryScanner:
    def __init__(
        self,
        *,
        steam_root_candidates: list[str | Path] | None = None,
        refresh_interval_seconds: float = 300.0,
    ) -> None:
        self._explicit_roots = [Path(item) for item in (steam_root_candidates or [])]
        self.refresh_interval_seconds = max(0.0, float(refresh_interval_seconds))
        self._cached_games: list[SteamGame] = []
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
        deduplicated: dict[str, SteamGame] = {}
        for game in games:
            key = str(game.app_id).strip() or game.name.lower()
            if key not in deduplicated:
                deduplicated[key] = game
        self._cached_games = sorted(deduplicated.values(), key=lambda item: item.name.lower())
        self._cached_at = now
        return [item.to_dict() for item in self._cached_games]

    def _scan_games(self) -> list[SteamGame]:
        games: list[SteamGame] = []
        for library_root in self._discover_library_roots():
            steamapps = library_root / "steamapps"
            if not steamapps.exists():
                continue
            for manifest_path in steamapps.glob("appmanifest_*.acf"):
                payload = self._parse_acf_manifest(manifest_path)
                app_id = (
                    str(payload.get("appid") or "").strip()
                    or manifest_path.stem.replace("appmanifest_", "")
                )
                name = str(payload.get("name") or "").strip() or f"Steam App {app_id}"
                install_folder = str(payload.get("installdir") or "").strip() or name
                install_dir = steamapps / "common" / install_folder
                executable_path = self._find_primary_executable(install_dir)
                games.append(
                    SteamGame(
                        app_id=app_id,
                        name=name,
                        install_dir=str(install_dir),
                        manifest_path=str(manifest_path),
                        executable_path=str(executable_path) if executable_path else None,
                    )
                )
        return games

    def _discover_library_roots(self) -> list[Path]:
        discovered: list[Path] = []
        for steam_root in self._discover_steam_roots():
            if (steam_root / "steamapps").exists():
                discovered.append(steam_root)
            library_file = steam_root / "steamapps" / "libraryfolders.vdf"
            for parsed in self._parse_libraryfolders(library_file):
                candidate = Path(parsed)
                if (candidate / "steamapps").exists():
                    discovered.append(candidate)
        return self._dedupe_paths(discovered)

    def _discover_steam_roots(self) -> list[Path]:
        if self._explicit_roots:
            return self._dedupe_paths(self._explicit_roots)

        candidates: list[Path] = []
        for env_key in STEAM_ENV_KEYS:
            raw_value = os.environ.get(env_key)
            raw = Path(raw_value).expanduser() if raw_value else None
            if raw:
                candidates.append(raw)
        candidates.extend(self._roots_from_registry())
        candidates.extend(DEFAULT_STEAM_ROOTS)
        return self._dedupe_paths(candidates)

    def _roots_from_registry(self) -> list[Path]:
        try:
            import winreg  # type: ignore
        except Exception:
            return []

        roots: list[Path] = []
        lookups = (
            (winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam", "SteamPath"),
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Valve\Steam", "InstallPath"),
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Valve\Steam", "InstallPath"),
        )
        for hive, key_path, value_name in lookups:
            try:
                with winreg.OpenKey(hive, key_path) as key:
                    raw_value, _ = winreg.QueryValueEx(key, value_name)
            except OSError:
                continue
            if not raw_value:
                continue
            roots.append(Path(str(raw_value)))
        return roots

    def _parse_libraryfolders(self, vdf_path: Path) -> list[str]:
        if not vdf_path.exists():
            return []
        try:
            text = vdf_path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            return []

        paths: list[str] = []
        for key, value in re.findall(r'"([^"]+)"\s*"([^"]*)"', text):
            normalized_value = self._decode_vdf_path(value)
            lower_key = key.lower()
            if lower_key == "path" and normalized_value:
                paths.append(normalized_value)
                continue
            if key.isdigit() and self._looks_like_windows_path(normalized_value):
                paths.append(normalized_value)
        return paths

    def _parse_acf_manifest(self, manifest_path: Path) -> dict[str, str]:
        try:
            text = manifest_path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            return {}
        parsed: dict[str, str] = {}
        for key, value in re.findall(r'"([^"]+)"\s*"([^"]*)"', text):
            parsed[key.lower()] = value
        return parsed

    def _find_primary_executable(self, install_dir: Path) -> Path | None:
        if not install_dir.exists() or not install_dir.is_dir():
            return None

        ignore_names = {
            "dxsetup.exe",
            "unins000.exe",
            "unins001.exe",
            "vcredist_x64.exe",
            "vcredist_x86.exe",
        }
        candidates: list[tuple[int, Path]] = []

        def add_candidates(search_dir: Path) -> None:
            if not search_dir.exists() or not search_dir.is_dir():
                return
            for item in search_dir.glob("*.exe"):
                name = item.name.lower()
                if name in ignore_names or name.startswith("unins"):
                    continue
                try:
                    size = int(item.stat().st_size)
                except OSError:
                    size = 0
                candidates.append((size, item))

        add_candidates(install_dir)
        for subdir_name in ("bin", "binaries", "game", "win64", "x64"):
            add_candidates(install_dir / subdir_name)

        if not candidates:
            return None
        candidates.sort(key=lambda item: item[0], reverse=True)
        return candidates[0][1]

    @staticmethod
    def _decode_vdf_path(value: str) -> str:
        return str(value or "").replace("\\\\", "\\").strip()

    @staticmethod
    def _looks_like_windows_path(value: str) -> bool:
        sample = str(value or "").strip()
        if not sample:
            return False
        return bool(re.match(r"^[A-Za-z]:\\", sample) or sample.startswith("\\\\"))

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
