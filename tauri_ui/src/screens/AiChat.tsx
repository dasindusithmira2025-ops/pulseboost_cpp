import { useEffect, useMemo, useState } from "react";
import ChatBubble from "../components/ai/ChatBubble";
import QuickActionChips from "../components/ai/QuickActionChips";
import TypingIndicator from "../components/ai/TypingIndicator";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { useCommands } from "../hooks/useCommands";
import { isTauriRuntime, tauriInvoke, tauriListen } from "../lib/tauri";
import { useChatStore } from "../stores/chatStore";
import { useSystemStore } from "../stores/systemStore";
import type { AiPreferences } from "../types/system";

const quick = [
  "Why is my PC slow?",
  "Clean junk files",
  "Check for issues",
  "Boost game performance"
];

export default function AiChat() {
  const { run } = useCommands();
  const snapshot = useSystemStore((s) => s.snapshot);
  const notifications = useSystemStore((s) => s.notifications);
  const messages = useChatStore((s) => s.messages);
  const sendMessage = useChatStore((s) => s.sendMessage);
  const appendStreamChunk = useChatStore((s) => s.appendStreamChunk);
  const isTyping = useChatStore((s) => s.isTyping);
  const [value, setValue] = useState("");
  const [aiPreferences, setAiPreferences] = useState<AiPreferences | null>(null);

  useEffect(() => {
    if (!isTauriRuntime()) {
      return;
    }

    let disposed = false;
    let unlisten: (() => void) | null = null;

    void tauriListen<string>("agent_stream_chunk", (event) => {
      if (!disposed) {
        appendStreamChunk(event.payload);
      }
    }).then((cleanup) => {
      unlisten = cleanup;
    });

    return () => {
      disposed = true;
      if (unlisten) {
        void unlisten();
      }
    };
  }, [appendStreamChunk]);

  useEffect(() => {
    if (!isTauriRuntime()) {
      return;
    }
    void tauriInvoke<AiPreferences>("get_ai_preferences")
      .then((result) => setAiPreferences(result))
      .catch(() => undefined);
  }, []);

  const topIssue = useMemo(() => snapshot?.issues[0] ?? "No critical issue", [snapshot]);
  const recentEvents = notifications.slice(-3).reverse();
  const isHighRam = (snapshot?.ramPercent ?? 0) >= 80;
  const isHighCpu = (snapshot?.cpuPercent ?? 0) >= 85;

  const submit = async () => {
    const text = value.trim();
    if (!text) {
      return;
    }
    setValue("");
    await sendMessage(text);
  };

  return (
    <div
      className="screen-enter"
      style={{
        height: "100%",
        display: "grid",
        gridTemplateColumns: "minmax(0, 1fr) 340px",
        gap: "var(--sp-4)"
      }}
    >
      <GlassCard className="h-full" glow="accent" style={{ display: "flex", flexDirection: "column", minHeight: 0 }}>
        <SectionHeader title="AI Assistant" subtitle={aiPreferences?.mode === "cloud" ? "Cloud-preferred AI with local fallback and live system context" : "Local PulseModel with live system context and action routing"} />

        {messages.length === 0 ? (
          <div style={{ display: "grid", gap: "var(--sp-3)", marginBottom: "var(--sp-4)" }}>
            {isHighRam ? (
              <GlassCard glow="warn">
                <div className="title-kicker" style={{ marginBottom: 8 }}>Proactive insight</div>
                <div className="font-display" style={{ marginBottom: "var(--sp-2)", fontSize: 22, fontWeight: 700 }}>
                  High RAM usage detected ({snapshot?.ramPercent.toFixed(0) ?? "--"}%)
                </div>
                <div className="muted">I can recover memory immediately and identify the top consumers.</div>
                <div style={{ marginTop: "var(--sp-3)", display: "flex", gap: "var(--sp-2)" }}>
                  <GlowButton label="Optimize RAM" onClick={() => void run("optimize_ram")} />
                </div>
              </GlassCard>
            ) : null}
            {isHighCpu ? (
              <GlassCard>
                <div className="title-kicker" style={{ marginBottom: 8 }}>Live alert</div>
                <div className="font-display" style={{ marginBottom: "var(--sp-2)", fontSize: 22, fontWeight: 700 }}>
                  CPU pressure is elevated ({snapshot?.cpuPercent.toFixed(0) ?? "--"}%)
                </div>
                <div className="muted">Ask me to analyze process impact, startup overhead, or game-mode readiness.</div>
              </GlassCard>
            ) : null}
            {!isHighCpu && !isHighRam ? (
              <GlassCard>
                <div className="title-kicker" style={{ marginBottom: 8 }}>Ready</div>
                <div className="font-display" style={{ marginBottom: "var(--sp-2)", fontSize: 22, fontWeight: 700 }}>
                  System looks stable
                </div>
                <div className="muted">Ask for a full scan, startup analysis, tweak recommendations, or gaming advice.</div>
              </GlassCard>
            ) : null}
          </div>
        ) : null}

        <div
          className="surface-panel"
          style={{
            flex: 1,
            minHeight: 0,
            borderRadius: "var(--r-xl)",
            padding: "var(--sp-4)",
            display: "flex",
            flexDirection: "column",
            background: "linear-gradient(180deg, rgba(255,255,255,0.035), rgba(255,255,255,0.015))"
          }}
        >
          <div style={{ flex: 1, overflowY: "auto", paddingRight: "var(--sp-1)", minHeight: 0 }}>
            {messages.length === 0 ? (
              <div style={{ display: "grid", placeItems: "center", height: "100%" }}>
                <div style={{ width: "min(620px, 100%)", display: "grid", gap: "var(--sp-4)" }}>
                  <div>
                    <div className="title-kicker" style={{ marginBottom: 8 }}>Ask your PC anything</div>
                    <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.05, marginBottom: 10 }}>
                      Diagnostics, cleanup, gaming tweaks, startup analysis.
                    </div>
                    <div className="muted">
                      {aiPreferences?.mode === "cloud"
                        ? "Cloud mode is preferred for complex prompts, with automatic local fallback when unavailable."
                        : "PulseModel has your current telemetry, active issues, and recent optimization history in context."}
                    </div>
                  </div>
                  <QuickActionChips chips={quick} onSelect={(chip) => setValue(chip)} />
                </div>
              </div>
            ) : (
              messages.map((message) => (
                <ChatBubble
                  key={message.id}
                  role={message.role}
                  content={message.content}
                  timestamp={message.timestamp}
                  streaming={message.streaming}
                  actionId={message.actionId}
                  actionLabel={message.actionLabel}
                  onAction={(actionId) => void run(actionId)}
                />
              ))
            )}
            {isTyping ? <TypingIndicator /> : null}
          </div>

          <div style={{ marginTop: "var(--sp-4)", display: "grid", gap: "var(--sp-3)" }}>
            {messages.length > 0 ? <QuickActionChips chips={quick} onSelect={(chip) => setValue(chip)} /> : null}
            <div
              className="input-shell"
              style={{
                display: "grid",
                gridTemplateColumns: "1fr auto",
                gap: "var(--sp-2)",
                padding: "var(--sp-2)",
                borderRadius: "var(--r-xl)"
              }}
            >
              <input
                className="focus-ring"
                value={value}
                onChange={(event) => setValue(event.target.value)}
                onKeyDown={(event) => {
                  if (event.key === "Enter") {
                    void submit();
                  }
                }}
                placeholder="Ask PulseBoost AI anything about your system..."
                style={{
                  border: "none",
                  outline: "none",
                  background: "transparent",
                  color: "var(--text-primary)",
                  fontSize: 14,
                  padding: "0 var(--sp-3)"
                }}
              />
              <div style={{ display: "flex", gap: "var(--sp-2)" }}>
                <GlowButton label="Mic" variant="ghost" />
                <GlowButton label="Send" onClick={() => void submit()} disabled={value.trim().length === 0} />
              </div>
            </div>
          </div>
        </div>
      </GlassCard>

      <GlassCard className="h-full" style={{ minHeight: 0 }}>
        <SectionHeader title="Live Context" subtitle="What the active AI mode sees right now" />

        <div style={{ display: "grid", gap: "var(--sp-3)" }}>
          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-3)" }}>
            <div className="badge-row">
              <StatusPill status={aiPreferences?.mode === "cloud" ? "warn" : "ok"} label={aiPreferences?.mode === "cloud" ? "Cloud preferred" : "Local mode"} />
              <StatusPill status={aiPreferences?.cloudConfigured ? "ok" : "neutral"} label={aiPreferences?.cloudConfigured ? "Cloud key ready" : "Fallback only"} />
            </div>
            {[ 
              ["CPU", `${snapshot?.cpuPercent.toFixed(0) ?? "--"}%`, snapshot && snapshot.cpuPercent > 85 ? "warn" : "info"],
              ["RAM", `${snapshot?.ramPercent.toFixed(0) ?? "--"}%`, snapshot && snapshot.ramPercent > 85 ? "warn" : "info"],
              ["Disk", `${snapshot?.diskPercent.toFixed(0) ?? "--"}%`, "neutral"],
              ["Net", `${((snapshot?.netDownloadKbps ?? 0) / 1024).toFixed(1)} MB/s`, "neutral"]
            ].map(([key, display, tone]) => (
              <div
                key={String(key)}
                className="value-updated"
                style={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "space-between",
                  paddingBottom: "var(--sp-2)",
                  borderBottom: "1px solid rgba(255,255,255,0.06)"
                }}
              >
                <span className="title-kicker">{key}</span>
                <StatusPill status={tone as "ok" | "warn" | "danger" | "info" | "neutral"} label={String(display)} />
              </div>
            ))}
          </div>

          <GlassCard glow={isHighCpu || isHighRam ? "warn" : "accent"}>
            <div className="title-kicker" style={{ marginBottom: 8 }}>Top issue</div>
            <div className="font-display" style={{ fontSize: 18, fontWeight: 700, marginBottom: 8 }}>
              {topIssue}
            </div>
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>
              Ask for a fix, a root-cause explanation, or a game-performance interpretation.
            </div>
          </GlassCard>

          <div>
            <div className="section-title" style={{ marginBottom: "var(--sp-2)" }}>
              Recent Events
            </div>
            <div style={{ display: "grid", gap: "var(--sp-2)" }}>
              {recentEvents.length === 0 ? (
                <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", fontSize: 12 }}>
                  No actions yet in this session.
                </div>
              ) : (
                recentEvents.map((entry) => (
                  <div
                    key={entry.id}
                    className="surface-panel"
                    style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", fontSize: 12, lineHeight: 1.45 }}
                  >
                    {entry.message}
                  </div>
                ))
              )}
            </div>
          </div>
        </div>
      </GlassCard>
    </div>
  );
}

