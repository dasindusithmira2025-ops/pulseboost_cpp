import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import LineChart from "../components/ui/LineChart";
import MetricCard from "../components/ui/MetricCard";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { mbToGb } from "../lib/format";
import { useCommands } from "../hooks/useCommands";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { RamBreakdown } from "../types/system";

const fallbackBreakdown: RamBreakdown = {
  ok: true,
  totalMb: 1,
  usedMb: 0,
  freeMb: 1,
  appsMb: 0,
  systemMb: 0,
  cachedMb: 0,
  recoverableMb: 256,
};

export default function RamOptimizer() {
  const snapshot = useSystemStore((s) => s.snapshot);
  const history = useSystemStore((s) => s.history);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const { run } = useCommands();
  const [breakdown, setBreakdown] = useState<RamBreakdown>(fallbackBreakdown);
  const [isLoadingBreakdown, setIsLoadingBreakdown] = useState(false);

  const loadBreakdown = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setIsLoadingBreakdown(true);
    try {
      const result = await tauriInvoke<RamBreakdown>("get_ram_breakdown");
      setBreakdown(result);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load RAM breakdown", "danger");
    } finally {
      setIsLoadingBreakdown(false);
    }
  };

  useEffect(() => {
    void loadBreakdown();
  }, [snapshot?.timestamp]);

  const total = breakdown.totalMb || snapshot?.ramTotalMb || 1;
  const used = breakdown.usedMb || snapshot?.ramUsedMb || 0;
  const free = breakdown.freeMb || Math.max(0, total - used);
  const recoverable = breakdown.recoverableMb || 256;

  const segments = useMemo(
    () => [
      ["System", breakdown.systemMb, "var(--status-warn)"],
      ["Apps", breakdown.appsMb, "var(--accent)"],
      ["Cached", breakdown.cachedMb, "var(--accent2)"],
      ["Free", free, "var(--status-ok)"]
    ] as const,
    [breakdown.appsMb, breakdown.cachedMb, breakdown.systemMb, free]
  );

  return (
    <div className="screen-enter" style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow="accent">
        <SectionHeader
          title="RAM Optimizer"
          subtitle={`Recoverable memory now: ${recoverable.toFixed(0)} MB`}
          action={<GlowButton label="Refresh Memory" variant="ghost" onClick={() => void loadBreakdown()} loading={isLoadingBreakdown} />}
        />
        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 320px", gap: "var(--sp-4)" }}>
          <div>
            <div className="title-kicker" style={{ marginBottom: 8 }}>Memory pressure</div>
            <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.05, marginBottom: 10 }}>
              Reclaim pressure and inspect real memory segments before you trim working sets.
            </div>
            <div className="muted">
              This view now uses live kernel cache and physical memory statistics from the backend instead of inferred percentages.
            </div>
          </div>
          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
            <StatusPill status={(snapshot?.ramPercent ?? 0) > 85 ? "danger" : (snapshot?.ramPercent ?? 0) > 70 ? "warn" : "ok"} label={`Pressure ${(snapshot?.ramPercent ?? 0).toFixed(1)}%`} />
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>
              Total {mbToGb(total).toFixed(2)} GB | Used {mbToGb(used).toFixed(2)} GB | Free {mbToGb(free).toFixed(2)} GB
            </div>
          </div>
        </div>
      </GlassCard>

      <div className="metric-grid">
        <MetricCard label="Used" value={mbToGb(used).toFixed(2)} unit="GB" icon="USED" glowColor="warn" sparkline={history.ram.slice(-30)} />
        <MetricCard label="Free" value={mbToGb(free).toFixed(2)} unit="GB" icon="FREE" glowColor="ok" sparkline={history.ram.slice(-30).map((v) => 100 - v)} />
        <MetricCard label="Pressure" value={(snapshot?.ramPercent ?? 0).toFixed(1)} unit="%" icon="LOAD" glowColor={(snapshot?.ramPercent ?? 0) > 85 ? "danger" : "accent"} sparkline={history.ram.slice(-30)} />
        <MetricCard label="Recoverable" value={recoverable.toFixed(0)} unit="MB" icon="GAIN" glowColor="accent" sparkline={history.ram.slice(-30).map((v) => Math.max(5, 110 - v))} />
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "340px minmax(0, 1fr)", gap: "var(--sp-4)" }}>
        <GlassCard>
          <SectionHeader title="RAM Breakdown" subtitle="Live memory segmentation" />
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            {segments.map(([name, value, color]) => (
              <div key={name}>
                <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", marginBottom: 6 }}>
                  <span>{name}</span>
                  <span className="font-mono tiny">{mbToGb(value).toFixed(2)} GB</span>
                </div>
                <div style={{ height: 8, borderRadius: 999, background: "rgba(255,255,255,0.06)" }}>
                  <div style={{ width: `${Math.min(100, (value / total) * 100)}%`, height: "100%", borderRadius: 999, background: color }} />
                </div>
              </div>
            ))}
          </div>
        </GlassCard>

        <GlassCard glow="accent">
          <SectionHeader title="RAM Timeline" subtitle="Recent memory pressure trend" />
          <LineChart data={history.ram} labels={history.labels} color="var(--accent2)" />
        </GlassCard>
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 320px", gap: "var(--sp-4)" }}>
        <GlassCard>
          <SectionHeader title="Top Consumers" subtitle="Processes consuming the most memory right now" />
          <div style={{ display: "grid", gap: "var(--sp-2)" }}>
            {(snapshot?.processes ?? []).slice(0, 5).map((process, index) => (
              <div key={`${process.pid}-${process.name}`} className="surface-panel screen-enter" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", alignItems: "center", animationDelay: `${Math.min(index, 8) * 35}ms` }}>
                <div style={{ minWidth: 0 }}>
                  <div className="font-display" style={{ fontSize: 15, fontWeight: 700 }}>{process.name}</div>
                  <div className="tiny" style={{ color: "var(--text-tertiary)" }}>{process.pid}</div>
                </div>
                <span className="font-mono tiny">{process.ramMb.toFixed(0)} MB</span>
              </div>
            ))}
          </div>
        </GlassCard>

        <GlassCard>
          <SectionHeader title="Actions" subtitle="Direct memory maintenance commands" />
          <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
            <GlowButton label="Deep Clean RAM" onClick={() => void run("optimize_ram")} />
            <GlowButton label="Flush Standby List" variant="ghost" onClick={() => void run("flush_standby_list")} />
            <GlowButton label="Enable RAM Saver Mode" variant="ghost" onClick={() => void run("enable_ram_saver")} />
          </div>
        </GlassCard>
      </div>
    </div>
  );
}

