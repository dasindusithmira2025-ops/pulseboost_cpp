from __future__ import annotations

import time
import uuid
from typing import Any

from core.audit_log import AuditLog
from core.database import DatabaseService
from core.game_detection import GameDetector
from core.models import SessionRecord


class SessionManager:
    def __init__(self, database: DatabaseService, audit_log: AuditLog, game_detector: GameDetector) -> None:
        self.database = database
        self.audit_log = audit_log
        self.game_detector = game_detector
        self.current_session: dict[str, Any] | None = None

    async def update(self, snapshot, session_mode: str, health_score: float) -> dict[str, Any] | None:
        candidate = self.game_detector.detect(snapshot, session_mode)
        if candidate and not self.current_session:
            await self._start_session(candidate, snapshot)
        elif candidate and self.current_session and candidate["name"] != self.current_session.get("game_name"):
            await self.finalize_active_session(snapshot, stability_score=health_score, clean_exit=True)
            await self._start_session(candidate, snapshot)
        elif not candidate and self.current_session:
            await self.finalize_active_session(snapshot, stability_score=health_score, clean_exit=True)
        return self.current_session

    async def finalize_active_session(self, snapshot, *, stability_score: float | None, clean_exit: bool) -> None:
        if not self.current_session:
            return
        final_metrics = snapshot.to_dict() if snapshot and hasattr(snapshot, "to_dict") else {}
        session_id = self.current_session["id"]
        await self.database.complete_session(
            session_id,
            final_metrics_snapshot=final_metrics,
            stability_score=stability_score,
            clean_exit=clean_exit,
        )
        await self.audit_log.record_event(
            module="session_manager",
            action="end_session",
            target=self.current_session.get("game_name") or "unknown",
            before_value={"session_id": session_id},
            after_value={"clean_exit": clean_exit, "stability_score": stability_score},
            rationale="Closed the active game session record.",
            validity_tag="VALIDATED",
            triggered_by="session_engine",
            status="success",
        )
        self.current_session = None

    async def recent_sessions(self, limit: int = 10) -> list[dict[str, Any]]:
        return await self.database.list_sessions(limit=limit)

    async def _start_session(self, candidate: dict[str, Any], snapshot) -> None:
        record = SessionRecord(
            id=str(uuid.uuid4()),
            game_name=candidate.get("name"),
            executable_path=candidate.get("executable_path"),
            started_at=snapshot.timestamp,
            baseline_metrics_snapshot=snapshot.to_dict() if hasattr(snapshot, "to_dict") else {},
            clean_exit=False,
        )
        await self.database.insert_session(record)
        self.current_session = record.to_dict()
        await self.audit_log.record_event(
            module="session_manager",
            action="start_session",
            target=record.game_name or "unknown",
            before_value=None,
            after_value={"session_id": record.id, "started_at": record.started_at},
            rationale="Started a new detected game session.",
            validity_tag="VALIDATED",
            triggered_by="session_engine",
            status="success",
        )
