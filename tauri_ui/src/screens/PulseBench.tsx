import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import HealthRing from "../components/ui/HealthRing";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { BenchmarkDelta, BenchmarkResult } from "../types/system";

type BenchmarkHistoryPayload = {
  ok: boolean;
  history: BenchmarkResult[];
  latestDelta?: BenchmarkDelta;
};

function gradeStatus(grade: BenchmarkResult["grade"]): "ok" | "warn" | "danger" | "info" {
  if (grade === "S" || grade === "A") {
    return "ok";
  }
  if (grade === "B" || grade === "C") {
    return "warn";
  }
  return "danger";
}

function formatDelta(value: number): string {
  return `${value >= 0 ? "+" : ""}${value.toFixed(1)}%`;
}

export default function PulseBench() {
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const [historyPayload, setHistoryPayload] = useState<BenchmarkHistoryPayload>({ ok: true, history: [] });
  const [runningMode, setRunningMode] = useState<"quick" | "full" | null>(null);

  const loadHistory = async () => {
    try {
      const result = await tauriInvoke<BenchmarkHistoryPayload>("get_benchmark_history");
      setHistoryPayload(result);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load benchmark history", "danger");
    }
  };

  useEffect(() => {
    void loadHistory();
  }, []);

  const latest = historyPayload.history.length ? historyPayload.history[historyPayload.history.length - 1] : null;
  const best = useMemo(() => {
    return [...historyPayload.history].sort((left, right) => right.compositeScore - left.compositeScore)[0] ?? null;
  }, [historyPayload.history]);

  const runBenchmark = async (mode: "quick" | "full") => {
    setRunningMode(mode);
    try {
      const result = await tauriInvoke<BenchmarkResult>(mode === "quick" ? "quick_benchmark" : "full_benchmark");
      pushNotification(
        `${mode === "quick" ? "Quick" : "Full"} benchmark complete - PulseScore ${Math.round(result.pulseScore)}`,
        "ok"
      );
      await loadHistory();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Benchmark failed", "danger");
    } finally {
      setRunningMode(null);
    }
  };

  return (
    <div style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow="accent">
        <SectionHeader
          title="PulseBench"
          subtitle="Run local before-and-after benchmarks and make performance gains visible"
          action={latest ? <StatusPill status={gradeStatus(latest.grade)} label={`Grade ${latest.grade}`} /> : null}
        />
        <div style={{ display: "grid", gridTemplateColumns: "220px minmax(0, 1fr)", gap: "var(--sp-4)", alignItems: "center" }}>
          <div style={{ display: "grid", placeItems: "center" }}>
            <HealthRing score={latest ? Math.min(100, Math.round(latest.pulseScore / 10)) : 0} />
            <div className="title-kicker" style={{ marginTop: "var(--sp-2)" }}>Current PulseScore</div>
            <div className="font-display" style={{ fontSize: 34, fontWeight: 800, marginTop: 4 }}>
              {latest ? Math.round(latest.pulseScore) : "--"}
            </div>
          </div>

          <div style={{ display: "grid", gap: "var(--sp-4)" }}>
            <div>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Benchmarking modes</div>
              <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.05, marginBottom: 10 }}>
                Capture proof before and after you optimize.
              </div>
              <div className="muted">
                CPU, RAM, disk, and network are measured locally and written to history so performance changes are visible instead of implied.
              </div>
            </div>

            <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
              <GlowButton label="Quick Benchmark" onClick={() => void runBenchmark("quick")} loading={runningMode === "quick"} />
              <GlowButton label="Full Benchmark" variant="ghost" onClick={() => void runBenchmark("full")} loading={runningMode === "full"} />
              <GlowButton label="Refresh History" variant="ghost" onClick={() => void loadHistory()} />
            </div>

            {historyPayload.latestDelta ? (
              <GlassCard glow={historyPayload.latestDelta.percentChange >= 0 ? "ok" : "danger"}>
                <div className="title-kicker" style={{ marginBottom: 8 }}>Latest change</div>
                <div className="font-display" style={{ fontSize: 22, fontWeight: 800, marginBottom: 8 }}>
                  {formatDelta(historyPayload.latestDelta.percentChange)} composite delta
                </div>
                <div className="tiny">
                  Score delta {historyPayload.latestDelta.scoreDelta.toFixed(1)} between the last two recorded runs.
                </div>
              </GlassCard>
            ) : null}
          </div>
        </div>
      </GlassCard>

      <div style={{ display: "grid", gridTemplateColumns: "repeat(2, minmax(0, 1fr))", gap: "var(--sp-4)" }}>
        {[{ title: "Latest Benchmark", value: latest }, { title: "Best Benchmark", value: best }].map((entry) => (
          <GlassCard key={entry.title}>
            <SectionHeader title={entry.title} subtitle={entry.value ? new Date(entry.value.timestamp).toLocaleString() : "No benchmark yet"} />
            {entry.value ? (
              <div style={{ display: "grid", gap: "var(--sp-3)" }}>
                <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                  <div>CPU Score: <span className="font-mono">{entry.value.cpuScore.toFixed(1)}</span></div>
                  <div>RAM Bandwidth: <span className="font-mono">{entry.value.ramBandwidthMBps.toFixed(1)} MB/s</span></div>
                  <div>Disk Read: <span className="font-mono">{entry.value.diskReadMBps.toFixed(1)} MB/s</span></div>
                  <div>Network Latency: <span className="font-mono">{entry.value.networkLatencyMs.toFixed(1)} ms</span></div>
                  <div>Composite: <span className="font-mono">{entry.value.compositeScore.toFixed(1)}</span></div>
                </div>
                <div className="badge-row">
                  <StatusPill status={gradeStatus(entry.value.grade)} label={`Grade ${entry.value.grade}`} />
                  <StatusPill status="info" label={`Pulse ${Math.round(entry.value.pulseScore)}`} />
                </div>
              </div>
            ) : (
              <div className="tiny" style={{ color: "var(--text-secondary)" }}>Run a benchmark to populate this card.</div>
            )}
          </GlassCard>
        ))}
      </div>

      <GlassCard>
        <SectionHeader title="Benchmark History" subtitle={`${historyPayload.history.length} stored runs`} />
        <div style={{ display: "grid", gap: "var(--sp-2)" }}>
          {historyPayload.history.slice().reverse().map((entry, index) => (
            <div
              key={entry.timestamp}
              className="surface-panel screen-enter"
              style={{
                display: "grid",
                gridTemplateColumns: "180px repeat(5, minmax(0, 1fr))",
                gap: "var(--sp-3)",
                padding: "var(--sp-3)",
                borderRadius: "var(--r-lg)",
                animationDelay: `${Math.min(index, 8) * 35}ms`
              }}
            >
              <div className="tiny">{new Date(entry.timestamp).toLocaleString()}</div>
              <div className="font-mono tiny">CPU {entry.cpuScore.toFixed(1)}</div>
              <div className="font-mono tiny">RAM {entry.ramBandwidthMBps.toFixed(1)}</div>
              <div className="font-mono tiny">Disk {entry.diskReadMBps.toFixed(1)}</div>
              <div className="font-mono tiny">Net {entry.networkLatencyMs.toFixed(1)} ms</div>
              <div className="font-mono tiny">Pulse {Math.round(entry.pulseScore)}</div>
            </div>
          ))}
          {historyPayload.history.length === 0 ? (
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>No benchmark history yet.</div>
          ) : null}
        </div>
      </GlassCard>
    </div>
  );
}

