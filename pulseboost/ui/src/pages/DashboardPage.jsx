import {
  ActivityIcon,
  ArrowRightIcon,
  CheckCircleIcon,
  ShieldCheckIcon,
  ZapIcon,
} from "lucide-react";

import Card from "../components/Card";
import HealthRing from "../components/HealthRing";
import StatusBadge from "../components/StatusBadge";
import { formatBytes, formatPercent, formatDateTime } from "../utils/formatters";

function sessionLabel(activeSession) {
  if (!activeSession?.started_at) return "No active session";
  const seconds = Math.max(0, Math.floor(Date.now() / 1000) - Number(activeSession.started_at));
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  return `${hours}h ${minutes}m`;
}

function trustReadiness(trustCenter) {
  const snapshots = trustCenter?.rollback_readiness?.pending_revert_snapshots ?? 0;
  return snapshots > 0 ? "Ready" : "Monitoring";
}

export default function DashboardPage({
  activeSession,
  connected,
  healthScore = 92,
  metrics,
  trustCenter,
  visibleOptimizations,
  setActivePage,
}) {
  const recommendations = (visibleOptimizations || []).slice(0, 3);
  const trustItems = [
    {
      label: "Rollback",
      value: trustReadiness(trustCenter),
      icon: CheckCircleIcon,
      color: "text-success",
    },
    {
      label: "Snapshot",
      value: trustCenter?.last_clean_exit?.recorded_at
        ? formatDateTime(trustCenter.last_clean_exit.recorded_at)
        : "Unavailable",
      icon: ShieldCheckIcon,
      color: "text-txt-secondary",
    },
    {
      label: "Active tweaks",
      value: `${trustCenter?.rollback_readiness?.active_temporary_tweaks ?? 0}`,
      icon: ZapIcon,
      color: "text-accent",
    },
    {
      label: "Protected rules",
      value: `${trustCenter?.protected_process_rules?.length ?? 0}`,
      icon: ActivityIcon,
      color: "text-txt-tertiary",
    },
  ];

  const systemRows = [
    {
      label: "CPU",
      value: formatPercent(metrics?.cpu_total),
      extra: metrics?.temperature !== null && metrics?.temperature !== undefined ? `${Number(metrics.temperature).toFixed(1)}°C` : "n/a",
      pct: Number(metrics?.cpu_total || 0),
      color: "bg-accent",
    },
    {
      label: "RAM",
      value: formatPercent(metrics?.ram_percent),
      extra: "Live",
      pct: Number(metrics?.ram_percent || 0),
      color: "bg-warning",
    },
    {
      label: "Disk",
      value: formatPercent(metrics?.disk_percent),
      extra: "Active",
      pct: Number(metrics?.disk_percent || 0),
      color: "bg-info",
    },
    {
      label: "Network",
      value: formatBytes(Number(metrics?.net_recv_bytes || 0) + Number(metrics?.net_sent_bytes || 0)),
      extra: connected ? "Streaming" : "Waiting",
      pct: Math.min(100, Number(metrics?.net_recv_bytes || 0) / (1024 * 1024)),
      color: "bg-success",
    },
  ];

  const openPulseAI = () => {
    setActivePage?.("pulsecore");
    window.setTimeout(() => {
      window.dispatchEvent(new CustomEvent("pulseboost:focus-chat-input"));
    }, 120);
  };

  return (
    <div className="relative isolate space-y-6 overflow-hidden">
      <div
        className="pointer-events-none absolute z-0 rounded-full"
        style={{
          top: "-200px",
          left: "50%",
          transform: "translateX(-50%)",
          width: "600px",
          height: "600px",
          borderRadius: "50%",
          background: "#2D7FF9",
          filter: "blur(120px)",
          opacity: 0.06,
        }}
      />
      <div className="relative z-10 flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Dashboard
        </h1>
        <StatusBadge status={connected ? "LIVE" : "RECOVERY_MODE"} />
      </div>

      <Card className="relative z-10 p-6" elevated>
        <div className="flex flex-col gap-6 md:flex-row md:items-center">
          <HealthRing score={healthScore} size={72} strokeWidth={6} />
          <div className="flex-1 space-y-1">
            <div className="flex items-center gap-2">
              <h2 className="text-lg font-semibold text-txt-primary">
                System Health: {healthScore >= 80 ? "Excellent" : healthScore >= 60 ? "Good" : "Watch"}
              </h2>
              <span className="h-2 w-2 rounded-full bg-success animate-pulse-dot" />
            </div>
            <div className="flex flex-wrap items-center gap-4 text-sm text-txt-secondary">
              <span className="flex items-center gap-1.5">
                <ActivityIcon className="h-3.5 w-3.5" />
                Session active · {sessionLabel(activeSession)}
              </span>
              <span>
                Last optimization: {trustCenter?.last_clean_exit?.recorded_at ? formatDateTime(trustCenter.last_clean_exit.recorded_at) : "Awaiting activity"}
              </span>
            </div>
            <div className="flex items-center gap-1.5 text-sm text-success">
              <ShieldCheckIcon className="h-3.5 w-3.5" />
              Recovery: {trustReadiness(trustCenter)}
            </div>
          </div>
        </div>
      </Card>

      <div className="relative z-10 flex flex-wrap gap-3 border-b border-border-subtle pb-2">
        <button
          type="button"
          onClick={() => setActivePage?.("pulsecore")}
          className="text-xs uppercase tracking-[0.14em] text-txt-tertiary transition-colors hover:text-txt-secondary"
        >
          Boost Now
        </button>
        <button
          type="button"
          onClick={openPulseAI}
          className="text-xs uppercase tracking-[0.14em] text-txt-tertiary transition-colors hover:text-txt-secondary"
        >
          Chat with AI
        </button>
        <button
          type="button"
          onClick={() => setActivePage?.("audit")}
          className="text-xs uppercase tracking-[0.14em] text-txt-tertiary transition-colors hover:text-txt-secondary"
        >
          View Audit
        </button>
      </div>

      <div className="relative z-10 grid gap-5 xl:grid-cols-5">
        <div className="space-y-5 xl:col-span-3">
          <Card className="p-5">
            <h3 className="mb-4 text-[15px] font-semibold text-txt-primary">
              System
            </h3>
            <div className="space-y-3">
              {systemRows.map((metric) => (
                <div key={metric.label} className="flex items-center gap-3">
                  <span className="w-14 text-xs font-medium text-txt-tertiary">
                    {metric.label}
                  </span>
                  <div className="h-1 flex-1 overflow-hidden rounded-full bg-surface-sunken">
                    <div
                      className={`h-full rounded-full transition-all ${metric.color}`}
                      style={{ width: `${metric.pct}%` }}
                    />
                  </div>
                  <span className="numeric-tabular w-24 text-right text-sm font-semibold text-txt-primary">
                    {metric.value}
                  </span>
                  <span className="numeric-tabular w-16 text-[11px] text-txt-tertiary">
                    {metric.extra}
                  </span>
                </div>
              ))}
            </div>
          </Card>

          <Card className="p-5">
            <div className="mb-4 flex items-center justify-between">
              <h3 className="text-[15px] font-semibold text-txt-primary">
                Trust & Recovery
              </h3>
              <button
                onClick={() => setActivePage?.("trust")}
                className="flex items-center gap-1 text-xs text-accent transition-colors hover:text-accent-hover"
                type="button"
              >
                Trust Center <ArrowRightIcon className="h-3 w-3" />
              </button>
            </div>
            <div className="grid gap-3 md:grid-cols-2">
              {trustItems.map((item) => (
                <div
                  key={item.label}
                  className="flex items-center gap-2.5 rounded-md bg-surface-sunken p-2.5"
                >
                  <item.icon className={`h-4 w-4 shrink-0 ${item.color}`} />
                  <div>
                    <div className="text-[11px] text-txt-tertiary">
                      {item.label}
                    </div>
                    <div className="numeric-tabular text-sm font-medium text-txt-primary">
                      {item.value}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </Card>
        </div>

        <div className="xl:col-span-2">
          <Card className="h-full p-5">
            <div className="mb-4 flex items-center justify-between">
              <h3 className="text-[15px] font-semibold text-txt-primary">
                Recommendations
              </h3>
              <span className="rounded bg-accent-muted px-2 py-0.5 text-[10px] font-semibold uppercase tracking-[0.18em] text-accent">
                {recommendations.length} available
              </span>
            </div>
            <div className="space-y-3">
              {recommendations.length ? recommendations.map((recommendation, index) => (
                <div
                  key={recommendation.id || index}
                  className="group flex cursor-pointer items-start gap-2.5 rounded-md bg-surface-sunken p-3 transition-colors hover:bg-surface-hover"
                >
                  <span className="mt-1.5 h-1.5 w-1.5 shrink-0 rounded-full bg-accent" />
                  <div className="min-w-0 flex-1">
                    <div className="text-sm text-txt-primary transition-colors group-hover:text-white">
                      {recommendation.title || recommendation.name}
                    </div>
                    <div className="mt-1 text-xs text-txt-tertiary">
                      {recommendation.description || recommendation.rationale || "Awaiting detailed rationale."}
                    </div>
                    <div className="mt-2 flex items-center gap-2">
                      {recommendation.priority ? <StatusBadge status={recommendation.priority.toUpperCase()} /> : null}
                      {recommendation.requires_admin ? <StatusBadge status="REQUIRES_ADMIN" /> : null}
                    </div>
                  </div>
                </div>
              )) : (
                <div className="rounded-md bg-surface-sunken p-4 text-sm text-txt-tertiary">
                  No recommendations are pending. PulseBoost is holding the current posture.
                </div>
              )}
            </div>
            <button
              onClick={() => setActivePage?.("pulsecore")}
              className="mt-4 flex items-center gap-1 text-xs text-accent transition-colors hover:text-accent-hover"
              type="button"
            >
              Open PulseCore <ArrowRightIcon className="h-3 w-3" />
            </button>
          </Card>
        </div>
      </div>
    </div>
  );
}
