from __future__ import annotations

import json
from typing import Any

from core.database import DatabaseService
from core.game_library import InstalledGameScanner
from core.optimizer import SystemOptimizer
from core.steam_library import SteamLibraryScanner


class GameProfileService:
    def __init__(
        self,
        *,
        database: DatabaseService,
        optimizer: SystemOptimizer,
        steam_scanner: SteamLibraryScanner | None = None,
        installed_game_scanner: InstalledGameScanner | None = None,
    ) -> None:
        self.database = database
        self.optimizer = optimizer
        self.steam_scanner = steam_scanner
        self.installed_game_scanner = installed_game_scanner

    async def list_games(self) -> list[dict[str, Any]]:
        profiles = {item["game_id"]: item for item in await self.database.list_game_profiles()}
        sessions = await self.database.list_sessions(limit=200)
        benchmarks = await self.database.list_benchmark_results(limit=200)
        catalog: dict[str, dict[str, Any]] = {}
        discovered_games = self._list_discovered_games()

        for discovered in discovered_games:
            game_name = str(discovered.get("name") or "").strip()
            if not game_name:
                continue
            game_id = self._game_id(game_name)
            source = self._canonical_source(discovered.get("source"))
            app_id = discovered.get("app_id")
            catalog.setdefault(
                game_id,
                {
                    "game_id": game_id,
                    "game_name": game_name,
                    "executable_path": discovered.get("executable_path") or discovered.get("install_dir"),
                    "install_dir": discovered.get("install_dir"),
                    "app_id": app_id,
                    "steam_app_id": app_id if source == "steam" else None,
                    "epic_app_id": app_id if source == "epic" else None,
                    "gog_game_id": app_id if source == "gog" else None,
                    "xbox_game_id": app_id if source == "xbox" else None,
                    "last_session_at": None,
                    "last_verdict": None,
                    "source": source,
                },
            )

        for session in sessions:
            if not session.get("game_name"):
                continue
            game_id = self._game_id(session["game_name"])
            entry = catalog.setdefault(
                game_id,
                {
                    "game_id": game_id,
                    "game_name": session["game_name"],
                    "executable_path": session.get("executable_path"),
                    "last_session_at": session.get("started_at"),
                    "last_verdict": None,
                },
            )
            if session.get("executable_path"):
                entry["executable_path"] = session.get("executable_path")
            entry["last_session_at"] = session.get("started_at")

        for benchmark in benchmarks:
            game_id = self._game_id(benchmark["workload_name"])
            entry = catalog.setdefault(
                game_id,
                {
                    "game_id": game_id,
                    "game_name": benchmark["workload_name"],
                    "executable_path": None,
                    "last_session_at": None,
                    "last_verdict": None,
                },
            )
            entry["last_verdict"] = benchmark.get("verdict")

        for game_id, profile in profiles.items():
            entry = catalog.setdefault(
                game_id,
                {
                    "game_id": game_id,
                    "game_name": profile.get("game_name", game_id),
                    "executable_path": profile.get("executable_path"),
                    "last_session_at": None,
                    "last_verdict": profile.get("last_verdict"),
                },
            )
            entry["has_profile"] = True
            entry["updated_at"] = profile.get("updated_at")

        return sorted(
            catalog.values(),
            key=lambda item: item.get("last_session_at") or item.get("updated_at") or 0,
            reverse=True,
        )

    async def get_profile(self, game_id: str) -> dict[str, Any]:
        existing = await self.database.get_game_profile(game_id)
        if existing:
            return existing

        sessions = [item for item in await self.database.list_sessions(limit=200) if self._game_id(item.get("game_name") or "") == game_id]
        benchmarks = [item for item in await self.database.list_benchmark_results(limit=200) if self._game_id(item.get("workload_name") or "") == game_id]
        discovered_match = self._discovered_game_for_id(game_id)
        game_name = (
            sessions[0]["game_name"]
            if sessions
            else (benchmarks[0]["workload_name"] if benchmarks else (discovered_match.get("name") if discovered_match else game_id))
        )
        executable_path = (
            sessions[0].get("executable_path")
            if sessions
            else (
                discovered_match.get("executable_path") or discovered_match.get("install_dir")
                if discovered_match
                else None
            )
        )
        recommended_tweaks = [item["id"] for item in self.optimizer.catalog()[:3]]
        source = self._canonical_source(discovered_match.get("source")) if discovered_match else None
        app_id = discovered_match.get("app_id") if discovered_match else None
        return {
            "game_id": game_id,
            "game_name": game_name,
            "executable_path": executable_path,
            "recommended_tweaks": recommended_tweaks,
            "notes": (
                "Generated from current safe tweak catalog, launcher/library discovery, and observed session/benchmark history."
            ),
            "history": {
                "session_count": len(sessions),
                "benchmark_count": len(benchmarks),
                "last_verdict": benchmarks[0]["verdict"] if benchmarks else None,
                "catalog_discovered": bool(discovered_match),
                "discovery_source": source,
            },
            "recommendation_basis": {
                "sessions": len(sessions),
                "benchmarks": len(benchmarks),
                "latest_benchmark_ids": [item["benchmark_id"] for item in benchmarks[:3]],
                "app_id": app_id,
                "steam_app_id": app_id if source == "steam" else None,
                "discovery_source": source,
            },
        }

    async def save_profile(self, game_id: str, payload: dict[str, Any]) -> dict[str, Any]:
        profile = dict(payload)
        profile.setdefault("game_id", game_id)
        profile.setdefault("recommended_tweaks", [])
        profile.setdefault("history", {})
        await self.database.upsert_game_profile(
            game_id,
            profile.get("game_name", game_id),
            profile.get("executable_path"),
            profile,
        )
        return await self.get_profile(game_id)

    async def export_profile(self, game_id: str) -> str:
        profile = await self.get_profile(game_id)
        return json.dumps({"format": "pbprofile.v1", **profile}, indent=2)

    async def import_profile(self, payload: dict[str, Any]) -> dict[str, Any]:
        game_id = payload.get("game_id") or self._game_id(payload.get("game_name") or "imported-game")
        return await self.save_profile(game_id, payload)

    def _game_id(self, name: str) -> str:
        return name.strip().lower().replace(" ", "-") or "unknown-game"

    def _list_steam_games(self) -> list[dict[str, Any]]:
        if not self.steam_scanner:
            return []
        try:
            games = self.steam_scanner.list_installed_games()
        except Exception:
            return []
        normalized: list[dict[str, Any]] = []
        for item in games:
            if not isinstance(item, dict):
                continue
            payload = dict(item)
            payload["source"] = "steam"
            if payload.get("app_id") is None and payload.get("steam_app_id") is not None:
                payload["app_id"] = payload.get("steam_app_id")
            normalized.append(payload)
        return normalized

    def _list_discovered_games(self) -> list[dict[str, Any]]:
        if self.installed_game_scanner:
            try:
                games = self.installed_game_scanner.list_installed_games()
            except Exception:
                games = []
            return [item for item in games if isinstance(item, dict)]
        return self._list_steam_games()

    def _discovered_game_for_id(self, game_id: str) -> dict[str, Any] | None:
        for item in self._list_discovered_games():
            name = str(item.get("name") or "")
            if self._game_id(name) == game_id:
                return item
        return None

    @staticmethod
    def _canonical_source(raw: Any) -> str:
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
