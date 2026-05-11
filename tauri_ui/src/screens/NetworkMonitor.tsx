import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import LineChart from "../components/ui/LineChart";
import MetricCard from "../components/ui/MetricCard";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { useCommands } from "../hooks/useCommands";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { NetworkDiagnosticsPayload, NetworkPingResult } from "../types/system";

function latencyTone(latencyMs: number): "ok" | "warn" | "danger" | "neutral" {
  if (latencyMs < 0) {
    return "neutral";
  }
  if (latencyMs <= 35) {
    return "ok";
  }
  if (latencyMs <= 80) {
    return "warn";
  }
  return "danger";
}

function pingTone(status: NetworkPingResult["status"]): "ok" | "warn" | "danger" | "neutral" {
  if (status === "ok") {
    return "ok";
  }
  if (status === "slow") {
    return "warn";
  }
  return "danger";
}

export default function NetworkMonitor() {
  const snapshot = useSystemStore((s) => s.snapshot);
  const history = useSystemStore((s) => s.history);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const { run } = useCommands();
  const [diagnostics, setDiagnostics] = useState<NetworkDiagnosticsPayload | null>(null);
  const [isLoadingDiagnostics, setIsLoadingDiagnostics] = useState(false);

  const loadDiagnostics = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setIsLoadingDiagnostics(true);
    try {
      const result = await tauriInvoke<NetworkDiagnosticsPayload>("get_network_diagnostics");
      setDiagnostics(result);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load network diagnostics", "danger");
    } finally {
      setIsLoadingDiagnostics(false);
    }
  };

  useEffect(() => {
    void loadDiagnostics();
    const timer = window.setInterval(() => {
      void loadDiagnostics();
    }, 30000);
    return () => window.clearInterval(timer);
  }, []);

  const primaryPing = useMemo(
    () => diagnostics?.pings.find((item) => item.name === "Cloudflare") ?? diagnostics?.pings[0] ?? null,
    [diagnostics]
  );

  const latencyMs = primaryPing?.latencyMs ?? -1;
  const downloadMbps = (snapshot?.netDownloadKbps ?? 0) / 1024;
  const uploadMbps = (snapshot?.netUploadKbps ?? 0) / 1024;
  const packetRate = useMemo(() => {
    const totalKbps = (snapshot?.netDownloadKbps ?? 0) + (snapshot?.netUploadKbps ?? 0);
    return Math.max(0, Math.round(totalKbps / 6));
  }, [snapshot?.netDownloadKbps, snapshot?.netUploadKbps]);

  return (
    <div className="screen-enter" style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow="accent">
        <SectionHeader
          title="Network Monitor"
          subtitle="Live throughput, DNS context, adapter telemetry, and server latency"
          action={<GlowButton label="Refresh Network" variant="ghost" onClick={() => void loadDiagnostics()} loading={isLoadingDiagnostics} />}
        />
        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 340px", gap: "var(--sp-4)" }}>
          <div>
            <div className="title-kicker" style={{ marginBottom: 8 }}>Connection state</div>
            <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.05, marginBottom: 10 }}>
              Track live traffic and verify the actual network path before applying tweaks.
            </div>
            <div className="muted">
              PulseBoost now reads the active adapter, DNS servers, and game-service latency instead of showing inferred placeholders.
            </div>
          </div>
          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
            <div className="badge-row">
              <StatusPill status={latencyTone(latencyMs)} label={latencyMs >= 0 ? `Latency ${latencyMs} ms` : "Latency unavailable"} />
              <StatusPill status="info" label={diagnostics?.connectionType || "Disconnected"} />
            </div>
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>
              {diagnostics?.adapterName || "No active adapter detected"}
            </div>
            <div className="tiny" style={{ color: "var(--text-tertiary)" }}>
              {diagnostics?.adapterDescription || "Network adapter details will appear here when connected."}
            </div>
          </div>
        </div>
      </GlassCard>

      <div className="metric-grid">
        <MetricCard label="Download" value={downloadMbps.toFixed(2)} unit="MB/s" icon="DL" glowColor="accent" sparkline={history.net.slice(-30)} />
        <MetricCard label="Upload" value={uploadMbps.toFixed(2)} unit="MB/s" icon="UL" glowColor="accent" sparkline={history.net.slice(-30).map((v) => v * 0.25)} />
        <MetricCard label="Latency" value={latencyMs >= 0 ? latencyMs.toFixed(0) : "--"} unit="ms" icon="LT" glowColor={latencyMs > 80 ? "warn" : "ok"} sparkline={diagnostics?.pings.map((item) => Math.max(item.latencyMs, 0)) ?? history.net.slice(-30).map(() => 0)} />
        <MetricCard label="Packets" value={packetRate.toFixed(0)} unit="/s" icon="PK" glowColor="accent" sparkline={history.net.slice(-30).map((v) => Math.round(v * 12))} />
      </div>

      <GlassCard glow="accent">
        <SectionHeader title="Bandwidth History" subtitle="Recent throughput trend" />
        <LineChart data={history.net} labels={history.labels} color="var(--accent)" secondaryData={history.net.map((v) => v * 0.25)} secondaryColor="var(--accent2)" />
      </GlassCard>

      <div style={{ display: "grid", gridTemplateColumns: "1.1fr 0.9fr", gap: "var(--sp-4)" }}>
        <GlassCard>
          <SectionHeader title="Adapter and DNS" subtitle="Actual network path context" />
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)" }}>
              <div className="tiny">Adapter</div>
              <div className="font-mono">{diagnostics?.adapterName || "Unavailable"}</div>
            </div>
            <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)" }}>
              <div className="tiny">DNS Servers</div>
              <div className="font-mono" style={{ display: "grid", gap: 4 }}>
                {(diagnostics?.dnsServers.length ?? 0) > 0 ? diagnostics?.dnsServers.map((server) => <span key={server}>{server}</span>) : <span>System default / unavailable</span>}
              </div>
            </div>
            <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)" }}>
              <div className="tiny">DNS Suffix</div>
              <div className="font-mono">{diagnostics?.dnsSuffix || "None"}</div>
            </div>
          </div>
        </GlassCard>

        <GlassCard>
          <SectionHeader title="Game Server Ping" subtitle="Polled every 30 seconds" />
          <div style={{ display: "grid", gap: "var(--sp-2)" }}>
            {(diagnostics?.pings ?? []).map((ping, index) => (
              <div key={`${ping.name}-${ping.host}`} className="surface-panel screen-enter" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", alignItems: "center", animationDelay: `${Math.min(index, 8) * 35}ms` }}>
                <div style={{ minWidth: 0 }}>
                  <div className="font-display" style={{ fontSize: 15, fontWeight: 700 }}>{ping.name}</div>
                  <div className="tiny" style={{ color: "var(--text-tertiary)" }}>{ping.host}</div>
                </div>
                <StatusPill status={pingTone(ping.status)} label={ping.latencyMs >= 0 ? `${ping.latencyMs} ms` : "offline"} dot />
              </div>
            ))}
          </div>
        </GlassCard>
      </div>

      <GlassCard>
        <SectionHeader title="Quick Actions" subtitle="Apply stack-level maintenance commands" />
        <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
          <GlowButton label="Flush DNS" onClick={() => void run("flush_dns")} />
          <GlowButton label="Optimize TCP" onClick={() => void run("optimize_tcp")} />
          <GlowButton label="Re-check Diagnostics" variant="ghost" onClick={() => void loadDiagnostics()} loading={isLoadingDiagnostics} />
        </div>
      </GlassCard>
    </div>
  );
}

