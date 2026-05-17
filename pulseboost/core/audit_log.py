from __future__ import annotations

import time
import uuid
from typing import Any

from core.database import DatabaseService
from core.event_bus import EventBus
from core.models import AuditEntry


class AuditLog:
    def __init__(self, database: DatabaseService, event_bus: EventBus | None = None) -> None:
        self.database = database
        self.event_bus = event_bus
        self.session_id: str | None = None

    def set_session_id(self, session_id: str) -> None:
        self.session_id = session_id

    async def record_event(
        self,
        *,
        module: str,
        action: str,
        target: str,
        before_value: Any = None,
        after_value: Any = None,
        rationale: str,
        validity_tag: str = "VALIDATED",
        triggered_by: str = "system",
        status: str = "success",
    ) -> AuditEntry:
        entry = AuditEntry(
            id=str(uuid.uuid4()),
            timestamp=time.time(),
            module=module,
            action=action,
            target=target,
            before_value=before_value,
            after_value=after_value,
            rationale=rationale,
            validity_tag=validity_tag,
            triggered_by=triggered_by,
            session_id=self.session_id,
            status=status,
        )
        await self.database.append_audit_entry(entry)
        if self.event_bus:
            await self.event_bus.publish("audit.entry.created", {"entry": entry.to_dict()})
        return entry

    async def recent(self, limit: int = 100) -> list[dict[str, Any]]:
        return await self.database.list_audit_entries(limit=limit)
