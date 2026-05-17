"""
PulseBoost plan and feature gating helpers.
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class PlanFeatures:
    name: str
    machines: str
    history_hours: int | None
    ai_chat_daily_limit: int | None
    auto_heal: str
    predictions: bool
    audit_log: bool
    anomaly_model: str
    api_access: str


PLAN_ORDER = {
    "free": 0,
    "pro": 1,
    "team": 2,
    "enterprise": 3,
}

PLAN_FEATURES = {
    "free": PlanFeatures(
        name="free",
        machines="1",
        history_hours=1,
        ai_chat_daily_limit=10,
        auto_heal="suggest",
        predictions=False,
        audit_log=False,
        anomaly_model="basic",
        api_access="none",
    ),
    "pro": PlanFeatures(
        name="pro",
        machines="1",
        history_hours=None,
        ai_chat_daily_limit=None,
        auto_heal="full_auto",
        predictions=True,
        audit_log=True,
        anomaly_model="learned_baseline",
        api_access="none",
    ),
    "team": PlanFeatures(
        name="team",
        machines="10",
        history_hours=None,
        ai_chat_daily_limit=None,
        auto_heal="full_auto",
        predictions=True,
        audit_log=True,
        anomaly_model="learned_baseline",
        api_access="limited",
    ),
    "enterprise": PlanFeatures(
        name="enterprise",
        machines="unlimited",
        history_hours=None,
        ai_chat_daily_limit=None,
        auto_heal="custom_rules",
        predictions=True,
        audit_log=True,
        anomaly_model="custom_models",
        api_access="full",
    ),
}


def normalize_plan(plan: str | None) -> str:
    candidate = (plan or "free").strip().lower()
    return candidate if candidate in PLAN_FEATURES else "free"


def features_for_plan(plan: str | None) -> PlanFeatures:
    return PLAN_FEATURES[normalize_plan(plan)]


def plan_at_least(plan: str | None, required: str) -> bool:
    return PLAN_ORDER[normalize_plan(plan)] >= PLAN_ORDER[normalize_plan(required)]
