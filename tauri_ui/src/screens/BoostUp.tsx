import { useEffect, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { BoostUpAction, NetworkDiagnosticsPayload, RamBreakdown } from "../types/system";

type BoostPayload = {
  ok: boolean;
  actions: BoostUpAction[];
};

export default function BoostUp() {
  const snapshot = useSystemStore((s) => s.snapshot);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const lastProof = useSystemStore((s) => s.lastProof);
  const [actions, setActions] = useState<BoostUpAction[]>([]);
  const [network, setNetwork] = useState<NetworkDiagnosticsPayload | null>(null);
  const [ram, setRam] = useState<RamBreakdown | null>(null);
  const [junkBytes, setJunkBytes] = useState(0);
  const [activeAction, setActiveAction] = useState<string | null>(null);
  const [detailsLoading, setDetailsLoading] = useState(true);

  const loadCore = async () => {
    try {
      const boost = await tauriInvoke<BoostPayload>("get_boost_up_actions");
      setActions(boost.actions ?? []);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load Boost-Up actions", "danger");
    }
  };

  const loadDetails = async () => {
    setDetailsLoading(true);
    try {
      const [networkPayload, ramPayload, junkPayload] = await Promise.all([
        tauriInvoke<NetworkDiagnosticsPayload>("get_network_diagnostics"),
        tauriInvoke<RamBreakdown>("get_ram_breakdown"),
        tauriInvoke<{ ok: boolean; bytes: number }>("estimate_junk")
      ]);
      setNetwork(networkPayload);
      setRam(ramPayload);
      setJunkBytes(junkPayload.bytes ?? 0);
    } catch {
      setNetwork(null);
      setRam(null);
      setJunkBytes(0);
    } finally {
      setDetailsLoading(false);
    }
  };

  const load = async () => {
    await loadCore();
    void loadDetails();
  };

  useEffect(() => {
    void load();
  }, [pushNotification]);

  const runAction = async (id: string, label: string) => {
    setActiveAction(id);
    try {
      await tauriInvoke("run_boost_up_action", { actionId: id });
      pushNotification(`${label} complete`, "ok");
      await load();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : `${label} failed`, "danger");
    } finally {
      setActiveAction(null);
    }
  };

  return (
    <div style={{ display: "grid", gap: "var(--sp-4)" }} className="screen-enter">
      <GlassCard glow="accent">
        <SectionHeader
          title="Boost-Up"
          subtitle="Prep the system before play, clear soft bottlenecks, and keep the path focused on immediate gains."
          eyebrow="Maintenance and prep"
          action={<GlowButton label="Refresh" variant="ghost" onClick={() => void load()} />}
        />
        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 360px", gap: "var(--sp-4)", alignItems: "stretch" }}>
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Pre-session workflow</div>
              <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.04, marginBottom: 10 }}>
                Clean up pressure before you launch a game or start heavy work.
              </div>
              <div className="muted">
                This page consolidates the temporary actions that should feel immediate instead of burying them across separate utility pages.
              </div>
            </div>
            <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
              <GlowButton label="Pre-Game Clean" onClick={() => void runAction("pre_game_clean", "Pre-Game Clean")} loading={activeAction === "pre_game_clean"} />
              <GlowButton label="Optimize RAM" variant="ghost" onClick={() => void runAction("optimize_ram", "Optimize RAM")} loading={activeAction === "optimize_ram"} />
              <GlowButton label="Refresh Network" variant="ghost" onClick={() => void runAction("network_reset", "Refresh Network")} loading={activeAction === "network_reset"} />
            </div>
          </div>

          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
            {detailsLoading ? (
              <div className="skeleton" style={{ height: 128 }} />
            ) : (
              <>
                <div className="badge-row">
                  <StatusPill status="info" label={`${(junkBytes / (1024 * 1024)).toFixed(0)} MB junk`} />
                  <StatusPill status={snapshot && snapshot.ramPercent > 80 ? "warn" : "ok"} label={`RAM ${snapshot?.ramPercent.toFixed(0) ?? "--"}%`} />
                  <StatusPill status={snapshot && snapshot.temperatureCelsius > 80 ? "danger" : "ok"} label={`Temp ${snapshot?.temperatureCelsius.toFixed(0) ?? "--"}C`} />
                </div>
                <div>
                  <div className="title-kicker" style={{ marginBottom: 8 }}>Current prep state</div>
                  <div className="font-display" style={{ fontSize: 20, fontWeight: 800, lineHeight: 1.05, marginBottom: 8 }}>
                    {network?.connectionType ?? "Network pending"} | {network?.adapterName ?? "adapter pending"}
                  </div>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>
                    Recoverable memory {ram?.recoverableMb?.toFixed(0) ?? "--"} MB | DNS {network?.dnsServers?.[0] ?? "pending"}
                  </div>
                </div>
              </>
            )}
          </div>
        </div>
      </GlassCard>

      {lastProof ? (
        <GlassCard glow="ok">
          <SectionHeader title="Proof of Work" subtitle="Concrete changes from the last completed action" eyebrow="What changed" />
          <div style={{ display: "grid", gap: "var(--sp-2)" }}>
            <StatusPill status="ok" label={lastProof.action.replace(/_/g, " ")} />
            {lastProof.lines.map((line) => (
              <div key={line} className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", color: "var(--text-secondary)" }}>
                {line}
              </div>
            ))}
          </div>
        </GlassCard>
      ) : null}

      <div style={{ display: "grid", gridTemplateColumns: "repeat(2, minmax(0, 1fr))", gap: "var(--sp-4)" }}>
        {actions.map((action, index) => (
          <GlassCard
            key={action.id}
            glow={action.tone === "accent" ? "accent" : action.tone === "warn" ? "warn" : action.tone === "danger" ? "danger" : "none"}
            className="screen-enter"
            style={{ animationDelay: `${Math.min(index, 8) * 35}ms` }}
          >
            <div style={{ display: "grid", gap: "var(--sp-3)" }}>
              <div className="badge-row">
                <StatusPill status={action.tone === "accent" ? "info" : action.tone === "warn" ? "warn" : action.tone === "danger" ? "danger" : "ok"} label={action.duration} />
              </div>
              <div>
                <div className="font-display" style={{ fontSize: 22, fontWeight: 800, marginBottom: 8 }}>{action.label}</div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>{action.description}</div>
              </div>
              <div>
                <GlowButton label={action.label} onClick={() => void runAction(action.id, action.label)} loading={activeAction === action.id} />
              </div>
            </div>
          </GlassCard>
        ))}
      </div>
    </div>
  );
}
