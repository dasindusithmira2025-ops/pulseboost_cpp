from __future__ import annotations

from typing import Any

from core.database import DatabaseService


class TemporaryTweakManager:
    STATE_KEY = "optimizer.active_temporary_tweaks"

    def __init__(self, database: DatabaseService) -> None:
        self.database = database

    async def list_active(self) -> list[dict[str, Any]]:
        return await self.database.list_active_tweak_applications()

    async def register(self, record: dict[str, Any]) -> None:
        snapshot_id = str(record.get("snapshot_id") or "").strip()
        if not snapshot_id:
            return
        existing = await self.list_active()
        if any(item.get("snapshot_id") == snapshot_id for item in existing):
            return
        await self.database.create_tweak_application(
            tweak_id=record.get("tweak_id") or "unknown_tweak",
            session_id=record.get("session_id"),
            triggered_by=record.get("triggered_by") or "optimizer",
            temporary_flag=True,
            apply_result=record.get("apply_result") or "success",
            revert_snapshot_id=snapshot_id,
            target=record.get("target"),
            metadata=record,
            before_value=record.get("before_value"),
            after_value=record.get("after_value"),
            error_text=record.get("error_text"),
        )

    async def remove(self, snapshot_id: str) -> None:
        await self.database.mark_tweak_application_reverted(
            revert_snapshot_id=snapshot_id,
            revert_result="success",
        )

    async def clear(self) -> None:
        await self.database.clear_active_tweak_applications()
