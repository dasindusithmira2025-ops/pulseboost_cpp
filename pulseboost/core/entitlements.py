from __future__ import annotations

import time
from dataclasses import asdict

from core.cognition.plans import features_for_plan, normalize_plan
from core.models import EntitlementSnapshot, FeatureEntitlement, PlanInfo


FEATURE_KEYS = (
    "core_monitoring",
    "safe_optimizations",
    "benchmark_history",
    "premium_benchmark_packs",
    "advanced_gpu_controls",
    "cloud_profile_sync",
    "multi_device_license",
    "enterprise_policy_templates",
    "audit_export",
    "advanced_network_controls",
)


PLAN_FEATURE_RULES = {
    "free": {
        "core_monitoring": (True, "Core machine monitoring remains available on the local desktop runtime."),
        "safe_optimizations": (True, "Safe optimization controls remain available on the base tier."),
        "benchmark_history": (True, "Benchmark history remains visible so local proof workflows keep working."),
        "premium_benchmark_packs": (False, "Premium benchmark packs require a paid entitlement."),
        "advanced_gpu_controls": (False, "Advanced GPU controls require a paid entitlement."),
        "cloud_profile_sync": (False, "Cloud profile sync requires a website-backed account entitlement."),
        "multi_device_license": (False, "Multi-device licensing requires a higher-tier entitlement."),
        "enterprise_policy_templates": (False, "Enterprise policy templates require enterprise licensing."),
        "audit_export": (False, "Audit export requires a paid entitlement."),
        "advanced_network_controls": (False, "Advanced network controls require a paid entitlement."),
    },
    "pro": {
        "core_monitoring": (True, "Core monitoring is included in the Pro tier."),
        "safe_optimizations": (True, "Safe optimizations are included in the Pro tier."),
        "benchmark_history": (True, "Benchmark history is included in the Pro tier."),
        "premium_benchmark_packs": (True, "Premium benchmark packs are included in the Pro tier."),
        "advanced_gpu_controls": (True, "Advanced GPU controls are included in the Pro tier."),
        "cloud_profile_sync": (True, "Cloud profile sync is enabled for Pro accounts when website authority is connected."),
        "multi_device_license": (False, "Multi-device licensing begins at the Team tier."),
        "enterprise_policy_templates": (False, "Enterprise policy templates require enterprise licensing."),
        "audit_export": (True, "Audit export is included in the Pro tier."),
        "advanced_network_controls": (True, "Advanced network controls are included in the Pro tier."),
    },
    "team": {
        "core_monitoring": (True, "Core monitoring is included in the Team tier."),
        "safe_optimizations": (True, "Safe optimizations are included in the Team tier."),
        "benchmark_history": (True, "Benchmark history is included in the Team tier."),
        "premium_benchmark_packs": (True, "Premium benchmark packs are included in the Team tier."),
        "advanced_gpu_controls": (True, "Advanced GPU controls are included in the Team tier."),
        "cloud_profile_sync": (True, "Cloud profile sync is enabled for Team accounts when website authority is connected."),
        "multi_device_license": (True, "The Team tier includes multi-device licensing."),
        "enterprise_policy_templates": (False, "Enterprise policy templates require enterprise licensing."),
        "audit_export": (True, "Audit export is included in the Team tier."),
        "advanced_network_controls": (True, "Advanced network controls are included in the Team tier."),
    },
    "enterprise": {
        "core_monitoring": (True, "Core monitoring is included in the Enterprise tier."),
        "safe_optimizations": (True, "Safe optimizations are included in the Enterprise tier."),
        "benchmark_history": (True, "Benchmark history is included in the Enterprise tier."),
        "premium_benchmark_packs": (True, "Premium benchmark packs are included in the Enterprise tier."),
        "advanced_gpu_controls": (True, "Advanced GPU controls are included in the Enterprise tier."),
        "cloud_profile_sync": (True, "Cloud profile sync is enabled for Enterprise accounts when website authority is connected."),
        "multi_device_license": (True, "Enterprise licensing includes multi-device activation."),
        "enterprise_policy_templates": (True, "Enterprise policy templates are included in the Enterprise tier."),
        "audit_export": (True, "Audit export is included in the Enterprise tier."),
        "advanced_network_controls": (True, "Advanced network controls are included in the Enterprise tier."),
    },
}


def build_plan_info(
    plan_tier: str | None,
    *,
    status: str = "active",
    renewal_at: float | None = None,
    trial_ends_at: float | None = None,
) -> PlanInfo:
    return PlanInfo(
        plan_tier=normalize_plan(plan_tier),
        status=status,
        renewal_at=renewal_at,
        trial_ends_at=trial_ends_at,
    )


def build_entitlement_snapshot(
    *,
    account_id: str | None,
    plan_tier: str | None,
    source: str,
    generated_at: float | None = None,
) -> EntitlementSnapshot:
    normalized_plan = normalize_plan(plan_tier)
    feature_map: dict[str, FeatureEntitlement] = {}
    for feature_key in FEATURE_KEYS:
        enabled, reason = PLAN_FEATURE_RULES[normalized_plan][feature_key]
        feature_map[feature_key] = FeatureEntitlement(
            feature_key=feature_key,
            enabled=enabled,
            reason=reason,
            source=source,
        )
    return EntitlementSnapshot(
        account_id=account_id,
        plan_tier=normalized_plan,
        features=feature_map,
        generated_at=generated_at or time.time(),
        source=source,
    )


def entitlement_access_map(snapshot: EntitlementSnapshot | None) -> dict[str, bool]:
    if snapshot is None:
        return {feature_key: False for feature_key in FEATURE_KEYS}
    return {feature_key: snapshot.feature_enabled(feature_key) for feature_key in FEATURE_KEYS}


def legacy_plan_features(plan_tier: str | None) -> dict[str, object]:
    return asdict(features_for_plan(plan_tier))
