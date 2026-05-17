from __future__ import annotations

import time
import uuid

from core.audit_log import AuditLog
from core.database import DatabaseService
from core.event_bus import EventBus
from core.models import AppRuntimeState, RecoveryDecision


class SessionRecovery:
    def __init__(
        self,
        database: DatabaseService,
        audit_log: AuditLog | None = None,
        event_bus: EventBus | None = None,
    ) -> None:
        self.database = database
        self.audit_log = audit_log
        self.event_bus = event_bus
        self.current_session_id: str | None = None

    async def begin_session(self, runtime: str) -> RecoveryDecision:
        now = time.time()
        previous = await self.database.get_runtime_state("current")
        session_id = str(uuid.uuid4())
        recovery_required = bool(previous and previous.get("active") and not previous.get("clean_exit"))

        runtime_state = AppRuntimeState(
            session_id=session_id,
            runtime=runtime,
            started_at=now,
            active=True,
            clean_exit=False,
            recovery_required=recovery_required,
            previous_session_id=(previous or {}).get("session_id"),
        )
        decision = RecoveryDecision(
            recovery_required=recovery_required,
            detected_at=now,
            session_id=session_id,
            previous_session_id=(previous or {}).get("session_id"),
            rationale=(
                "Previous session did not record a clean exit. Temporary changes must be reviewed before reuse."
                if recovery_required
                else "No unclean previous session was detected."
            ),
            recommended_actions=["review temporary tweaks", "verify rollback readiness"] if recovery_required else [],
            status="pending" if recovery_required else "clear",
        )

        await self.database.set_runtime_state("current", runtime_state.to_dict())
        await self.database.set_runtime_state("recovery", decision.to_dict())
        await self.database.mark_clean_exit(False)
        self.current_session_id = session_id

        if self.audit_log:
            await self.audit_log.record_event(
                module="session_recovery",
                action="begin_session",
                target=runtime,
                before_value=previous,
                after_value=runtime_state.to_dict(),
                rationale=decision.rationale,
                validity_tag="VALIDATED",
                triggered_by="startup",
                status="warning" if recovery_required else "success",
            )
        if self.event_bus:
            await self.event_bus.publish("session.started", {"state": runtime_state.to_dict(), "recovery": decision.to_dict()})
        return decision

    async def complete_session(self) -> None:
        if not self.current_session_id:
            return
        current = await self.database.get_runtime_state("current")
        if not current:
            return
        current["active"] = False
        current["clean_exit"] = True
        current["ended_at"] = time.time()
        current["recovery_required"] = False
        await self.database.set_runtime_state("current", current)
        await self.database.set_runtime_state("last_clean_exit", current)
        await self.database.set_runtime_state(
            "recovery",
            {
                "recovery_required": False,
                "detected_at": time.time(),
                "session_id": self.current_session_id,
                "previous_session_id": current.get("previous_session_id"),
                "rationale": "Latest session ended cleanly.",
                "recommended_actions": [],
                "status": "clear",
            },
        )
        await self.database.mark_clean_exit(True)
        if self.audit_log:
            await self.audit_log.record_event(
                module="session_recovery",
                action="complete_session",
                target=current["runtime"],
                before_value={"session_id": self.current_session_id},
                after_value=current,
                rationale="Recorded clean shutdown state.",
                validity_tag="VALIDATED",
                triggered_by="shutdown",
                status="success",
            )
        if self.event_bus:
            await self.event_bus.publish("session.completed", {"state": current})

    async def current_recovery_decision(self) -> dict | None:
        return await self.database.get_runtime_state("recovery")
