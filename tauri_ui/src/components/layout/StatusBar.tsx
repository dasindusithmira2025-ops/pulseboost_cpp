import { format } from "date-fns";
import { useEffect, useState } from "react";
import StatusPill from "../ui/StatusPill";
import { useSystemStore } from "../../stores/systemStore";
import { useUiStore } from "../../stores/uiStore";
import { getScreenLabel } from "../../lib/nav";

function healthStatus(score: number): "ok" | "warn" | "danger" {
  if (score >= 75) {
    return "ok";
  }
  if (score >= 45) {
    return "warn";
  }
  return "danger";
}

export default function StatusBar() {
  const [now, setNow] = useState(Date.now());
  const screen = useUiStore((s) => s.activeScreen);
  const snapshot = useSystemStore((s) => s.snapshot);
  const score = snapshot?.healthScore ?? 0;
  const cpu = snapshot?.cpuPercent ?? 0;
  const ram = snapshot?.ramPercent ?? 0;

  useEffect(() => {
    const t = window.setInterval(() => setNow(Date.now()), 30000);
    return () => window.clearInterval(t);
  }, []);

  return (
    <footer
      className="glass-card shell-panel"
      style={{
        gridColumn: 2,
        gridRow: 3,
        minWidth: 0,
        height: 38,
        padding: "0 var(--sp-4)",
        display: "flex",
        alignItems: "center",
        justifyContent: "space-between",
        gap: "var(--sp-4)"
      }}
    >
      <div style={{ display: "flex", alignItems: "center", gap: "var(--sp-3)", minWidth: 0 }}>
        <StatusPill status="ok" label="PulseModel active" dot />
        <div style={{ width: 1, height: 16, background: "var(--border-glass)" }} />
        <span className="font-mono tiny" style={{ whiteSpace: "nowrap" }}>
          {getScreenLabel(screen)}
        </span>
      </div>

      <div style={{ display: "flex", alignItems: "center", gap: "var(--sp-3)", minWidth: 0 }}>
        <span className="font-mono tiny">CPU {cpu.toFixed(0)}%</span>
        <span className="font-mono tiny">RAM {ram.toFixed(0)}%</span>
        <StatusPill status={healthStatus(score)} label={`Health ${score.toFixed(0)}`} />
        <div style={{ width: 1, height: 16, background: "var(--border-glass)" }} />
        <span className="font-mono tiny">{format(now, "HH:mm")}</span>
      </div>
    </footer>
  );
}
