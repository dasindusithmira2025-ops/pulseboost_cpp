from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any


class SerializableModel:
    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def _coalesce(*values: Any) -> Any:
    for value in values:
        if value is not None:
            return value
    return None


def _safe_int(value: Any, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def _percent_delta(updated: float | None, baseline: float | None) -> float | None:
    if updated is None or baseline in (None, 0):
        return None
    return round(((updated - baseline) / baseline) * 100.0, 2)


def normalize_benchmark_capture_payload(payload: dict[str, Any] | None) -> dict[str, Any]:
    raw = dict(payload or {})
    return {
        "avg_fps": raw.get("avg_fps"),
        "one_percent_low_fps": raw.get("one_percent_low_fps"),
        "average_frame_time_ms": raw.get("average_frame_time_ms"),
        "p95_frame_time_ms": raw.get("p95_frame_time_ms"),
        "frame_time_variance_ms": raw.get("frame_time_variance_ms"),
        "frametime_supported": bool(raw.get("frametime_supported", False)),
        "frametime_source": raw.get("frametime_source"),
        "frametime_reason": raw.get("frametime_reason"),
        "cpu_percent": raw.get("cpu_percent"),
        "ram_percent": raw.get("ram_percent"),
        "gpu_percent": raw.get("gpu_percent"),
        "ping_ms": raw.get("ping_ms"),
        "jitter_ms": raw.get("jitter_ms"),
        "sample_count": _safe_int(raw.get("sample_count"), 0),
        "frame_sample_count": _safe_int(raw.get("frame_sample_count"), 0),
        "unstable": bool(raw.get("unstable", False)),
        "unsupported_reasons": list(raw.get("unsupported_reasons") or []),
    }


def normalize_benchmark_result_payload(payload: dict[str, Any] | None) -> dict[str, Any]:
    raw = dict(payload or {})
    baseline = normalize_benchmark_capture_payload(raw.get("baseline"))
    optimized = normalize_benchmark_capture_payload(raw.get("optimized"))
    frametime_supported = bool(
        raw.get("frametime_supported", baseline["frametime_supported"] and optimized["frametime_supported"])
    )
    normalized = {
        "benchmark_id": raw.get("benchmark_id"),
        "workload_name": raw.get("workload_name"),
        "created_at": raw.get("created_at"),
        "duration_seconds": _safe_int(raw.get("duration_seconds"), 0),
        "tweak_set": list(raw.get("tweak_set") or []),
        "session_id": raw.get("session_id"),
        "baseline": baseline,
        "optimized": optimized,
        "frametime_supported": frametime_supported,
        "frametime_source": _coalesce(raw.get("frametime_source"), baseline["frametime_source"], optimized["frametime_source"]),
        "baseline_fps_average": _coalesce(raw.get("baseline_fps_average"), baseline["avg_fps"]),
        "optimized_fps_average": _coalesce(raw.get("optimized_fps_average"), optimized["avg_fps"]),
        "baseline_fps_1_low": _coalesce(raw.get("baseline_fps_1_low"), baseline["one_percent_low_fps"]),
        "optimized_fps_1_low": _coalesce(raw.get("optimized_fps_1_low"), optimized["one_percent_low_fps"]),
        "baseline_average_frame_time_ms": _coalesce(raw.get("baseline_average_frame_time_ms"), baseline["average_frame_time_ms"]),
        "optimized_average_frame_time_ms": _coalesce(raw.get("optimized_average_frame_time_ms"), optimized["average_frame_time_ms"]),
        "baseline_p95_frame_time_ms": _coalesce(raw.get("baseline_p95_frame_time_ms"), baseline["p95_frame_time_ms"]),
        "optimized_p95_frame_time_ms": _coalesce(raw.get("optimized_p95_frame_time_ms"), optimized["p95_frame_time_ms"]),
        "baseline_frame_time_variance_ms": _coalesce(raw.get("baseline_frame_time_variance_ms"), baseline["frame_time_variance_ms"]),
        "optimized_frame_time_variance_ms": _coalesce(raw.get("optimized_frame_time_variance_ms"), optimized["frame_time_variance_ms"]),
        "fps_delta_percent": raw.get("fps_delta_percent"),
        "p95_frame_time_delta_percent": raw.get("p95_frame_time_delta_percent"),
        "baseline_frame_sample_count": _safe_int(_coalesce(raw.get("baseline_frame_sample_count"), baseline["frame_sample_count"]), 0),
        "optimized_frame_sample_count": _safe_int(_coalesce(raw.get("optimized_frame_sample_count"), optimized["frame_sample_count"]), 0),
        "frame_time_reason": _coalesce(raw.get("frame_time_reason"), baseline["frametime_reason"], optimized["frametime_reason"]),
        "frame_time_evidence_unstable": bool(raw.get("frame_time_evidence_unstable", False)),
        "frame_time_evidence_reason": raw.get("frame_time_evidence_reason"),
        "avg_fps_delta": raw.get("avg_fps_delta"),
        "one_percent_low_delta": raw.get("one_percent_low_delta"),
        "frame_time_variance_delta": raw.get("frame_time_variance_delta"),
        "cpu_delta": raw.get("cpu_delta"),
        "ram_delta": raw.get("ram_delta"),
        "gpu_delta": raw.get("gpu_delta"),
        "ping_delta": raw.get("ping_delta"),
        "jitter_delta": raw.get("jitter_delta"),
        "notes": raw.get("notes", ""),
        "verdict": raw.get("verdict", "NO_MEASURABLE_IMPACT"),
        "supported_metrics": dict(raw.get("supported_metrics") or {}),
        "unsupported_reasons": list(raw.get("unsupported_reasons") or []),
        "apply_results": list(raw.get("apply_results") or []),
        "revert_results": list(raw.get("revert_results") or []),
    }
    if normalized["fps_delta_percent"] is None:
        normalized["fps_delta_percent"] = _percent_delta(
            normalized["optimized_fps_average"],
            normalized["baseline_fps_average"],
        )
    if normalized["p95_frame_time_delta_percent"] is None:
        normalized["p95_frame_time_delta_percent"] = _percent_delta(
            normalized["optimized_p95_frame_time_ms"],
            normalized["baseline_p95_frame_time_ms"],
        )
    return normalized


@dataclass(slots=True)
class TweakObject(SerializableModel):
    id: str
    name: str
    category: str
    registry_path: str | None = None
    key: str | None = None
    before_value: Any = None
    after_value: Any = None
    rationale: str = ""
    validity: str = "VALIDATED"
    impact: str = ""
    compatibility_note: str = ""
    hardware_requirements: list[str] = field(default_factory=list)
    temporary: bool = True
    requires_admin: bool = False
    applied: bool = False
    timestamp: float | None = None
    apply_fn: str | None = None
    revert_fn: str | None = None


@dataclass(slots=True)
class AuditEntry(SerializableModel):
    id: str
    timestamp: float
    module: str
    action: str
    target: str
    before_value: Any = None
    after_value: Any = None
    rationale: str = ""
    validity_tag: str = "VALIDATED"
    triggered_by: str = "system"
    reverted: bool = False
    revert_timestamp: float | None = None
    session_id: str | None = None
    status: str = "success"


@dataclass(slots=True)
class SessionRecord(SerializableModel):
    id: str
    game_name: str | None
    executable_path: str | None
    started_at: float
    ended_at: float | None = None
    baseline_metrics_snapshot: dict[str, Any] = field(default_factory=dict)
    final_metrics_snapshot: dict[str, Any] = field(default_factory=dict)
    tweaks_applied_count: int = 0
    adaptive_actions_count: int = 0
    stability_score: float | None = None
    clean_exit: bool = True


@dataclass(slots=True)
class HardwareProfile(SerializableModel):
    machine_id: str
    machine_name: str
    captured_at: float
    os_name: str
    os_version: str
    os_build: str | None
    cpu_name: str
    cpu_logical_cores: int
    cpu_physical_cores: int | None
    ram_total_bytes: int
    gpu_vendor: str | None = None
    gpu_model: str | None = None
    motherboard: str | None = None
    bios_advisory: list[str] = field(default_factory=list)
    nic_details: list[dict[str, Any]] = field(default_factory=list)
    supported_capabilities: list[str] = field(default_factory=list)
    desktop_runtime: str = "unknown"


@dataclass(slots=True)
class BenchmarkCapture(SerializableModel):
    avg_fps: float | None = None
    one_percent_low_fps: float | None = None
    average_frame_time_ms: float | None = None
    p95_frame_time_ms: float | None = None
    frame_time_variance_ms: float | None = None
    frametime_supported: bool = False
    frametime_source: str | None = None
    frametime_reason: str | None = None
    cpu_percent: float | None = None
    ram_percent: float | None = None
    gpu_percent: float | None = None
    ping_ms: float | None = None
    jitter_ms: float | None = None
    sample_count: int = 0
    frame_sample_count: int = 0
    unstable: bool = False
    unsupported_reasons: list[str] = field(default_factory=list)


@dataclass(slots=True)
class BenchmarkResult(SerializableModel):
    benchmark_id: str
    workload_name: str
    created_at: float
    duration_seconds: int
    tweak_set: list[str] = field(default_factory=list)
    session_id: str | None = None
    baseline: dict[str, Any] = field(default_factory=dict)
    optimized: dict[str, Any] = field(default_factory=dict)
    frametime_supported: bool = False
    frametime_source: str | None = None
    baseline_fps_average: float | None = None
    optimized_fps_average: float | None = None
    baseline_fps_1_low: float | None = None
    optimized_fps_1_low: float | None = None
    baseline_average_frame_time_ms: float | None = None
    optimized_average_frame_time_ms: float | None = None
    baseline_p95_frame_time_ms: float | None = None
    optimized_p95_frame_time_ms: float | None = None
    baseline_frame_time_variance_ms: float | None = None
    optimized_frame_time_variance_ms: float | None = None
    fps_delta_percent: float | None = None
    p95_frame_time_delta_percent: float | None = None
    baseline_frame_sample_count: int = 0
    optimized_frame_sample_count: int = 0
    frame_time_reason: str | None = None
    frame_time_evidence_unstable: bool = False
    frame_time_evidence_reason: str | None = None
    avg_fps_delta: float | None = None
    one_percent_low_delta: float | None = None
    frame_time_variance_delta: float | None = None
    cpu_delta: float | None = None
    ram_delta: float | None = None
    gpu_delta: float | None = None
    ping_delta: float | None = None
    jitter_delta: float | None = None
    notes: str = ""
    verdict: str = "NO_MEASURABLE_IMPACT"
    supported_metrics: dict[str, bool] = field(default_factory=dict)
    unsupported_reasons: list[str] = field(default_factory=list)
    apply_results: list[dict[str, Any]] = field(default_factory=list)
    revert_results: list[dict[str, Any]] = field(default_factory=list)


@dataclass(slots=True)
class CapabilitySnapshot(SerializableModel):
    machine_id: str
    captured_at: float
    os_name: str
    os_version: str
    is_windows: bool
    is_admin: bool
    desktop_runtime: str
    python_version: str
    has_powershell: bool
    has_wmi: bool
    has_temperature_sensors: bool
    has_gpu_runtime: bool
    supports_electron_runtime: bool
    supports_webview_runtime: bool
    can_manage_services: bool
    can_edit_registry: bool
    can_control_process_priority: bool
    can_control_affinity: bool
    notes: list[str] = field(default_factory=list)


@dataclass(slots=True)
class AppRuntimeState(SerializableModel):
    session_id: str
    runtime: str
    started_at: float
    active: bool
    clean_exit: bool
    recovery_required: bool = False
    previous_session_id: str | None = None
    ended_at: float | None = None


@dataclass(slots=True)
class RecoveryDecision(SerializableModel):
    recovery_required: bool
    detected_at: float
    session_id: str
    previous_session_id: str | None
    rationale: str
    recommended_actions: list[str] = field(default_factory=list)
    status: str = "clear"


@dataclass(slots=True)
class RevertSnapshot(SerializableModel):
    snapshot_id: str
    target_type: str
    target_id: str
    before_value: Any
    created_at: float
    session_id: str | None = None
    restored: bool = False
    restored_at: float | None = None
    restore_status: str | None = None
    metadata: dict[str, Any] = field(default_factory=dict)


@dataclass(slots=True)
class CompatibilityResult(SerializableModel):
    supported: bool
    reasons: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)


@dataclass(slots=True)
class AccountIdentity(SerializableModel):
    account_id: str
    email: str
    display_name: str
    avatar_url: str | None = None
    created_at: float | None = None


@dataclass(slots=True)
class AuthSession(SerializableModel):
    account_id: str
    access_token: str
    refresh_token: str
    expires_at: float
    last_verified_at: float | None
    signed_in: bool


@dataclass(slots=True)
class DeviceActivation(SerializableModel):
    activation_id: str
    account_id: str
    machine_hash: str
    device_name: str
    app_version: str
    activated_at: float
    last_seen_at: float
    revoked: bool = False


@dataclass(slots=True)
class PlanInfo(SerializableModel):
    plan_tier: str
    status: str
    renewal_at: float | None = None
    trial_ends_at: float | None = None


@dataclass(slots=True)
class FeatureEntitlement(SerializableModel):
    feature_key: str
    enabled: bool
    reason: str
    source: str


@dataclass(slots=True)
class EntitlementSnapshot(SerializableModel):
    account_id: str | None
    plan_tier: str
    features: dict[str, FeatureEntitlement] = field(default_factory=dict)
    generated_at: float = 0.0
    source: str = "unknown"

    def feature_enabled(self, feature_key: str) -> bool:
        feature = self.features.get(feature_key)
        return bool(feature.enabled) if feature else False
