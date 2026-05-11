import { lazy, Suspense, useEffect, useMemo, useRef, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import HealthRing from "../components/ui/HealthRing";
import MetricCard from "../components/ui/MetricCard";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { useCommands } from "../hooks/useCommands";
import { healthLabel, percentDelta } from "../lib/format";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import { useUiStore } from "../stores/uiStore";
import type { HomeSummary } from "../types/system";

const LineChart = lazy(() => import("../components/ui/LineChart"));

function relativeTime(timestamp: string): string {
  const date = new Date(timestamp);
  if (Number.isNaN(date.getTime())) {
    return "just now";
  }
  const deltaMs = Date.now() - date.getTime();
  const deltaMinutes = Math.max(0, Math.round(deltaMs / 60000));
  if (deltaMinutes < 1) {
    return "just now";
  }
  if (deltaMinutes < 60) {
    return `${deltaMinutes}m ago`;
  }
  const deltaHours = Math.round(deltaMinutes / 60);
  if (deltaHours < 24) {
    return `${deltaHours}h ago`;
  }
  return `${Math.round(deltaHours / 24)}d ago`;
}

function pulseScoreTone(score: number): "ok" | "warn" | "danger" {
  if (score >= 750) {
    return "ok";
  }
  if (score >= 450) {
    return "warn";
  }
  return "danger";
}

export default function Dashboard() {
  const snapshot = useSystemStore((s) => s.snapshot);
  const history = useSystemStore((s) => s.history);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const setActiveScreen = useUiStore((s) => s.setActiveScreen);
  const { run } = useCommands();
  const [summary, setSummary] = useState<HomeSummary | null>(null);
  const lastSummaryFetchRef = useRef(0);

  useEffect(() => {
    let cancelled = false;

    async function loadHomeSummary() {
      if (!isTauriRuntime()) {
        return;
      }
      const now = Date.now();
      if (summary && now - lastSummaryFetchRef.current < 15000) {
        return;
      }
      try {
        const value = await tauriInvoke<HomeSummary>("get_home_summary");
        if (!cancelled) {
          setSummary(value);
          lastSummaryFetchRef.current = now;
        }
      } catch (error) {
        if (!cancelled) {
          setSummary(null);
        }
        pushNotification(error instanceof Error ? error.message : "Failed to load home summary", "danger");
      }
    }

    void loadHomeSummary();
    return () => {
      cancelled = true;
    };
  }, [pushNotification, snapshot?.timestamp, summary]);

  const topProcesses = useMemo(
    () => (snapshot?.processes ?? []).slice().sort((a, b) => b.cpuPercent - a.cpuPercent).slice(0, 5),
    [snapshot]
  );

  if (!snapshot) {
    return <div className="screen-enter">Loading home summary...</div>;
  }

  const cpuDelta = percentDelta(snapshot.cpuPercent, history.cpu[history.cpu.length - 15]);
  const ramDelta = percentDelta(snapshot.ramPercent, history.ram[history.ram.length - 15]);
  const netCurrent = snapshot.netDownloadKbps / 1024;
  const netDelta = percentDelta(netCurrent, history.net[history.net.length - 15]);
  const tempDelta = percentDelta(snapshot.temperatureCelsius, history.cpu[history.cpu.length - 15]);
  const pulseScore = summary?.pulseScore ?? null;
  const advisorHighlights = (summary?.advisorItems ?? []).filter((item) => item.actionable).slice(0, 2);
  const pulseScorePercent = pulseScore ? Math.max(0, Math.min(100, Math.round(pulseScore.total / 10))) : snapshot.healthScore;

  return (
    <div className="screen-grid screen-enter">
      <GlassCard className="col-span-4" glow="accent" style={{ minHeight: 300 }}>
        <SectionHeader title="PulseScore" subtitle="Understand what matters and what to do next" eyebrow="Home" />
        <div style={{ display: "grid", gridTemplateColumns: "180px 1fr", gap: "var(--sp-4)", alignItems: "center" }}>
          <div style={{ display: "grid", placeItems: "center" }}>
            <HealthRing score={pulseScorePercent} size={176} />
          </div>
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Current system rating</div>
              <div className="font-display" style={{ fontSize: 48, fontWeight: 800, lineHeight: 0.95, letterSpacing: "-0.04em" }}>
                {pulseScore?.total ?? "--"}
              </div>
              <div className="tiny" style={{ color: "var(--text-secondary)", marginTop: 8 }}>
                {summary?.healthSummary ?? "Checking the fastest safe improvements for this PC."}
              </div>
            </div>

            <div className="badge-row">
              <StatusPill status={pulseScoreTone(pulseScore?.total ?? 0)} label={`Grade ${pulseScore?.grade ?? "--"}`} />
              <StatusPill status="info" label={`Tweaks ${pulseScore?.tweaksApplied ?? 0}/${pulseScore?.tweaksAvailable ?? 0}`} />
              <StatusPill status="warn" label={`Bench ${summary?.benchmarkSummary.runs ?? 0}`} />
            </div>

            <div
              className="surface-panel"
              style={{
                display: "grid",
                gridTemplateColumns: "1fr 1fr",
                gap: "var(--sp-2)",
                padding: "var(--sp-3)",
                borderRadius: "var(--r-lg)"
              }}
            >
              <div>
                <div className="tiny">Hardware tier</div>
                <div className="font-mono" style={{ fontSize: 12 }}>{pulseScore?.hardwareTier ?? "--"}</div>
              </div>
              <div>
                <div className="tiny">Optimization</div>
                <div className="font-mono" style={{ fontSize: 12 }}>{pulseScore?.optimizationLevel ?? "--"}</div>
              </div>
              <div>
                <div className="tiny">Health state</div>
                <div className="font-mono" style={{ fontSize: 12 }}>{pulseScore?.healthState ?? "--"}</div>
              </div>
              <div>
                <div className="tiny">Benchmark</div>
                <div className="font-mono" style={{ fontSize: 12 }}>{pulseScore?.benchScore ?? "--"}</div>
              </div>
            </div>

            <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
              <GlowButton label="Open Optimizations" onClick={() => setActiveScreen("optimizations")} />
              <GlowButton label="Run Quick Benchmark" variant="ghost" onClick={() => void run("quick_benchmark")} />
            </div>
          </div>
        </div>
      </GlassCard>

      <div className="col-span-8 metric-grid">
        <MetricCard
          label="CPU"
          value={snapshot.cpuPercent.toFixed(1)}
          unit="%"
          delta={cpuDelta}
          trend={cpuDelta > 0 ? "up" : "down"}
          icon="CPU"
          glowColor={snapshot.cpuPercent > 85 ? "danger" : snapshot.cpuPercent > 65 ? "warn" : "accent"}
          sparkline={history.cpu.slice(-24)}
        />
        <MetricCard
          label="RAM"
          value={snapshot.ramPercent.toFixed(1)}
          unit="%"
          delta={ramDelta}
          trend={ramDelta > 0 ? "up" : "down"}
          icon="RAM"
          glowColor={snapshot.ramPercent > 85 ? "danger" : snapshot.ramPercent > 65 ? "warn" : "accent"}
          sparkline={history.ram.slice(-24)}
        />
        <MetricCard
          label="GPU"
          value={snapshot.gpuPercent.toFixed(1)}
          unit="%"
          delta={percentDelta(snapshot.gpuPercent, history.cpu[history.cpu.length - 8])}
          trend="flat"
          icon="GPU"
          glowColor={snapshot.gpuPercent > 90 ? "danger" : snapshot.gpuPercent > 60 ? "warn" : "ok"}
          sparkline={history.cpu.slice(-24).map((value, index) => Math.min(100, Math.max(0, value * 0.65 + index % 5)))}
        />
        <MetricCard
          label="TEMP"
          value={snapshot.temperatureCelsius.toFixed(1)}
          unit="C"
          delta={tempDelta}
          trend={tempDelta > 0 ? "up" : "down"}
          icon="TMP"
          glowColor={snapshot.temperatureCelsius > 85 ? "danger" : snapshot.temperatureCelsius > 70 ? "warn" : "ok"}
          sparkline={history.cpu.slice(-24).map((value) => Math.min(100, Math.max(20, value * 0.8)))}
        />
        <MetricCard
          label="NETWORK"
          value={netCurrent.toFixed(1)}
          unit="MB/s"
          delta={netDelta}
          trend={netDelta > 0 ? "up" : "down"}
          icon="NET"
          glowColor="accent"
          sparkline={history.net.slice(-24)}
        />
      </div>

      {advisorHighlights.length > 0 ? (
        <GlassCard className="col-span-12" glow="warn">
          <SectionHeader
            title="Advisor Highlights"
            subtitle="The highest-signal improvements detected on this system"
            eyebrow="Recommendations"
            action={<GlowButton label="Open Optimizations" variant="ghost" onClick={() => setActiveScreen("optimizations")} />}
          />
          <div style={{ display: "grid", gap: "var(--sp-3)", gridTemplateColumns: "repeat(2, minmax(0, 1fr))" }}>
            {advisorHighlights.map((item) => (
              <div
                key={item.id}
                className="surface-panel interactive"
                style={{
                  display: "grid",
                  gap: "var(--sp-2)",
                  padding: "var(--sp-4)",
                  borderRadius: "var(--r-lg)"
                }}
              >
                <div className="badge-row">
                  <StatusPill status={item.impact === "high" ? "danger" : item.impact === "medium" ? "warn" : "ok"} label={item.impact} />
                  <StatusPill status="info" label={item.category} />
                </div>
                <div className="font-display" style={{ fontSize: 18, fontWeight: 700 }}>{item.title}</div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>{item.description}</div>
                <div>
                  <GlowButton
                    label={item.actionLabel ?? "Open recommendation"}
                    variant="ghost"
                    onClick={() => setActiveScreen(item.category === "gpu" || item.category === "bios" ? "games" : "optimizations")}
                  />
                </div>
              </div>
            ))}
          </div>
        </GlassCard>
      ) : null}

      <GlassCard className="col-span-8" glow="accent">
        <SectionHeader title="Live Telemetry" subtitle="CPU and RAM pressure over the recent interval" eyebrow="Now" />
        <Suspense fallback={<div className="skeleton" style={{ height: 210 }} />}>
          <LineChart
            data={history.cpu}
            secondaryData={history.ram}
            labels={history.labels}
            color="var(--accent)"
            secondaryColor="var(--accent2)"
          />
        </Suspense>
      </GlassCard>

      <GlassCard className="col-span-4">
        <SectionHeader
          title="Top Processes"
          subtitle="The current busiest apps on the system"
          eyebrow="Load"
          action={<GlowButton label="Advanced Controls" variant="ghost" onClick={() => setActiveScreen("settings")} />}
        />
        <div style={{ display: "flex", flexDirection: "column", gap: "var(--sp-2)" }}>
          {topProcesses.map((process, index) => (
            <div
              key={`${process.pid}-${process.name}`}
              className="surface-panel interactive screen-enter"
              style={{
                display: "grid",
                gridTemplateColumns: "1fr auto auto",
                alignItems: "center",
                gap: "var(--sp-3)",
                padding: "var(--sp-3)",
                borderRadius: "var(--r-lg)",
                animationDelay: `${index * 45}ms`
              }}
            >
              <div style={{ minWidth: 0 }}>
                <div className="font-display nav-item-label" style={{ fontSize: 14, fontWeight: 700 }}>
                  {process.name}
                </div>
                <div style={{ height: 5, borderRadius: 999, background: "rgba(255,255,255,0.06)", marginTop: "var(--sp-2)" }}>
                  <div
                    style={{
                      width: `${Math.min(100, process.cpuPercent)}%`,
                      height: "100%",
                      borderRadius: 999,
                      background:
                        process.cpuPercent > 80
                          ? "var(--status-danger)"
                          : process.cpuPercent > 40
                            ? "var(--status-warn)"
                            : "var(--status-ok)"
                    }}
                  />
                </div>
              </div>
              <span className="font-mono tiny">{process.cpuPercent.toFixed(1)}%</span>
              <GlowButton label="Advanced" variant="ghost" onClick={() => setActiveScreen("settings")} />
            </div>
          ))}
        </div>
      </GlassCard>

      <GlassCard className="col-span-7">
        <SectionHeader title="Quick Actions" subtitle="Start with the most useful flows, not the deepest tools" eyebrow="Optimize now" />
        <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
          <GlowButton label="Clean Junk" onClick={() => void run("clean_junk")} title="~3s" />
          <GlowButton label="Optimize RAM" onClick={() => void run("optimize_ram")} title="~2s" />
          <GlowButton label="Boost-Up" onClick={() => setActiveScreen("boost_up")} title="Prep and cleanup" />
          <GlowButton label="Snapshot" onClick={() => void run("take_snapshot")} title="~1s" />
          <GlowButton label="Optimizations" variant="ghost" onClick={() => setActiveScreen("optimizations")} />
          <GlowButton label="Games" variant="ghost" onClick={() => setActiveScreen("games")} />
        </div>
        {summary?.benchmarkSummary.latest ? (
          <div className="surface-panel" style={{ marginTop: "var(--sp-4)", padding: "var(--sp-4)", borderRadius: "var(--r-lg)" }}>
            <div className="title-kicker" style={{ marginBottom: 8 }}>Latest benchmark</div>
            <div className="font-display" style={{ fontSize: 18, fontWeight: 700, marginBottom: 6 }}>
              Pulse {Math.round(summary.benchmarkSummary.latest.pulseScore)}
            </div>
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>
              CPU {summary.benchmarkSummary.latest.cpuScore.toFixed(1)} | RAM {summary.benchmarkSummary.latest.ramBandwidthMBps.toFixed(1)} MB/s | Disk {summary.benchmarkSummary.latest.diskReadMBps.toFixed(1)} MB/s | Net {summary.benchmarkSummary.latest.networkLatencyMs.toFixed(1)} ms
            </div>
          </div>
        ) : null}
      </GlassCard>

      <GlassCard className="col-span-5">
        <SectionHeader title="Recent Actions" subtitle="Recent optimization work and system changes" eyebrow="Recent activity" />
        <div style={{ display: "flex", flexDirection: "column", gap: "var(--sp-2)", maxHeight: 260, overflowY: "auto" }}>
          {(summary?.recentActions ?? []).length === 0 ? (
            <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)" }}>
              <span className="tiny">No persisted actions recorded yet.</span>
            </div>
          ) : (summary?.recentActions ?? []).map((entry, index) => (
            <div
              key={`${entry.timestamp}-${entry.action}`}
              className="surface-panel screen-enter"
              style={{
                padding: "var(--sp-3)",
                borderRadius: "var(--r-lg)",
                display: "flex",
                alignItems: "center",
                justifyContent: "space-between",
                gap: "var(--sp-3)",
                animationDelay: `${index * 40}ms`
              }}
            >
              <div style={{ minWidth: 0 }}>
                <div className="font-display nav-item-label" style={{ fontSize: 14, fontWeight: 700 }}>
                  {entry.details || entry.action}
                </div>
                <div className="tiny">{entry.action}</div>
              </div>
              <div style={{ display: "grid", justifyItems: "end", gap: 4, flexShrink: 0 }}>
                <StatusPill status={entry.success ? "ok" : "danger"} label={entry.success ? "ok" : "fail"} />
                <span className="font-mono tiny">{relativeTime(entry.timestamp)}</span>
              </div>
            </div>
          ))}
        </div>
      </GlassCard>

      <GlassCard className="col-span-12">
        <SectionHeader
          title="System Health"
          subtitle="Current telemetry summary and issue context"
          eyebrow="Current state"
          action={<StatusPill status={snapshot.healthScore >= 75 ? "ok" : snapshot.healthScore >= 45 ? "warn" : "danger"} label={healthLabel(snapshot.healthScore)} />}
        />
        <div className="tiny" style={{ color: "var(--text-secondary)", fontSize: 12 }}>{snapshot.healthSummary}</div>
      </GlassCard>
    </div>
  );
}
