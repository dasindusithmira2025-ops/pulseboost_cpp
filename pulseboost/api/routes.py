from __future__ import annotations

import time
from typing import Any

from fastapi import APIRouter, HTTPException, Query, Request
from fastapi.responses import PlainTextResponse, StreamingResponse

from api.schemas import (
    ActionRequest,
    AdaptiveToggleRequest,
    BenchmarkRunRequest,
    ExecuteActionRequest,
    ExpertModeRequest,
    GameProfileRequest,
    GpuSettingRequest,
    LocalPlaceholderSignInRequest,
    NetworkQosRequest,
    OptimizationDecisionRequest,
    SettingsDataActionRequest,
    SettingsPreferencesRequest,
    TokenExchangeRequest,
    TweakApplyRequest,
    TweakRevertRequest,
)
from config import get_settings
from core.entitlements import build_entitlement_snapshot, entitlement_access_map
from core.report_generator import ReportGenerator


router = APIRouter(prefix="/api")
settings = get_settings()

DEFAULT_SETTINGS_PREFERENCES: dict[str, Any] = {
    "activeSection": "general",
    "startWithWindows": True,
    "minimizeToTray": True,
    "checkUpdates": True,
    "updateChannel": "stable",
    "sessionTracking": True,
    "autoOptimize": False,
    "sessionSummary": True,
    "telemetryPollingInterval": "1 second",
    "backgroundMonitoring": True,
    "autoSnapshot": True,
    "revertOnFailure": True,
    "adminPrompt": True,
    "dryRunDefault": False,
    "expertMode": False,
    "experimentalTweaks": False,
    "verboseLogging": False,
    "exportAllData": False,
    "importSettings": False,
    "clearBenchmarkHistory": False,
    "resetAllSettings": False,
}


def _response_cache(request: Request) -> dict[str, dict[str, Any]]:
    cache = getattr(request.app.state, "response_cache", None)
    if cache is None:
        cache = {}
        request.app.state.response_cache = cache
    return cache


def _cache_get(request: Request, key: str, ttl_seconds: float) -> Any | None:
    cache = _response_cache(request)
    entry = cache.get(key)
    if not entry:
        return None
    if (time.time() - float(entry.get("cached_at", 0.0))) > ttl_seconds:
        cache.pop(key, None)
        return None
    return entry.get("data")


def _cache_set(request: Request, key: str, data: Any) -> Any:
    cache = _response_cache(request)
    cache[key] = {"data": data, "cached_at": time.time()}
    return data


def _cache_invalidate(request: Request, prefix: str) -> None:
    cache = _response_cache(request)
    stale_keys = [key for key in cache.keys() if key.startswith(prefix)]
    for key in stale_keys:
        cache.pop(key, None)


def merged_settings_preferences(saved: dict[str, Any] | None, *, expert_mode: bool) -> dict[str, Any]:
    merged = {**DEFAULT_SETTINGS_PREFERENCES, **(saved or {})}
    merged["expertMode"] = bool(expert_mode)
    return merged


async def feature_access(request: Request) -> dict[str, bool]:
    if not hasattr(request.app.state, "auth_service"):
        snapshot = build_entitlement_snapshot(
            account_id=None,
            plan_tier=settings.default_plan,
            source="local-default-config",
        )
        return entitlement_access_map(snapshot)
    return (await request.app.state.auth_service.current_access_context())["feature_access"]


@router.get("/status")
async def status(request: Request) -> dict:
    orchestrator = request.app.state.orchestrator
    foundation = request.app.state.foundation
    optimizer = request.app.state.optimizer
    benchmark_engine = request.app.state.benchmark_engine
    adaptive_engine = request.app.state.adaptive_engine
    state = await orchestrator.state_for_plan()
    return {
        "ok": True,
        "runtime": foundation["runtime"],
        "machine": state.get("machine") or {"id": settings.machine_id, "name": settings.machine_name},
        "plan": state.get("plan", settings.default_plan),
        "plan_info": state.get("plan_info") or {"plan_tier": state.get("plan", settings.default_plan), "status": "active"},
        "executor": state.get("executor") or {
            "dry_run": settings.executor_dry_run,
            "auto_heal_enabled": settings.enable_auto_heal,
            "mode": settings.auto_heal_mode,
        },
        "recovery": foundation["recovery"],
        "capabilities": foundation["capability_snapshot"],
        "hardware_profile": foundation["hardware_profile"],
        "active_temporary_tweaks": len(await optimizer.temporary_tweaks.list_active()),
        "active_temporary_tweak_items": await optimizer.temporary_tweaks.list_active(),
        "benchmark_running": benchmark_engine.running,
        "adaptive_engine": await adaptive_engine.status(limit=6),
        "network_diagnostics": await request.app.state.network_optimizer.latest_diagnostics(),
        "expert_mode": (await request.app.state.foundation["database"].get_setting("expert_mode") or {"enabled": False}).get("enabled", False),
        "auth": await request.app.state.auth_service.get_auth_status(),
        "feature_access": state.get("feature_access") or await feature_access(request),
        "entitlement_snapshot": state.get("entitlement_snapshot"),
    }


@router.get("/capabilities")
async def capabilities(request: Request) -> dict:
    cache_key = "capabilities.snapshot"
    cached = _cache_get(request, cache_key, ttl_seconds=60.0)
    if cached is not None:
        return cached
    foundation = request.app.state.foundation
    payload = {
        "capability_snapshot": foundation.get("capability_snapshot"),
        "hardware_profile": foundation.get("hardware_profile"),
    }
    return _cache_set(request, cache_key, payload)


@router.get("/state")
async def state(request: Request) -> dict:
    orchestrator = request.app.state.orchestrator
    return await orchestrator.state_for_plan()


@router.get("/metrics")
async def metrics(request: Request) -> dict:
    state = request.app.state.orchestrator.current_state
    return {
        "metrics": state.get("metrics"),
        "health_score": state.get("health_score"),
        "session_mode": state.get("session_mode"),
        "active_session": state.get("active_session"),
    }


@router.get("/metrics/live")
async def metrics_live(request: Request) -> StreamingResponse:
    metrics_service = request.app.state.metrics_service
    orchestrator = request.app.state.orchestrator
    return StreamingResponse(metrics_service.stream(orchestrator), media_type="text/event-stream")


@router.get("/tweaks")
async def tweaks(request: Request) -> list[dict]:
    orchestrator = request.app.state.orchestrator
    optimizer = request.app.state.optimizer
    snapshot = orchestrator.last_snapshot
    session_mode = orchestrator.current_state.get("session_mode", "normal")
    catalog = optimizer.catalog(snapshot=snapshot, session_mode=session_mode)
    active_by_tweak = {
        item.get("tweak_id"): item
        for item in await optimizer.temporary_tweaks.list_active()
        if item.get("tweak_id")
    }
    for tweak in catalog:
        active_record = active_by_tweak.get(tweak.get("id"))
        tweak["applied"] = bool(active_record)
        tweak["active_snapshot_id"] = active_record.get("snapshot_id") if active_record else None
        tweak["active_target"] = active_record.get("target") if active_record else None
    return catalog


@router.post("/tweaks/{tweak_id}/apply")
async def apply_tweak(request: Request, tweak_id: str, payload: TweakApplyRequest) -> dict:
    optimizer = request.app.state.optimizer
    snapshot = request.app.state.orchestrator.last_snapshot
    try:
        result = await optimizer.apply_tweak(tweak_id, snapshot=snapshot, params=payload.params, triggered_by="api")
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc
    if not result.get("success"):
        raise HTTPException(status_code=400, detail=result.get("error", "Failed to apply tweak."))
    _cache_invalidate(request, "games.profiles")
    return result


@router.post("/tweaks/{tweak_id}/revert")
async def revert_tweak(request: Request, tweak_id: str, payload: TweakRevertRequest) -> dict:
    optimizer = request.app.state.optimizer
    try:
        result = await optimizer.revert_tweak(tweak_id=tweak_id, snapshot_id=payload.snapshot_id, triggered_by="api")
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc
    if not result.get("success"):
        raise HTTPException(status_code=400, detail=result.get("error", "Failed to revert tweak."))
    _cache_invalidate(request, "games.profiles")
    return result


@router.get("/settings")
async def app_settings(request: Request) -> dict:
    orchestrator = request.app.state.orchestrator
    state = await orchestrator.state_for_plan()
    auth_status = await request.app.state.auth_service.get_auth_status()
    database = request.app.state.foundation["database"]
    expert_mode_enabled = (await database.get_setting("expert_mode") or {"enabled": False}).get("enabled", False)
    saved_preferences = await database.get_setting("preferences") or {}
    return {
        "plan": state["plan"],
        "plan_info": state.get("plan_info") or auth_status["plan"],
        "features": state["features"],
        "feature_access": state.get("feature_access") or auth_status["feature_access"],
        "auth_enabled": settings.auth_enabled,
        "auth_dev_mode": settings.auth_dev_mode,
        "stripe_public_key": settings.stripe_public_key or None,
        "machine": state.get("machine") or {"id": settings.machine_id, "name": settings.machine_name},
        "executor": state.get("executor") or {
            "dry_run": settings.executor_dry_run,
            "auto_heal_enabled": settings.enable_auto_heal,
            "mode": settings.auto_heal_mode,
        },
        "expert_mode": expert_mode_enabled,
        "preferences": merged_settings_preferences(saved_preferences, expert_mode=expert_mode_enabled),
        "auth": auth_status,
    }


@router.get("/auth/status")
async def auth_status(request: Request) -> dict:
    return await request.app.state.auth_service.get_auth_status()


@router.post("/auth/local-session")
async def auth_local_session(request: Request, payload: LocalPlaceholderSignInRequest) -> dict:
    if not settings.auth_dev_mode:
        raise HTTPException(status_code=403, detail="Local placeholder auth mode is disabled.")
    try:
        return await request.app.state.auth_service.sign_in_local_placeholder(
            email=payload.email,
            display_name=payload.display_name,
            plan_tier=payload.plan_tier,
        )
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@router.post("/auth/token-exchange")
async def auth_token_exchange(request: Request, payload: TokenExchangeRequest) -> dict:
    return await request.app.state.auth_service.token_exchange_placeholder(
        provider=payload.provider,
        authorization_code=payload.authorization_code,
    )


@router.post("/auth/refresh-entitlements")
async def auth_refresh_entitlements(request: Request) -> dict:
    return await request.app.state.auth_service.refresh_entitlement_snapshot(triggered_by="api")


@router.post("/auth/sign-out")
async def auth_sign_out(request: Request) -> dict:
    return await request.app.state.auth_service.clear_session(triggered_by="api")


@router.get("/account/identity")
async def account_identity(request: Request) -> dict:
    status = await request.app.state.auth_service.get_auth_status()
    return {"signed_in": status["signed_in"], "identity": status["identity"]}


@router.get("/account/plan")
async def account_plan(request: Request) -> dict:
    status = await request.app.state.auth_service.get_auth_status()
    return {"signed_in": status["signed_in"], "plan": status["plan"]}


@router.get("/account/entitlements")
async def account_entitlements(request: Request) -> dict:
    status = await request.app.state.auth_service.get_auth_status()
    return {
        "signed_in": status["signed_in"],
        "entitlement_snapshot": status["entitlement_snapshot"],
        "feature_access": status["feature_access"],
    }


@router.get("/account/activation")
async def account_activation(request: Request) -> dict:
    status = await request.app.state.auth_service.get_auth_status()
    return {"signed_in": status["signed_in"], "activation": status["activation"]}


@router.get("/audit")
async def audit(request: Request, limit: int = Query(default=100, le=500)) -> list[dict]:
    return await request.app.state.foundation["audit_log"].recent(limit=limit)


@router.post("/audit/{audit_id}/revert")
async def audit_revert(request: Request, audit_id: str) -> dict:
    database = request.app.state.foundation["database"]
    entry = await database.get_audit_entry(audit_id)
    if not entry:
        raise HTTPException(status_code=404, detail="Audit entry not found.")
    snapshot_id = (entry.get("after_value") or {}).get("revert_snapshot_id")
    if not snapshot_id:
        raise HTTPException(status_code=409, detail="This audit entry does not expose a revert snapshot.")
    result = await request.app.state.optimizer.revert_tweak(snapshot_id=snapshot_id, triggered_by="audit_revert")
    if not result.get("success"):
        raise HTTPException(status_code=400, detail=result.get("error", "Failed to revert audit entry."))
    return result


@router.post("/settings/expert-mode")
async def settings_expert_mode(request: Request, payload: ExpertModeRequest) -> dict:
    database = request.app.state.foundation["database"]
    before = await database.get_setting("expert_mode") or {"enabled": False}
    await database.set_setting("expert_mode", {"enabled": payload.enabled}, source="api")
    await request.app.state.foundation["audit_log"].record_event(
        module="settings",
        action="set_expert_mode",
        target="settings.expert_mode",
        before_value=before,
        after_value={"enabled": payload.enabled},
        rationale="Updated the expert-mode gate for high-risk controls.",
        validity_tag="VALIDATED",
        triggered_by="api",
        status="success",
    )
    return {"enabled": payload.enabled}


@router.post("/settings/preferences")
async def settings_preferences(request: Request, payload: SettingsPreferencesRequest) -> dict:
    database = request.app.state.foundation["database"]
    expert_mode_enabled = (await database.get_setting("expert_mode") or {"enabled": False}).get("enabled", False)
    before = await database.get_setting("preferences") or {}
    merged = merged_settings_preferences(payload.preferences, expert_mode=expert_mode_enabled)
    await database.set_setting("preferences", merged, source="api")
    await request.app.state.foundation["audit_log"].record_event(
        module="settings",
        action="update_preferences",
        target="settings.preferences",
        before_value=before,
        after_value=merged,
        rationale="Persisted application-facing desktop preferences for the shipped UI.",
        validity_tag="VALIDATED",
        triggered_by="api",
        status="success",
    )
    return {"preferences": merged}


@router.post("/settings/data-action")
@router.post("/settings/data-actions")
async def settings_data_action(request: Request, payload: SettingsDataActionRequest) -> dict:
    database = request.app.state.foundation["database"]
    audit_log = request.app.state.foundation["audit_log"]
    action = payload.action

    if action == "export_all_data":
        settings_payload = await database.list_settings(category="settings")
        game_profiles = await database.list_game_profiles()
        benchmark_results = await database.list_benchmark_results(limit=1000)
        audit_entries = await audit_log.recent(limit=1000)
        export_payload = {
            "exported_at": time.time(),
            "settings": settings_payload,
            "game_profiles": game_profiles,
            "benchmark_results": benchmark_results,
            "audit_entries": audit_entries,
        }
        empty_state = not settings_payload and not game_profiles and not benchmark_results and not audit_entries
        return {
            "success": True,
            "action": action,
            "empty_state": empty_state,
            "payload": export_payload,
        }

    if action == "clear_benchmark_history":
        summary = await database.clear_benchmark_history()
        await audit_log.record_event(
            module="settings",
            action="clear_benchmark_history",
            target="benchmark_results",
            before_value=summary,
            after_value={"cleared": True},
            rationale="Cleared stored benchmark history through the settings data action.",
            validity_tag="VALIDATED",
            triggered_by="api",
            status="success",
        )
        return {
            "success": True,
            "action": action,
            "deleted_results": summary["deleted_results"],
            "deleted_runs": summary["deleted_runs"],
            "empty_state": summary["deleted_results"] == 0 and summary["deleted_runs"] == 0,
        }

    if action == "reset_all_settings":
        before_preferences = await database.get_setting("preferences") or {}
        before_expert_mode = await database.get_setting("expert_mode") or {"enabled": False}
        default_preferences = dict(DEFAULT_SETTINGS_PREFERENCES)
        default_preferences["expertMode"] = False
        await database.set_setting("preferences", default_preferences, source="api")
        await database.set_setting("expert_mode", {"enabled": False}, source="api")
        await audit_log.record_event(
            module="settings",
            action="reset_all_settings",
            target="settings.preferences",
            before_value={"preferences": before_preferences, "expert_mode": before_expert_mode},
            after_value={"preferences": default_preferences, "expert_mode": {"enabled": False}},
            rationale="Reset settings to a safe default baseline through the data-action surface.",
            validity_tag="VALIDATED",
            triggered_by="api",
            status="success",
        )
        return {
            "success": True,
            "action": action,
            "preferences": default_preferences,
            "expert_mode": {"enabled": False},
            "empty_state": not before_preferences,
        }

    if action == "export_report":
        report_generator = ReportGenerator()
        health_history = await database.list_health_history(days=7, limit=3000)
        report_path = await report_generator.export_markdown_report(
            database=database,
            optimizer=request.app.state.optimizer,
            state=request.app.state.orchestrator.current_state,
            health_history=health_history,
        )
        await audit_log.record_event(
            module="settings",
            action="export_report",
            target=str(report_path),
            before_value=None,
            after_value={"path": str(report_path)},
            rationale="Exported a one-click PulseBoost system report to the user documents directory.",
            validity_tag="VALIDATED",
            triggered_by="api",
            status="success",
        )
        return {
            "success": True,
            "action": action,
            "path": str(report_path),
        }

    imported = payload.payload if isinstance(payload.payload, dict) else {}
    import_preferences = imported.get("preferences", imported)
    if not isinstance(import_preferences, dict):
        raise HTTPException(status_code=400, detail="Invalid settings import payload.")
    explicit_expert = imported.get("expert_mode")
    if isinstance(explicit_expert, dict):
        expert_mode_enabled = bool(explicit_expert.get("enabled", False))
    else:
        expert_mode_enabled = bool(import_preferences.get("expertMode", False))
    merged = merged_settings_preferences(import_preferences, expert_mode=expert_mode_enabled)
    before = await database.get_setting("preferences") or {}
    await database.set_setting("preferences", merged, source="api")
    await database.set_setting("expert_mode", {"enabled": expert_mode_enabled}, source="api")
    await audit_log.record_event(
        module="settings",
        action="import_settings",
        target="settings.preferences",
        before_value=before,
        after_value=merged,
        rationale="Imported settings payload through the settings data-action endpoint.",
        validity_tag="VALIDATED",
        triggered_by="api",
        status="success",
    )
    return {
        "success": True,
        "action": action,
        "preferences": merged,
        "expert_mode": {"enabled": expert_mode_enabled},
        "empty_state": not bool(imported),
    }


@router.get("/timeline")
async def timeline(
    request: Request,
    limit: int = Query(default=300, le=1000),
    hours: int | None = Query(default=None, ge=1, le=24 * 30),
) -> list[dict]:
    orchestrator = request.app.state.orchestrator
    plan_state = await orchestrator.state_for_plan()
    allowed_window = plan_state["history_window_seconds"]
    requested_window = hours * 3600 if hours else allowed_window
    if allowed_window is not None and requested_window and requested_window > allowed_window:
        requested_window = allowed_window
    return await orchestrator.memory.timeline(limit=limit, window_seconds=requested_window)


@router.get("/health/history")
async def health_history(request: Request, days: int = Query(default=7, ge=1, le=30)) -> list[dict]:
    cache_key = f"health.history:{days}"
    cached = _cache_get(request, cache_key, ttl_seconds=30.0)
    if cached is not None:
        return cached
    database = request.app.state.foundation["database"]
    payload = await database.list_health_history(days=days)
    return _cache_set(request, cache_key, payload)


@router.get("/timeline/snapshot")
async def timeline_snapshot(request: Request, timestamp: float) -> dict:
    orchestrator = request.app.state.orchestrator
    snapshot = await orchestrator.memory.snapshot_nearest(timestamp)
    if not snapshot:
        raise HTTPException(status_code=404, detail="No historical snapshot found.")
    return snapshot


@router.get("/actions")
async def actions(request: Request, limit: int = Query(default=25, le=200)) -> list[dict]:
    orchestrator = request.app.state.orchestrator
    state = await orchestrator.state_for_plan()
    if not state["features"]["audit_log"]:
        return []
    return await orchestrator.memory.recent_actions(limit=limit)


@router.get("/actions/export", response_class=PlainTextResponse)
async def export_actions(request: Request, limit: int = Query(default=200, le=1000)) -> str:
    access = await feature_access(request)
    if not access.get("audit_export", False):
        raise HTTPException(status_code=403, detail="Audit export is locked until a paid entitlement is available.")
    orchestrator = request.app.state.orchestrator
    return await orchestrator.memory.export_actions_csv(limit=limit)


@router.get("/anomalies")
async def anomalies(
    request: Request,
    limit: int = Query(default=10, le=100),
    unresolved_only: bool = False,
) -> list[dict]:
    return await request.app.state.orchestrator.memory.recent_anomalies(limit=limit, unresolved_only=unresolved_only)


@router.get("/suggestions")
async def suggestions(request: Request) -> list[dict]:
    state = request.app.state.orchestrator.current_state
    smart = state.get("smart_suggestions", [])
    if smart:
        return smart
    return state.get("suggested_actions", [])


@router.get("/optimizations")
async def optimizations(request: Request, limit: int = Query(default=25, le=200)) -> list[dict]:
    return await request.app.state.orchestrator.memory.recent_optimizations(limit=limit)


@router.post("/actions/preview")
async def preview_action(request: Request, payload: ActionRequest) -> dict:
    return await request.app.state.orchestrator.preview_action(payload.action_type, payload.params)


@router.post("/actions/execute")
async def execute_action(request: Request, payload: ExecuteActionRequest) -> dict:
    result = await request.app.state.orchestrator.execute_action(
        payload.action_type,
        payload.params,
        confirmed=payload.confirmed,
    )
    if result.get("blocked"):
        raise HTTPException(status_code=409, detail=result["thought"])
    return result


@router.post("/optimizations/decision")
async def optimization_decision(request: Request, payload: OptimizationDecisionRequest) -> dict:
    result = await request.app.state.orchestrator.decide_optimization(payload.optimization_id, payload.decision)
    if result.get("blocked"):
        raise HTTPException(status_code=409, detail=result.get("thought"))
    if not result.get("success"):
        raise HTTPException(status_code=404, detail=result.get("error", "Optimization decision failed."))
    return result


@router.get("/adaptive/status")
async def adaptive_status(request: Request) -> dict:
    return await request.app.state.adaptive_engine.status(limit=10)


@router.post("/adaptive/toggle")
async def adaptive_toggle(request: Request, payload: AdaptiveToggleRequest) -> dict:
    return await request.app.state.adaptive_engine.set_enabled(payload.enabled, triggered_by="api")


@router.get("/network/diagnostics")
async def network_diagnostics(request: Request) -> dict:
    orchestrator = request.app.state.orchestrator
    return await request.app.state.network_optimizer.diagnostics(
        session_mode=orchestrator.current_state.get("session_mode", "normal"),
        active_session=orchestrator.current_state.get("active_session"),
    )


@router.post("/network/qos")
async def network_qos(request: Request, payload: NetworkQosRequest) -> dict:
    access = await feature_access(request)
    if not access.get("advanced_network_controls", False):
        raise HTTPException(status_code=403, detail="Advanced network controls are locked for the current entitlement snapshot.")
    return await request.app.state.network_optimizer.apply_qos_profile(
        payload.profile_name,
        protocol=payload.protocol,
        triggered_by="api",
    )


@router.get("/gpu/stats")
async def gpu_stats(request: Request) -> dict:
    return await request.app.state.gpu_controller.stats()


@router.post("/gpu/settings")
async def gpu_settings(request: Request, payload: GpuSettingRequest) -> dict:
    access = await feature_access(request)
    if not access.get("advanced_gpu_controls", False):
        raise HTTPException(status_code=403, detail="Advanced GPU controls are locked for the current entitlement snapshot.")
    return await request.app.state.gpu_controller.apply_setting(
        payload.setting_id,
        payload.value,
        confirm_risky=payload.confirm_risky,
        triggered_by="api",
    )


@router.get("/bios/checklist")
async def bios_checklist(request: Request) -> dict:
    return await request.app.state.gpu_controller.bios_checklist()


@router.get("/games")
async def games(request: Request) -> list[dict]:
    return await request.app.state.game_profile_service.list_games()


@router.get("/games/profiles")
async def games_profiles(request: Request) -> list[dict]:
    cache_key = "games.profiles"
    cached = _cache_get(request, cache_key, ttl_seconds=10.0)
    if cached is not None:
        return cached
    payload = await request.app.state.game_profile_service.list_games()
    return _cache_set(request, cache_key, payload)


@router.get("/games/{game_id}/profile")
async def game_profile(request: Request, game_id: str) -> dict:
    return await request.app.state.game_profile_service.get_profile(game_id)


@router.post("/games/{game_id}/profile")
async def save_game_profile(request: Request, game_id: str, payload: GameProfileRequest) -> dict:
    saved = await request.app.state.game_profile_service.save_profile(game_id, payload.model_dump())
    _cache_invalidate(request, "games.profiles")
    return saved


@router.get("/games/{game_id}/export", response_class=PlainTextResponse)
async def export_game_profile(request: Request, game_id: str) -> str:
    return await request.app.state.game_profile_service.export_profile(game_id)


@router.post("/games/import")
async def import_game_profile(request: Request, payload: GameProfileRequest) -> dict:
    saved = await request.app.state.game_profile_service.import_profile(payload.model_dump())
    _cache_invalidate(request, "games.profiles")
    return saved


@router.get("/trust-center/status")
async def trust_center_status(request: Request) -> dict:
    return await request.app.state.trust_center_service.status(request.app.state.foundation)


@router.post("/trust-center/rollback-all")
async def trust_center_rollback_all(request: Request) -> dict:
    results = await request.app.state.optimizer.restore_temporary_tweaks(triggered_by="trust_center")
    await request.app.state.foundation["audit_log"].record_event(
        module="trust_center",
        action="rollback_all",
        target="temporary_tweaks",
        before_value={"pending": len(results)},
        after_value={"results": results},
        rationale="Requested a Trust Center rollback of all active temporary tweaks.",
        validity_tag="VALIDATED",
        triggered_by="api",
        status="success",
    )
    return {
        "success": True,
        "restored_count": sum(1 for item in results if item.get("result", {}).get("success")),
        "results": results,
    }


@router.post("/trust-center/undo/{category}")
async def trust_center_undo_category(request: Request, category: str) -> dict:
    normalized = str(category or "").strip().lower()
    if normalized not in {"registry", "network"}:
        raise HTTPException(status_code=400, detail="This category does not support undo operations.")
    results = await request.app.state.optimizer.restore_temporary_tweaks(triggered_by=f"trust_center_undo_{normalized}")
    await request.app.state.foundation["audit_log"].record_event(
        module="trust_center",
        action="undo_category",
        target=normalized,
        before_value={"pending": len(results)},
        after_value={"results": results},
        rationale="Requested category-level rollback from Trust Center permission audit.",
        validity_tag="VALIDATED",
        triggered_by="api",
        status="success",
    )
    return {
        "success": True,
        "category": normalized,
        "restored_count": sum(1 for item in results if item.get("result", {}).get("success")),
        "results": results,
    }


@router.post("/benchmark/run")
async def benchmark_run(request: Request, payload: BenchmarkRunRequest) -> dict:
    engine = request.app.state.benchmark_engine
    orchestrator = request.app.state.orchestrator
    result = await engine.run(
        orchestrator,
        workload_name=payload.workload_name,
        tweak_set=payload.tweak_set,
        duration_seconds=payload.duration_seconds,
        notes=payload.notes,
        revert_after=payload.revert_after,
    )
    if not result.get("success"):
        raise HTTPException(status_code=409, detail=result.get("error", "Benchmark run failed."))
    return result


@router.get("/benchmark/results")
async def benchmark_results(request: Request, limit: int = Query(default=25, le=100)) -> list[dict]:
    access = await feature_access(request)
    if not access.get("benchmark_history", False):
        raise HTTPException(status_code=403, detail="Benchmark history is locked for the current entitlement snapshot.")
    return await request.app.state.benchmark_engine.results(limit=limit)


@router.get("/benchmark/results/{benchmark_id}")
async def benchmark_result(request: Request, benchmark_id: str) -> dict:
    access = await feature_access(request)
    if not access.get("benchmark_history", False):
        raise HTTPException(status_code=403, detail="Benchmark history is locked for the current entitlement snapshot.")
    result = await request.app.state.benchmark_engine.result(benchmark_id)
    if not result:
        raise HTTPException(status_code=404, detail="Benchmark result not found.")
    return result


@router.get("/benchmark/results/{benchmark_id}/export", response_class=PlainTextResponse)
async def benchmark_result_export(request: Request, benchmark_id: str) -> str:
    access = await feature_access(request)
    if not access.get("benchmark_history", False):
        raise HTTPException(status_code=403, detail="Benchmark history is locked for the current entitlement snapshot.")
    result = await request.app.state.benchmark_engine.result(benchmark_id)
    if not result:
        raise HTTPException(status_code=404, detail="Benchmark result not found.")

    def metric_line(label: str, baseline_key: str, optimized_key: str, delta_key: str, suffix: str = "") -> str:
        baseline = (result.get("baseline") or {}).get(baseline_key)
        optimized = (result.get("optimized") or {}).get(optimized_key)
        delta = result.get(delta_key)
        return f"- {label}: baseline={baseline}{suffix}, optimized={optimized}{suffix}, delta={delta}{suffix}"

    lines = [
        "# PulseBoost Benchmark Report",
        "",
        f"- Benchmark ID: {benchmark_id}",
        f"- Workload: {result.get('workload_name')}",
        f"- Verdict: {result.get('verdict')}",
        f"- Created: {result.get('created_at')}",
        "",
        "## Comparison",
        metric_line("CPU", "cpu_percent", "cpu_percent", "cpu_delta", "%"),
        metric_line("RAM", "ram_percent", "ram_percent", "ram_delta", "%"),
        metric_line("Ping", "ping_ms", "ping_ms", "ping_delta", " ms"),
        metric_line("Jitter", "jitter_ms", "jitter_ms", "jitter_delta", " ms"),
        "",
        "## Tweaks",
    ]
    for tweak in result.get("tweak_set") or []:
        lines.append(f"- {tweak}")
    if not (result.get("tweak_set") or []):
        lines.append("- No tweak set captured for this run.")
    return "\n".join(lines) + "\n"




















