"""
PulseBoost orchestrator.
"""
from __future__ import annotations

import asyncio
import time
from dataclasses import asdict
from typing import Any

from config import get_settings
from core.agents.executor import Executor
from core.cognition.anomaly import AnomalyDetector
from core.cognition.efficiency_scorer import EfficiencyScorer
from core.cognition.learning import LearningEngine
from core.cognition.memory import MemorySystem
from core.cognition.plans import features_for_plan, normalize_plan, plan_at_least
from core.cognition.planner import Planner
from core.cognition.predictor import Predictor
from core.cognition.reasoner import Reasoner
from core.cognition.scheduler import IntelligentScheduler
from core.cognition.scorer import HealthScorer
from core.cognition.suggestions import generate_smart_suggestions
from core.cognition.thermal import ThermalIntelligence
from core.cognition.waste_hunter import WasteHunter
from core.entitlements import build_entitlement_snapshot, entitlement_access_map, legacy_plan_features
from core.performance_diagnostics import BottleneckAnalyzer, FrameTimeCapture
from core.tools.collector import MetricCollector
from core.tools.session_detector import SessionDetector


BASELINE_METRICS = (
    "cpu_total",
    "ram_percent",
    "disk_percent",
    "disk_read_bytes",
    "disk_write_bytes",
    "net_recv_bytes",
    "net_sent_bytes",
)


class Orchestrator:
    def __init__(self, broadcast_fn):
        self.settings = get_settings()
        self.broadcast = broadcast_fn
        self.memory = MemorySystem(self.settings.resolved_db_path, self.settings.resolved_vector_path)
        self.collector = MetricCollector()
        self.scorer = HealthScorer()
        self.efficiency_scorer = EfficiencyScorer()
        self.thermal = ThermalIntelligence()
        self.session_detector = SessionDetector()
        self.frame_time_capture = FrameTimeCapture(self.settings.presentmon_csv_path)
        self.bottleneck_analyzer = BottleneckAnalyzer()
        self.anomaly_detector = AnomalyDetector(self.memory, minimum_samples=self.settings.baseline_min_samples)
        self.waste_hunter = WasteHunter(self.memory)
        self.scheduler = IntelligentScheduler(self.memory)
        self.planner = Planner(self.memory)
        self.reasoner = Reasoner(self.memory)
        self.predictor = Predictor()
        self.learning = LearningEngine(self.memory)
        self.executor = Executor(self.memory)
        self.session_manager = None
        self.account_service = None
        self.database = None
        self.gpu_controller = None
        self.running = False
        self.cycle_count = 0
        self.last_snapshot = None
        self._last_health_history_snapshot_at: float | None = None
        self.current_state: dict[str, Any] = {
            "health_score": 100.0,
            "metrics": None,
            "breakdown": {},
            "session_mode": "normal",
            "active_session": None,
            "recent_sessions": [],
            "anomalies": [],
            "predictions": [],
            "optimizations": [],
            "process_intelligence": [],
            "efficiency_score": 100.0,
            "efficiency_grade": "A",
            "efficiency_breakdown": {},
            "pillar_scores": {},
            "scheduler": {"is_idle": False, "deferral_candidates": [], "optimal_maintenance_window": "23:00", "queued_tasks": []},
            "thermal": {},
            "timeline": [],
            "actions": [],
            "alerts": [],
            "suggested_actions": [],
            "smart_suggestions": [],
            "plan": normalize_plan(self.settings.default_plan),
            "features": asdict(features_for_plan(self.settings.default_plan)),
            "plan_info": {"plan_tier": normalize_plan(self.settings.default_plan), "status": "active"},
            "feature_access": entitlement_access_map(build_entitlement_snapshot(account_id=None, plan_tier=self.settings.default_plan, source="local-default-config")),
            "entitlement_snapshot": build_entitlement_snapshot(account_id=None, plan_tier=self.settings.default_plan, source="local-default-config").to_dict(),
            "auth": {"signed_in": False, "identity": None},
            "baseline_status": {"ready": False, "progress_percent": 0.0},
            "adaptive": {"enabled": False, "recent_actions": [], "notifications": [], "executed_actions": []},
        }

    async def initialize(self) -> None:
        await self.memory.initialize()

    async def run(self) -> None:
        self.running = True
        while self.running:
            cycle_started = time.time()
            try:
                await self._cycle()
            except Exception as exc:  # pragma: no cover
                await self.broadcast({"type": "system_error", "message": str(exc), "timestamp": time.time()})
            elapsed = time.time() - cycle_started
            await asyncio.sleep(max(0.1, self.settings.poll_interval_seconds - elapsed))

    async def stop(self) -> None:
        self.running = False
        if self.session_manager and self.last_snapshot is not None:
            await self.session_manager.finalize_active_session(self.last_snapshot, stability_score=self.current_state.get("health_score"), clean_exit=True)

    async def _cycle(self) -> None:
        self.cycle_count += 1
        snapshot = self.collector.get_snapshot()
        self.last_snapshot = snapshot
        self.memory.push_snapshot(snapshot)

        hour = time.localtime(snapshot.timestamp).tm_hour
        session = self.session_detector.detect(snapshot, hour)
        snapshot.session_mode = session.session_mode
        gpu_stats = await self._gpu_stats()
        if gpu_stats:
            snapshot.gpu_percent = gpu_stats.get("utilization_percent")
            snapshot.gpu_temp = gpu_stats.get("temperature_c")
        frame_time = self.frame_time_capture.capture(
            foreground_app=snapshot.foreground_app,
            session_mode=session.session_mode,
        )
        bottleneck = self.bottleneck_analyzer.analyze(
            snapshot=snapshot,
            session_mode=session.session_mode,
            frame_time=frame_time,
            gpu_stats=gpu_stats,
        )
        snapshot.frametime = frame_time
        snapshot.current_bottleneck = bottleneck["current_bottleneck"]
        snapshot.bottleneck_details = bottleneck

        await self._update_baselines(snapshot, session.session_mode)
        anomalies = await self.anomaly_detector.analyze(snapshot, session.session_mode)
        score, breakdown = self.scorer.calculate(snapshot, anomalies)
        snapshot.health_score = score
        await self._persist_health_history(snapshot, score)
        if self.session_manager:
            await self.session_manager.update(snapshot, session.session_mode, score)
        optimization_state = self.waste_hunter.hunt(snapshot, session.session_mode)
        scheduler_state = self.scheduler.evaluate(snapshot)
        thermal_state = self.thermal.analyze(snapshot, session.session_mode)
        efficiency = self.efficiency_scorer.calculate(snapshot, optimization_state["optimizations"])

        access_context = await self._access_context()
        plan_name = access_context["plan_tier"]
        features = access_context["legacy_features"]
        predictions = self.predictor.forecast(self.memory.recent(limit=300)) if features["predictions"] else []

        await self.memory.store_snapshot(snapshot)
        await self.memory.store_anomalies(anomalies)
        await self.memory.store_process_intelligence(optimization_state["process_intelligence"], snapshot.timestamp)
        await self.memory.store_efficiency_snapshot(snapshot.timestamp, efficiency)
        await self.memory.store_scheduled_tasks(scheduler_state["queued_tasks"])

        if self.cycle_count % max(1, self.settings.history_rollup_interval_cycles) == 0:
            await self.memory.enforce_retention(
                keep_3s_hours=self.settings.history_window_hours_3s,
                keep_1m_days=self.settings.history_window_days_1m,
                keep_5m_days=self.settings.history_window_days_5m,
            )

        suggested_actions = await self._generate_action_suggestions(anomalies, snapshot, score, auto_execute=self.settings.enable_auto_heal and plan_at_least(plan_name, "pro"))
        optimization_feed = await self._process_optimizations(
            optimization_state["optimizations"],
            snapshot,
            efficiency,
            auto_execute=self.settings.enable_auto_heal and plan_at_least(plan_name, "pro"),
        )
        adaptive_state = await self._adaptive_cycle(snapshot, session.session_mode)
        smart_suggestions = generate_smart_suggestions(
            metrics=snapshot.to_dict(),
            session_mode=session.session_mode,
            thermal_state=thermal_state,
            active_session=self.session_manager.current_session if self.session_manager else None,
        )
        alerts = self._build_alerts(anomalies, predictions, suggested_actions, optimization_feed, adaptive_state)
        baseline_status = await self._baseline_status(session.session_mode)
        timeline_window = self._history_window_seconds(plan_name)
        timeline = await self.memory.timeline(limit=400, window_seconds=timeline_window)
        actions = await self.memory.recent_actions(limit=50) if features["audit_log"] else []
        persisted_optimizations = await self.memory.recent_optimizations(limit=20)
        similar_events = self.memory.find_similar_events(
            f"cpu_{snapshot.cpu_total:.1f} ram_{snapshot.ram_percent:.1f} session_{session.session_mode} anomalies_{','.join(item['metric'] for item in anomalies)}",
            limit=3,
        )
        recent_sessions = await self.session_manager.recent_sessions(limit=10) if self.session_manager else []

        self.current_state = {
            "health_score": score,
            "metrics": snapshot.to_dict(),
            "breakdown": breakdown,
            "efficiency_score": efficiency["score"],
            "efficiency_grade": efficiency["grade"],
            "efficiency_breakdown": efficiency["breakdown"],
            "session_mode": session.session_mode,
            "session_confidence": session.confidence,
            "session_evidence": session.evidence,
            "active_session": self.session_manager.current_session if self.session_manager else None,
            "recent_sessions": recent_sessions,
            "anomalies": anomalies,
            "predictions": predictions,
            "optimizations": optimization_feed or persisted_optimizations,
            "process_intelligence": optimization_state["process_intelligence"],
            "pillar_scores": optimization_state["pillar_scores"],
            "scheduler": scheduler_state,
            "thermal": thermal_state,
            "timeline": timeline,
            "actions": actions,
            "alerts": alerts,
            "suggested_actions": suggested_actions,
            "smart_suggestions": smart_suggestions,
            "similar_events": similar_events,
            "plan": plan_name,
            "features": features,
            "plan_info": access_context["plan_info"],
            "feature_access": access_context["feature_access"],
            "entitlement_snapshot": access_context["entitlement_snapshot"],
            "auth": access_context["auth_status"],
            "baseline_status": baseline_status,
            "adaptive": adaptive_state,
            "current_bottleneck": bottleneck["current_bottleneck"],
            "bottleneck_details": bottleneck,
            "frametime": frame_time,
            "history_window_seconds": timeline_window,
            "machine": {
                "id": self.settings.machine_id,
                "name": self.settings.machine_name,
            },
            "executor": {
                "dry_run": self.settings.executor_dry_run,
                "auto_heal_enabled": self.settings.enable_auto_heal and plan_at_least(plan_name, "pro"),
                "mode": self.settings.auto_heal_mode,
            },
        }

        await self.broadcast({"type": "full_state", "timestamp": snapshot.timestamp, **self.current_state})

    async def decide_optimization(self, optimization_id: str, decision: str) -> dict[str, Any]:
        optimization = await self.memory.get_optimization(optimization_id)
        if not optimization:
            return {"success": False, "error": "Optimization not found."}

        if decision == "dismiss":
            await self.memory.set_optimization_status(optimization_id, status="dismissed", user_approved=False)
            self.current_state["optimizations"] = [
                {**item, "status": "dismissed"} if item.get("id") == optimization_id else item
                for item in self.current_state.get("optimizations", [])
                if item.get("id") != optimization_id
            ]
            await self.broadcast({"type": "full_state", "timestamp": time.time(), **self.current_state})
            return {"success": True, "status": "dismissed"}

        thought = await self.reasoner.evaluate(
            {"action": optimization["action"], "params": optimization.get("params", {}), "risk_level": optimization.get("risk", "SAFE")},
            self.last_snapshot or self._snapshot_proxy(self.current_state.get("metrics") or {}),
            confirmed=True,
        )
        if not thought.get("proceed"):
            await self.memory.set_optimization_status(optimization_id, status="pending", user_approved=True)
            return {"success": False, "blocked": True, "thought": thought}

        result = await self.executor.run(
            thought,
            health_before=self.current_state.get("health_score", 100.0),
            trigger_reason="optimization_approved",
        )
        status = "done" if result.get("success") else "failed"
        await self.memory.set_optimization_status(
            optimization_id,
            status=status,
            user_approved=True,
            actual_gain=optimization.get("estimated_gain") if result.get("success") else None,
            efficiency_after=self.current_state.get("efficiency_score"),
        )
        self.current_state["optimizations"] = [
            {**item, "status": status, "user_approved": True} if item.get("id") == optimization_id else item
            for item in self.current_state.get("optimizations", [])
        ]
        await self.broadcast({"type": "full_state", "timestamp": time.time(), **self.current_state})
        return {"success": bool(result.get("success")), "status": status, "result": result, "thought": thought}

    async def preview_action(self, action_type: str, params: dict[str, Any]) -> dict[str, Any]:
        step = {
            "action": action_type,
            "params": params,
            "risk_level": self.planner.risk_for_action(action_type),
        }
        thought = await self.reasoner.evaluate(step, self.last_snapshot or self._snapshot_proxy(self.current_state.get("metrics") or {}))
        thought["requires_confirmation"] = not thought.get("proceed", False)
        return thought

    async def execute_action(
        self,
        action_type: str,
        params: dict[str, Any],
        *,
        confirmed: bool,
        trigger_reason: str = "manual_request",
    ) -> dict[str, Any]:
        step = {
            "action": action_type,
            "params": params,
            "risk_level": self.planner.risk_for_action(action_type),
        }
        snapshot = self.last_snapshot or self._snapshot_proxy(self.current_state.get("metrics") or {})
        thought = await self.reasoner.evaluate(step, snapshot, confirmed=confirmed)
        if not thought.get("proceed"):
            return {"success": False, "blocked": True, "thought": thought}

        result = await self.executor.run(thought, health_before=self.current_state.get("health_score", 100.0), trigger_reason=trigger_reason)
        if result.get("success"):
            for anomaly in self.current_state.get("anomalies", []):
                await self.memory.mark_anomaly_resolved(anomaly["timestamp"], anomaly["metric"], {"action": action_type, "params": params})
        return {"success": result.get("success", False), "thought": thought, "result": result}

    async def state_for_plan(self, plan_name: str | None = None) -> dict[str, Any]:
        state = dict(self.current_state)
        if plan_name is None and self.account_service is not None:
            access_context = await self._access_context()
            state["plan"] = access_context["plan_tier"]
            state["features"] = access_context["legacy_features"]
            state["plan_info"] = access_context["plan_info"]
            state["feature_access"] = access_context["feature_access"]
            state["entitlement_snapshot"] = access_context["entitlement_snapshot"]
            state["auth"] = access_context["auth_status"]
            features = access_context["legacy_features"]
            resolved_plan = access_context["plan_tier"]
        else:
            resolved_plan = normalize_plan(plan_name or self.settings.default_plan)
            features = asdict(features_for_plan(resolved_plan))
            state["plan"] = resolved_plan
            state["features"] = features
            state["plan_info"] = {"plan_tier": resolved_plan, "status": "active"}
            state["feature_access"] = entitlement_access_map(build_entitlement_snapshot(account_id=None, plan_tier=resolved_plan, source="local-default-config"))
            state["entitlement_snapshot"] = build_entitlement_snapshot(account_id=None, plan_tier=resolved_plan, source="local-default-config").to_dict()
            state["auth"] = {"signed_in": False, "identity": None}
        if not features["predictions"]:
            state["predictions"] = []
        if not features["audit_log"]:
            state["actions"] = []
        state["history_window_seconds"] = self._history_window_seconds(resolved_plan)
        return state

    async def _generate_action_suggestions(self, anomalies: list[dict], snapshot, health_score: float, *, auto_execute: bool) -> list[dict]:
        if not anomalies:
            return []
        plan = await self.planner.generate(anomalies, snapshot, health_score)
        suggestions = []
        for step in plan.get("steps", []):
            thought = await self.reasoner.evaluate(step, snapshot)
            entry = {"step": step, "thought": thought, "result": None}
            if thought.get("proceed") and auto_execute:
                before = health_score
                result = await self.executor.run(thought, health_before=before, trigger_reason="auto_heal")
                after = before + (8 if result.get("success") else -4)
                context_hash = self.learning.build_context_hash(snapshot, anomalies, snapshot.session_mode)
                delta = await self.learning.record_outcome(step["action"], context_hash, before, after)
                if result.get("success"):
                    self.memory.store_event_vector(
                        f"{step['action']} helped resolve {anomalies[0]['metric']} with delta {delta}",
                        delta,
                    )
                    for anomaly in anomalies:
                        await self.memory.mark_anomaly_resolved(anomaly["timestamp"], anomaly["metric"], {"action": step["action"], "params": step.get("params", {})})
                entry["result"] = result
                entry["health_delta"] = delta
            suggestions.append(entry)
        return suggestions

    async def _baseline_status(self, session_mode: str) -> dict[str, Any]:
        overview = await self.memory.baseline_overview(session_mode)
        if not overview:
            return {
                "ready": False,
                "progress_percent": 0.0,
                "minimum_samples": self.settings.baseline_min_samples,
                "estimated_hours_remaining": 48.0,
                "by_metric": {},
            }
        progress_values = [
            min(100.0, (item["max_sample_count"] / max(1, self.settings.baseline_min_samples)) * 100)
            for item in overview.values()
        ]
        progress = round(sum(progress_values) / len(progress_values), 1)
        return {
            "ready": progress >= 100.0,
            "progress_percent": progress,
            "minimum_samples": self.settings.baseline_min_samples,
            "estimated_hours_remaining": round(max(0.0, 48.0 * (1 - (progress / 100))), 1),
            "by_metric": overview,
        }

    async def _update_baselines(self, snapshot, session_mode: str) -> None:
        hour = time.localtime(snapshot.timestamp).tm_hour
        for metric in BASELINE_METRICS:
            value = float(getattr(snapshot, metric))
            await self.memory.update_baseline(metric, hour, session_mode, value, snapshot.timestamp)

    async def _process_optimizations(self, optimizations: list[dict], snapshot, efficiency: dict[str, Any], *, auto_execute: bool) -> list[dict]:
        if not optimizations:
            return []
        feed = []
        for item in optimizations[:8]:
            entry = dict(item)
            existing = await self.memory.get_optimization(entry["id"])
            if existing and existing.get("status") == "dismissed":
                continue
            if auto_execute and item.get("auto_execute") and item.get("risk") == "SAFE":
                thought = await self.reasoner.evaluate(
                    {"action": item["action"], "params": item.get("params", {}), "risk_level": item.get("risk", "SAFE")},
                    snapshot,
                    confirmed=True,
                )
                if thought.get("proceed"):
                    result = await self.executor.run(
                        thought,
                        health_before=self.current_state.get("health_score", 100.0),
                        trigger_reason="optimization_hunter",
                    )
                    entry["status"] = "done" if result.get("success") else "failed"
                    entry["actual_gain"] = item.get("estimated_gain") if result.get("success") else None
                else:
                    entry["status"] = "pending"
            elif existing and existing.get("status") in {"done", "failed"}:
                entry["status"] = existing["status"]
                entry["actual_gain"] = existing.get("actual_gain")
            else:
                entry["status"] = entry.get("status", "pending")
            feed.append(entry)
        await self.memory.store_optimizations(feed, efficiency_before=efficiency["score"], efficiency_after=efficiency["score"])
        return feed

    def _build_alerts(self, anomalies: list[dict], predictions: list[dict], suggestions: list[dict], optimizations: list[dict], adaptive_state: dict[str, Any]) -> list[dict]:
        alerts = []
        for optimization in optimizations[:4]:
            alerts.append(
                {
                    "severity": "HIGH" if optimization.get("priority") in {"immediate", "high"} else "LOW",
                    "message": optimization["user_message"],
                    "timestamp": optimization.get("timestamp", time.time()),
                    "action": "Optimize" if not optimization.get("auto_execute") else "Review",
                    "metric": optimization["pillar"],
                }
            )
        for anomaly in anomalies[:5]:
            alerts.append(
                {
                    "severity": anomaly["severity"],
                    "message": f"{anomaly['metric']} is outside the learned baseline.",
                    "timestamp": anomaly["timestamp"],
                    "action": "Fix Now" if anomaly["severity"] in {"HIGH", "CRITICAL"} else "Review",
                    "metric": anomaly["metric"],
                }
            )
        for prediction in predictions[:3]:
            alerts.append(
                {
                    "severity": "MEDIUM",
                    "message": f"{prediction['metric']} may hit {prediction['threshold']}% in {prediction['time_to_threshold_human']}.",
                    "timestamp": time.time(),
                    "action": "Investigate",
                    "metric": prediction["metric"],
                }
            )
        for suggestion in suggestions[:2]:
            thought = suggestion["thought"]
            if thought.get("abort_reason"):
                alerts.append(
                    {
                        "severity": "LOW" if thought.get("confidence", 0) < 0.5 else "MEDIUM",
                        "message": f"{suggestion['step']['action']} is available but waiting on {thought['abort_reason'].lower()}.",
                        "timestamp": time.time(),
                        "action": "Review",
                        "metric": suggestion["step"]["action"],
                    }
                )
        for notification in adaptive_state.get("notifications", [])[:2]:
            alerts.append(
                {
                    "severity": "LOW",
                    "message": notification,
                    "timestamp": time.time(),
                    "action": "Review",
                    "metric": "adaptive_engine",
                }
            )
        return alerts

    async def _adaptive_cycle(self, snapshot, session_mode: str) -> dict[str, Any]:
        if not self.adaptive_engine:
            return {"enabled": False, "recent_actions": [], "notifications": [], "executed_actions": []}
        state = await self.adaptive_engine.evaluate(
            snapshot,
            session_mode,
            active_session_id=(self.session_manager.current_session or {}).get("id") if self.session_manager else None,
        )
        if self.session_manager and self.session_manager.current_session:
            for action in state.get("executed_actions", []):
                await self.session_manager.database.record_session_action(
                    self.session_manager.current_session["id"],
                    "adaptive_action",
                    action,
                )
        return state

    async def _persist_health_history(self, snapshot, score: float) -> None:
        if self.database is None:
            return
        now_ts = float(getattr(snapshot, "timestamp", time.time()))
        if self._last_health_history_snapshot_at is not None and (now_ts - self._last_health_history_snapshot_at) < 600:
            return
        await self.database.insert_health_history(
            score=score,
            cpu_load=float(getattr(snapshot, "cpu_total", 0.0)),
            ram_percent=float(getattr(snapshot, "ram_percent", 0.0)),
            gpu_temp=float(getattr(snapshot, "temperature")) if getattr(snapshot, "temperature", None) is not None else None,
            timestamp=now_ts,
        )
        self._last_health_history_snapshot_at = now_ts

    async def _gpu_stats(self) -> dict[str, Any]:
        if self.gpu_controller is None:
            return {}
        try:
            return await self.gpu_controller.stats()
        except Exception:
            return {}

    def _history_window_seconds(self, plan_name: str) -> int | None:
        features = features_for_plan(plan_name)
        if features.history_hours is None:
            return None
        return features.history_hours * 3600

    async def _access_context(self) -> dict[str, Any]:
        if self.account_service is not None:
            return await self.account_service.current_access_context()
        plan_name = normalize_plan(self.settings.default_plan)
        snapshot = build_entitlement_snapshot(account_id=None, plan_tier=plan_name, source="local-default-config")
        return {
            "signed_in": False,
            "identity": None,
            "auth_status": {"signed_in": False, "identity": None},
            "plan_info": {"plan_tier": plan_name, "status": "active"},
            "plan_tier": plan_name,
            "entitlement_snapshot": snapshot.to_dict(),
            "feature_access": entitlement_access_map(snapshot),
            "legacy_features": legacy_plan_features(plan_name),
        }

    def _snapshot_proxy(self, payload: dict[str, Any]):
        class SnapshotProxy:
            pass

        proxy = SnapshotProxy()
        for key, value in payload.items():
            setattr(proxy, key, value)
        if not hasattr(proxy, "top_processes"):
            proxy.top_processes = []
        if not hasattr(proxy, "session_mode"):
            proxy.session_mode = "normal"
        return proxy





