import { LockIcon, SparklesIcon } from "lucide-react";

const TONE_BY_STATUS = {
  VALIDATED: "bg-success-muted text-success",
  LEGACY: "bg-surface-hover text-txt-tertiary",
  HARDWARE_SPECIFIC: "bg-info-muted text-info",
  PLACEBO_RISK: "bg-warning-muted text-warning",
  ACTIVE: "bg-success-muted text-success",
  REVERTED: "bg-surface-hover text-txt-tertiary",
  FAILED: "bg-error-muted text-error",
  AI_APPLIED: "bg-accent-muted text-accent",
  TEMPORARY: "bg-info-muted text-info",
  REQUIRES_ADMIN: "bg-warning-muted text-warning",
  UNSUPPORTED: "bg-surface-hover text-txt-tertiary",
  EXPERT_ONLY: "bg-warning-muted text-warning",
  DRY_RUN: "bg-info-muted text-info",
  LIVE: "bg-success-muted text-success",
  SAFE_DEFAULT: "bg-success-muted text-success",
  RECOVERY_MODE: "bg-error-muted text-error",
  LOCKED: "bg-surface-hover text-txt-tertiary",
  PREMIUM: "bg-premium-muted text-premium",
  MONITORED: "bg-info-muted text-info",
  SIGNED_IN: "bg-success-muted text-success",
  SIGNED_OUT: "bg-surface-hover text-txt-tertiary",
  ENABLED: "bg-success-muted text-success",
  FREE_PLAN: "bg-surface-hover text-txt-tertiary",
  ENTITLED: "bg-premium-muted text-premium",
  WARNING: "bg-warning-muted text-warning",
  AVAILABLE: "bg-success-muted text-success",
};

function renderIcon(status) {
  if (status === "LOCKED") return <LockIcon className="h-3 w-3" />;
  if (status === "PREMIUM" || status === "ENTITLED") return <SparklesIcon className="h-3 w-3" />;
  return null;
}

export default function StatusBadge({ status, label, subtle = false, className = "" }) {
  const raw = String(status || label || "MONITORED").toUpperCase();
  const style = TONE_BY_STATUS[raw] || "bg-surface-hover text-txt-tertiary";
  const dot = raw === "LIVE" || raw === "ACTIVE" || raw === "VALIDATED";
  const pulse = raw === "LIVE" || raw === "RECOVERY_MODE";

  return (
    <span
      className={[
        "inline-flex items-center gap-1 rounded px-2 py-0.5 text-[10px] font-semibold uppercase tracking-[0.14em]",
        subtle ? "border border-border-default/70" : "",
        style,
        className,
      ].join(" ")}
    >
      {dot ? (
        <span className={`h-1.5 w-1.5 rounded-full ${raw === "RECOVERY_MODE" ? "bg-error" : "bg-current"} ${pulse ? "animate-pulse-dot" : ""}`} />
      ) : null}
      {!dot ? renderIcon(raw) : null}
      {raw.replaceAll("_", " ")}
    </span>
  );
}
