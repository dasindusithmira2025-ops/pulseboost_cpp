from __future__ import annotations

import time
import uuid
from typing import Any

from core.audit_log import AuditLog
from core.database import DatabaseService
from core.models import RevertSnapshot


class RevertManager:
    def __init__(self, database: DatabaseService, audit_log: AuditLog | None = None) -> None:
        self.database = database
        self.audit_log = audit_log
        self.session_id: str | None = None

    def set_session_id(self, session_id: str) -> None:
        self.session_id = session_id

    async def capture_snapshot(
        self,
        *,
        target_type: str,
        target_id: str,
        before_value: Any,
        metadata: dict[str, Any] | None = None,
        rationale: str = "Saved pre-change state for safe rollback.",
    ) -> str:
        snapshot = RevertSnapshot(
            snapshot_id=str(uuid.uuid4()),
            target_type=target_type,
            target_id=target_id,
            before_value=before_value,
            created_at=time.time(),
            session_id=self.session_id,
            metadata=metadata or {},
        )
        await self.database.insert_revert_snapshot(snapshot)
        if self.audit_log:
            await self.audit_log.record_event(
                module="revert_manager",
                action="capture_snapshot",
                target=f"{target_type}:{target_id}",
                before_value=before_value,
                after_value={"snapshot_id": snapshot.snapshot_id},
                rationale=rationale,
                validity_tag="VALIDATED",
                triggered_by="system",
                status="success",
            )
        return snapshot.snapshot_id

    async def mark_restored(self, snapshot_id: str, *, status: str = "restored") -> None:
        snapshot = await self.database.get_revert_snapshot(snapshot_id)
        await self.database.mark_revert_snapshot_restored(snapshot_id, status=status)
        if self.audit_log and snapshot:
            await self.audit_log.record_event(
                module="revert_manager",
                action="restore_snapshot",
                target=f"{snapshot['target_type']}:{snapshot['target_id']}",
                before_value={"snapshot_id": snapshot_id},
                after_value={"status": status},
                rationale="Marked revert snapshot as restored.",
                validity_tag="VALIDATED",
                triggered_by="system",
                status="success",
            )
