from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Any

from core.audit_log import AuditLog
from core.database import DatabaseService
from core.optimizer import SystemOptimizer


@dataclass(frozen=True)
class AdaptiveRule:
    rule_id: str
    title: str
    tweak_id: str
    cooldown_seconds: int
    session_modes: tuple[str, ...]
    rationale: str


class AdaptiveEngine:
    ENABLED_KEY = "adaptive.enabled"
    COOLDOWN_KEY = "adaptive.cooldowns"

    RULES = (
        AdaptiveRule(
            rule_id="cpu_background_relief",
            title="Throttle Windows Search during CPU pressure",
            tweak_id="search_indexer_priority",
            cooldown_seconds=900,
            session_modes=("gaming", "heavy_compute", "development"),
            rationale="Foreground work is competing with Windows Search, so PulseBoost lowers indexing priority temporarily.",
        ),
        AdaptiveRule(
            rule_id="disk_indexing_relief",
            title="Pause Windows Search during disk pressure",
            tweak_id="wsearch_session_manual",
            cooldown_seconds=1800,
            session_modes=("gaming", "heavy_compute", "development"),
            rationale="Sustained disk pressure is better handled by pausing indexing for the session and restoring it later.",
        ),
        AdaptiveRule(
            rule_id="disk_prefetch_relief",
            title="Move SysMain to manual under heavy churn",
            tweak_id="sysmain_session_manual",
            cooldown_seconds=1800,
            session_modes=("gaming", "heavy_compute"),
            rationale="Heavy session disk churn makes SysMain a poor background tradeoff, so PulseBoost defers it temporarily.",
        ),
    )

    def __init__(self, *, database: DatabaseService, audit_log: AuditLog, optimizer: SystemOptimizer) -> None:
        self.database = database
        self.audit_log = audit_log
        self.optimizer = optimizer

    async def initialize(self) -> None:
        existing = await self.database.get_setting(self.ENABLED_KEY)
        if existing is None:
            await self.database.set_setting(self.ENABLED_KEY, {"enabled": True}, source="adaptive_engine")
        cooldowns = await self.database.get_setting(self.COOLDOWN_KEY)
        if cooldowns is None:
            await self.database.set_setting(self.COOLDOWN_KEY, {"rules": {}}, source="adaptive_engine")

    async def is_enabled(self) -> bool:
        payload = await self.database.get_setting(self.ENABLED_KEY)
        if payload is None:
            await self.database.set_setting(self.ENABLED_KEY, {"enabled": True}, source="adaptive_engine")
            return True
        return bool(payload.get("enabled", True))

    async def set_enabled(self, enabled: bool, *, triggered_by: str = "user") -> dict[str, Any]:
        before = await self.is_enabled()
        await self.database.set_setting(self.ENABLED_KEY, {"enabled": enabled}, source="adaptive_engine")
        await self.audit_log.record_event(
            module="adaptive_engine",
            action="toggle",
            target="adaptive_engine",
            before_value={"enabled": before},
            after_value={"enabled": enabled},
            rationale="Updated the local adaptive optimization engine state.",
            validity_tag="VALIDATED",
            triggered_by=triggered_by,
            status="success",
        )
        return await self.status()

    async def status(self, limit: int = 10) -> dict[str, Any]:
        cooldown_payload = await self.database.get_setting(self.COOLDOWN_KEY) or {"rules": {}}
        return {
            "enabled": await self.is_enabled(),
            "recent_actions": await self.database.list_adaptive_actions(limit=limit),
            "cooldowns": cooldown_payload.get("rules", {}),
        }

    async def evaluate(
        self,
        snapshot,
        session_mode: str,
        *,
        active_session_id: str | None,
    ) -> dict[str, Any]:
        status = await self.status(limit=8)
        if not status["enabled"]:
            status["notifications"] = []
            status["executed_actions"] = []
            return status

        active_tweaks = {item.get("tweak_id") for item in await self.optimizer.temporary_tweaks.list_active()}
        executed_actions: list[dict[str, Any]] = []
        notifications: list[str] = []

        for rule in self.RULES:
            if rule.tweak_id in active_tweaks:
                continue
            if not self._matches(rule, snapshot, session_mode):
                continue
            if not await self._cooldown_ready(rule.rule_id):
                continue

            result = await self.optimizer.apply_tweak(rule.tweak_id, snapshot=snapshot, triggered_by="adaptive_engine")
            record = {
                "timestamp": time.time(),
                "rule_id": rule.rule_id,
                "title": rule.title,
                "tweak_id": rule.tweak_id,
                "rationale": rule.rationale,
                "session_id": active_session_id,
                "success": bool(result.get("success")),
                "result": result,
            }
            await self.database.insert_adaptive_action(record)
            await self._touch_cooldown(rule.rule_id, rule.cooldown_seconds)
            executed_actions.append(record)
            notifications.append(
                f"Adaptive Engine {'applied' if result.get('success') else 'attempted'} {rule.tweak_id}: {rule.rationale}"
            )
            break

        refreshed = await self.status(limit=8)
        refreshed["notifications"] = notifications
        refreshed["executed_actions"] = executed_actions
        return refreshed

    def _matches(self, rule: AdaptiveRule, snapshot, session_mode: str) -> bool:
        if session_mode not in rule.session_modes:
            return False
        top_processes = getattr(snapshot, "top_processes", []) or []
        if rule.rule_id == "cpu_background_relief":
            indexer = next((proc for proc in top_processes if proc.get("name") == "SearchIndexer.exe"), None)
            return bool(indexer and snapshot.cpu_total >= 75 and indexer.get("cpu_percent", 0) >= 5)
        if rule.rule_id == "disk_indexing_relief":
            return float(getattr(snapshot, "disk_read_bytes", 0) + getattr(snapshot, "disk_write_bytes", 0)) >= 20 * 1024 * 1024
        if rule.rule_id == "disk_prefetch_relief":
            return float(getattr(snapshot, "disk_read_bytes", 0) + getattr(snapshot, "disk_write_bytes", 0)) >= 40 * 1024 * 1024
        return False

    async def _cooldown_ready(self, rule_id: str) -> bool:
        payload = await self.database.get_setting(self.COOLDOWN_KEY) or {"rules": {}}
        next_allowed_at = float(payload.get("rules", {}).get(rule_id, 0))
        return time.time() >= next_allowed_at

    async def _touch_cooldown(self, rule_id: str, cooldown_seconds: int) -> None:
        payload = await self.database.get_setting(self.COOLDOWN_KEY) or {"rules": {}}
        rules = dict(payload.get("rules", {}))
        rules[rule_id] = time.time() + cooldown_seconds
        await self.database.set_setting(self.COOLDOWN_KEY, {"rules": rules}, source="adaptive_engine")
