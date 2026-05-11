import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { GameModeStatus, GameProfile, SystemAdvisorItem } from "../types/system";

type GamePayload = {
  ok: boolean;
  games: GameProfile[];
};

type GameActionPayload = {
  ok: boolean;
  launched?: boolean;
  pending?: boolean;
  pid?: number;
  name?: string;
  executableName?: string;
  reason?: string;
};

const launcherLabel: Record<GameProfile["launcher"], string> = {
  steam: "Steam",
  epic: "Epic Games",
  battlenet: "Battle.net",
  direct: "Direct",
  other: "Other"
};

function stateTone(game: GameProfile): "accent" | "ok" | "warn" | "none" {
  if (game.isRunning) {
    return "accent";
  }
  if (game.isOptimized) {
    return "ok";
  }
  if (game.isDetected) {
    return "warn";
  }
  return "none";
}

export default function GameOptimizer() {
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const snapshot = useSystemStore((s) => s.snapshot);
  const [games, setGames] = useState<GameProfile[]>([]);
  const [status, setStatus] = useState<GameModeStatus | null>(null);
  const [advisorItems, setAdvisorItems] = useState<SystemAdvisorItem[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [advisorLoading, setAdvisorLoading] = useState(true);
  const [activeAction, setActiveAction] = useState<string | null>(null);

  const loadCore = async () => {
    setIsLoading(true);
    try {
      const [result, modeStatus] = await Promise.all([
        tauriInvoke<GamePayload>("detect_games"),
        tauriInvoke<GameModeStatus>("get_game_mode_status")
      ]);
      setGames(result.games ?? []);
      setStatus(modeStatus);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to detect games", "danger");
    } finally {
      setIsLoading(false);
    }
  };

  const loadAdvisor = async () => {
    setAdvisorLoading(true);
    try {
      const advisor = await tauriInvoke<{ ok: boolean; items: SystemAdvisorItem[] }>("get_system_advisor_summary");
      setAdvisorItems((advisor.items ?? []).filter((item) => item.category === "gpu" || item.category === "bios" || item.category === "windows").slice(0, 3));
    } catch {
      setAdvisorItems([]);
    } finally {
      setAdvisorLoading(false);
    }
  };

  const loadGames = async () => {
    await loadCore();
    void loadAdvisor();
  };

  useEffect(() => {
    void loadGames();
  }, []);

  const performAction = async (actionKey: string, run: () => Promise<GameActionPayload | boolean>, successMessage: string) => {
    setActiveAction(actionKey);
    try {
      const result = await run();
      if (typeof result === "object" && result && result.ok === false) {
        throw new Error(result.reason ?? "Game action failed");
      }
      if (typeof result === "object" && result && result.pending) {
        pushNotification("Game launched. Waiting for the process to settle before full optimization.", "warn");
      } else {
        pushNotification(successMessage, "ok");
      }
      await loadGames();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Game action failed", "danger");
    } finally {
      setActiveAction(null);
    }
  };

  const runningCount = useMemo(() => games.filter((game) => game.isRunning).length, [games]);
  const optimizedCount = useMemo(() => games.filter((game) => game.isOptimized).length, [games]);
  const detectedCount = useMemo(() => games.filter((game) => game.isDetected).length, [games]);
  const visibleGames = useMemo(() => games.slice(0, 12), [games]);
  const activeGame = useMemo(
    () => games.find((game) => game.isOptimized) ?? games.find((game) => status?.pid && game.runningPid === status.pid),
    [games, status?.pid]
  );

  return (
    <div style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow="accent">
        <SectionHeader
          title="Gaming"
          subtitle="Launch-time optimization, measured session control, and the key signals that affect play."
          eyebrow="Improve game performance now"
          action={<GlowButton label="Scan Again" variant="ghost" onClick={() => void loadGames()} loading={isLoading} />}
        />
        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 360px", gap: "var(--sp-4)", alignItems: "stretch" }}>
          <div style={{ display: "grid", gap: "var(--sp-4)" }}>
            <div>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Per-game execution</div>
              <div className="font-display" style={{ fontSize: 30, fontWeight: 800, lineHeight: 1.02, marginBottom: 12 }}>
                Improve game performance without leaving the gaming workflow.
              </div>
              <div className="muted">
                Detected installs, launch-time optimization, and reversible session controls now stay in one gaming-first workflow.
              </div>
            </div>
            <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
              <GlowButton
                label="Revert Active Session"
                variant="ghost"
                onClick={() => void performAction("revert-session", () => tauriInvoke<boolean>("revert_game_optimization"), "Game optimizations reverted")}
                loading={activeAction === "revert-session"}
                disabled={!status?.active}
              />
            </div>
          </div>

          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
            <div className="badge-row">
              <StatusPill status="info" label={`${detectedCount} detected`} />
              <StatusPill status="ok" label={`${optimizedCount} optimized`} />
              <StatusPill status={runningCount > 0 ? "warn" : "neutral"} label={`${runningCount} running`} />
            </div>
            <div style={{ display: "grid", gap: 6 }}>
              <div className="title-kicker">Live gaming state</div>
              <div className="font-display" style={{ fontSize: 20, fontWeight: 800, lineHeight: 1.05 }}>
                {status?.active ? (activeGame?.displayName ?? status.name ?? "Optimized game session") : "No active optimized game"}
              </div>
              <div className="tiny" style={{ color: "var(--text-secondary)" }}>
                {status?.active
                  ? `PID ${status.pid}${status.executableName ? ` - ${status.executableName}` : ""}`
                  : "Launch a detected game or optimize a running title to create an active session."}
              </div>
            </div>
          </div>
        </div>
      </GlassCard>

      <div className="metric-grid">
        <GlassCard>
          <div className="title-kicker" style={{ marginBottom: 8 }}>Thermals</div>
          <div className="font-display" style={{ fontSize: 24, fontWeight: 800 }}>{snapshot?.temperatureCelsius.toFixed(1) ?? "--"} C</div>
          <div className="tiny">Confirmed package temperature during play readiness</div>
        </GlassCard>
        <GlassCard>
          <div className="title-kicker" style={{ marginBottom: 8 }}>Memory</div>
          <div className="font-display" style={{ fontSize: 24, fontWeight: 800 }}>{snapshot?.ramPercent.toFixed(1) ?? "--"}%</div>
          <div className="tiny">Current RAM pressure before game launch</div>
        </GlassCard>
        <GlassCard>
          <div className="title-kicker" style={{ marginBottom: 8 }}>Network</div>
          <div className="font-display" style={{ fontSize: 24, fontWeight: 800 }}>{((snapshot?.netDownloadKbps ?? 0) / 1024).toFixed(1)} MB/s</div>
          <div className="tiny">Live throughput available to active sessions</div>
        </GlassCard>
      </div>

      {advisorLoading ? (
        <GlassCard>
          <div className="skeleton" style={{ height: 140 }} />
        </GlassCard>
      ) : null}

      {advisorItems.length > 0 ? (
        <GlassCard glow="warn">
          <SectionHeader title="Gaming Recommendations" subtitle="Advisor findings that matter to performance-sensitive sessions" eyebrow="Recommendations" />
          <div style={{ display: "grid", gridTemplateColumns: "repeat(3, minmax(0, 1fr))", gap: "var(--sp-3)" }}>
            {advisorItems.map((item) => (
              <div key={item.id} className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                <div className="badge-row">
                  <StatusPill status={item.impact === "high" ? "danger" : item.impact === "medium" ? "warn" : "ok"} label={item.impact} />
                  <StatusPill status="info" label={item.category} />
                </div>
                <div className="font-display" style={{ fontSize: 17, fontWeight: 800 }}>{item.title}</div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>{item.description}</div>
              </div>
            ))}
          </div>
        </GlassCard>
      ) : null}

      <div style={{ display: "grid", gap: "var(--sp-3)" }}>
        {visibleGames.map((game, index) => {
          const optimizeKey = `optimize-${game.executableName}`;
          const launchKey = `launch-${game.executableName}`;
          const revertKey = `revert-${game.executableName}`;
          const isActiveTarget = status?.active && status.executableName === game.executableName;

          return (
            <GlassCard
              key={`${game.displayName}-${game.installPath}-${game.runningPid ?? 0}`}
              glow={isActiveTarget ? "accent" : stateTone(game)}
              className="screen-enter"
              style={{ animationDelay: `${Math.min(index, 8) * 35}ms` }}
            >
              <div style={{ display: "grid", gap: "var(--sp-3)" }}>
                <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-4)", alignItems: "start" }}>
                  <div style={{ display: "grid", gap: "var(--sp-2)", minWidth: 0 }}>
                    <div className="badge-row">
                      <StatusPill
                        status={isActiveTarget || game.isRunning ? "info" : game.isOptimized ? "ok" : game.isDetected ? "warn" : "neutral"}
                        label={isActiveTarget ? "active session" : game.isRunning ? "running" : game.isOptimized ? "optimized" : "detected"}
                        dot={isActiveTarget || game.isRunning}
                      />
                      <StatusPill status="neutral" label={launcherLabel[game.launcher]} />
                      <StatusPill status="warn" label={`${game.tweaksAvailable} tweaks`} />
                      {game.runningPid ? <StatusPill status="info" label={`PID ${game.runningPid}`} /> : null}
                    </div>
                    <div className="font-display" style={{ fontSize: 22, fontWeight: 800, lineHeight: 1.05 }}>{game.displayName}</div>
                    <div className="tiny" style={{ color: "var(--text-secondary)", wordBreak: "break-word" }}>
                      {game.installPath || "Detected from an active process without a stable install path."}
                    </div>
                  </div>

                  <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap", justifyContent: "flex-end" }}>
                    <GlowButton
                      label={game.isRunning ? "Optimize Running Game" : "Launch & Optimize"}
                      onClick={() => void performAction(
                        game.isRunning ? optimizeKey : launchKey,
                        () => game.isRunning
                          ? tauriInvoke<GameActionPayload>("optimize_game", { executableName: game.executableName })
                          : tauriInvoke<GameActionPayload>("launch_optimized_game", { executableName: game.executableName }),
                        game.isRunning ? `${game.displayName} optimized` : `${game.displayName} launched`
                      )}
                      loading={activeAction === optimizeKey || activeAction === launchKey}
                      disabled={!game.isRunning && !game.isDetected}
                    />
                    <GlowButton
                      label="Revert"
                      variant="ghost"
                      onClick={() => void performAction(revertKey, () => tauriInvoke<boolean>("revert_game_optimization"), `${game.displayName} reverted`)}
                      loading={activeAction === revertKey}
                      disabled={!isActiveTarget && !game.isOptimized}
                    />
                  </div>
                </div>

                <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                  <div className="title-kicker">Applied profile elements</div>
                  <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                    {game.tweaksApplied.length > 0 ? game.tweaksApplied.map((tweak) => (
                      <span
                        key={tweak}
                        style={{
                          padding: "6px 10px",
                          borderRadius: 999,
                          border: "1px solid var(--border-glass)",
                          background: "rgba(255,255,255,0.03)",
                          fontSize: 11,
                          color: "var(--text-secondary)"
                        }}
                      >
                        {tweak}
                      </span>
                    )) : (
                      <span className="tiny" style={{ color: "var(--text-secondary)" }}>No per-game tweaks recorded yet.</span>
                    )}
                  </div>
                </div>
              </div>
            </GlassCard>
          );
        })}

        {!isLoading && games.length === 0 ? (
          <GlassCard>
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>
              No supported installed games were found yet. Running games will still appear here when detected.
            </div>
          </GlassCard>
        ) : null}
      </div>
    </div>
  );
}


