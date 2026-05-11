import { useEffect } from "react";
import { useSystemStore } from "../../stores/systemStore";

function titleForAction(action: string): string {
  return action.split("_").join(" ");
}

export default function ActionProgressOverlay() {
  const actions = useSystemStore((s) =>
    Object.values(s.actionProgress).sort((a, b) => b.startedAt - a.startedAt)
  );
  const clearActionProgress = useSystemStore((s) => s.clearActionProgress);

  useEffect(() => {
    const completed = actions.filter((action) => typeof action.completedAt === "number");
    if (completed.length === 0) {
      return;
    }

    const timers = completed.map((action) =>
      window.setTimeout(() => clearActionProgress(action.action), action.success ? 1800 : 6000)
    );

    return () => {
      timers.forEach((timer) => window.clearTimeout(timer));
    };
  }, [actions, clearActionProgress]);

  if (actions.length === 0) {
    return null;
  }

  return (
    <div
      style={{
        position: "fixed",
        left: "var(--sp-5)",
        bottom: "calc(var(--sp-10) + 12px)",
        zIndex: 28,
        display: "flex",
        flexDirection: "column",
        gap: "var(--sp-2)",
        width: 360,
        pointerEvents: "none"
      }}
    >
      {actions.map((action) => {
        const percent = Math.max(0, Math.min(100, action.percent));
        const borderColor = action.success === undefined
          ? "var(--accent)"
          : action.success
            ? "var(--status-ok)"
            : "var(--status-danger)";

        return (
          <article
            key={action.action}
            className="glass-card shell-panel"
            style={{
              padding: "var(--sp-3) var(--sp-4)",
              borderLeft: `3px solid ${borderColor}`,
              animation: "toastIn var(--dur-layout) var(--ease-out-premium) both",
              pointerEvents: "auto",
              overflow: "hidden"
            }}
          >
            <div className="title-kicker" style={{ marginBottom: 6, color: borderColor }}>
              Action progress
            </div>
            <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", alignItems: "flex-start" }}>
              <div style={{ minWidth: 0 }}>
                <div className="font-display" style={{ fontSize: 14, textTransform: "capitalize", fontWeight: 700 }}>
                  {titleForAction(action.action)}
                </div>
                <div className="tiny">{action.message}</div>
              </div>
              <div className="font-mono tiny" style={{ color: "var(--text-muted)", flexShrink: 0 }}>
                {percent}%
              </div>
            </div>
            <div
              style={{
                marginTop: "var(--sp-3)",
                height: 7,
                borderRadius: 999,
                background: "rgba(255,255,255,0.06)",
                overflow: "hidden"
              }}
            >
              <div
                style={{
                  height: "100%",
                  width: `${percent}%`,
                  background: `linear-gradient(90deg, ${borderColor}, rgba(255,255,255,0.88))`,
                  transition: "width 180ms ease"
                }}
              />
            </div>
          </article>
        );
      })}
    </div>
  );
}
