from __future__ import annotations

from typing import Any

from pydantic import BaseModel, Field


class ChatRequest(BaseModel):
    message: str = Field(min_length=1)
    history: list[dict[str, Any]] = Field(default_factory=list)


class ActionRequest(BaseModel):
    action_type: str
    params: dict[str, Any] = Field(default_factory=dict)


class ExecuteActionRequest(ActionRequest):
    confirmed: bool = False


class OptimizationDecisionRequest(BaseModel):
    optimization_id: str
    decision: str = Field(pattern="^(approve|dismiss)$")


class TweakApplyRequest(BaseModel):
    params: dict[str, Any] = Field(default_factory=dict)


class TweakRevertRequest(BaseModel):
    snapshot_id: str | None = None


class BenchmarkRunRequest(BaseModel):
    workload_name: str = Field(min_length=1)
    tweak_set: list[str] = Field(default_factory=list)
    duration_seconds: int = Field(default=6, ge=2, le=30)
    notes: str = ""
    revert_after: bool = True


class AdaptiveToggleRequest(BaseModel):
    enabled: bool


class NetworkQosRequest(BaseModel):
    profile_name: str = Field(min_length=1)
    protocol: str = Field(default="udp")


class GpuSettingRequest(BaseModel):
    setting_id: str = Field(min_length=1)
    value: Any
    confirm_risky: bool = False


class GameProfileRequest(BaseModel):
    game_name: str | None = None
    executable_path: str | None = None
    recommended_tweaks: list[str] = Field(default_factory=list)
    notes: str = ""
    history: dict[str, Any] = Field(default_factory=dict)
    recommendation_basis: dict[str, Any] = Field(default_factory=dict)


class ExpertModeRequest(BaseModel):
    enabled: bool


class SettingsPreferencesRequest(BaseModel):
    preferences: dict[str, Any] = Field(default_factory=dict)


class SettingsDataActionRequest(BaseModel):
    action: str = Field(
        pattern="^(export_all_data|import_settings|clear_benchmark_history|reset_all_settings|export_report)$"
    )
    payload: dict[str, Any] = Field(default_factory=dict)


class LocalPlaceholderSignInRequest(BaseModel):
    email: str = Field(min_length=3)
    display_name: str = Field(default="", max_length=120)
    plan_tier: str = Field(default="free")


class TokenExchangeRequest(BaseModel):
    provider: str = Field(min_length=1)
    authorization_code: str = Field(min_length=1)


class SettingsResponse(BaseModel):
    plan: str
    features: dict[str, Any]
    auth_enabled: bool
    stripe_public_key: str | None = None
    machine: dict[str, Any]
    executor: dict[str, Any]


class SystemStateResponse(BaseModel):
    health_score: float
    efficiency_score: float | None = None
    efficiency_grade: str | None = None
    metrics: dict[str, Any] | None
    breakdown: dict[str, float]
    efficiency_breakdown: dict[str, float] | None = None
    session_mode: str
    anomalies: list[dict[str, Any]]
    predictions: list[dict[str, Any]]
    optimizations: list[dict[str, Any]] | None = None
    process_intelligence: list[dict[str, Any]] | None = None
    actions: list[dict[str, Any]]
    alerts: list[dict[str, Any]]










