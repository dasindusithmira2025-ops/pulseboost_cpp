import type { MouseEvent } from "react";
import Icon from "../ui/Icon";
import { useWindowControls } from "../../hooks/useWindowControls";
import { useSystemStore } from "../../stores/systemStore";
import { useUiStore } from "../../stores/uiStore";
import { getScreenLabel } from "../../lib/nav";

function isInteractiveElement(target: EventTarget | null): boolean {
  if (!(target instanceof HTMLElement)) {
    return false;
  }
  return Boolean(target.closest("[data-no-drag='true']"));
}

function healthTone(score: number): string {
  if (score >= 75) {
    return "var(--status-ok)";
  }
  if (score >= 45) {
    return "var(--status-warn)";
  }
  return "var(--status-danger)";
}

const windowControls = [
  { icon: "minimize", title: "Minimize", key: "minimize" },
  { icon: "maximize", title: "Maximize", key: "maximize" },
  { icon: "close", title: "Close", key: "close" }
] as const;

export default function TopBar() {
  const { startDragging, minimize, maximize, closeApp, refreshAll } = useWindowControls();
  const query = useUiStore((s) => s.searchQuery);
  const setQuery = useUiStore((s) => s.setSearchQuery);
  const activeScreen = useUiStore((s) => s.activeScreen);
  const notifications = useSystemStore((s) => s.notifications);
  const snapshot = useSystemStore((s) => s.snapshot);

  const onHeaderMouseDown = (event: MouseEvent<HTMLElement>) => {
    if (event.button !== 0 || isInteractiveElement(event.target)) {
      return;
    }
    void startDragging();
  };

  const unreadCount = notifications.length;
  return (
    <header
      className="glass-card shell-panel"
      onMouseDown={onHeaderMouseDown}
      style={{
        gridColumn: 2,
        gridRow: 1,
        minWidth: 0,
        height: 56,
        display: "grid",
        gridTemplateColumns: "auto minmax(0, 1fr) auto",
        alignItems: "center",
        gap: "var(--sp-4)",
        padding: "0 var(--sp-4)"
      }}
    >
      <div style={{ display: "flex", alignItems: "center", gap: "var(--sp-3)", minWidth: 0 }}>
        <div
          style={{
            width: 32,
            height: 32,
            borderRadius: 11,
            display: "grid",
            placeItems: "center",
            background: "linear-gradient(145deg, var(--accent), var(--accent2))",
            boxShadow: "0 10px 30px rgba(0, 220, 255, 0.24)",
            color: "#04111f",
            fontFamily: "var(--font-display)",
            fontSize: 13,
            fontWeight: 800,
            letterSpacing: "0.08em"
          }}
        >
          PB
        </div>
        <div style={{ minWidth: 0 }}>
          <div className="title-kicker">PulseBoost AI</div>
          <div className="font-display" style={{ fontSize: 15, fontWeight: 700, lineHeight: 1.15 }}>
            {getScreenLabel(activeScreen)}
          </div>
        </div>
      </div>

      <div data-no-drag="true" style={{ minWidth: 0 }}>
        <div
          className="input-shell interactive"
          style={{
            height: 40,
            display: "grid",
            gridTemplateColumns: "auto 1fr auto",
            alignItems: "center",
            gap: "var(--sp-3)",
            padding: "0 var(--sp-4)"
          }}
        >
          <span style={{ color: "var(--text-tertiary)", display: "grid", placeItems: "center" }}>
            <Icon name="home" size={14} />
          </span>
          <input
            className="focus-ring"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            placeholder="Search Home, Optimizations, Games, or Backup"
            style={{
              width: "100%",
              background: "transparent",
              border: "none",
              color: "var(--text-primary)",
              fontSize: 13
            }}
          />
          <span className="font-mono tiny">Ready</span>
        </div>
      </div>

      <div data-no-drag="true" style={{ display: "flex", alignItems: "center", gap: "var(--sp-2)" }}>
        <button
          type="button"
          className="icon-button focus-ring interactive"
          onClick={refreshAll}
          aria-label="Refresh telemetry"
          title="Refresh"
        >
          <Icon name="refresh" size={15} />
        </button>

        <button
          type="button"
          className="icon-button focus-ring interactive"
          aria-label="Notifications"
          title="Notifications"
          style={{ position: "relative" }}
        >
          <Icon name="notifications" size={15} />
          {unreadCount > 0 ? (
            <span
              style={{
                position: "absolute",
                top: -4,
                right: -4,
                minWidth: 18,
                height: 18,
                padding: "0 5px",
                borderRadius: 999,
                background: "var(--status-danger)",
                color: "#fff",
                display: "grid",
                placeItems: "center",
                fontSize: 10,
                fontWeight: 700,
                boxShadow: "0 6px 16px rgba(255, 93, 115, 0.35)"
              }}
            >
              {Math.min(unreadCount, 9)}
            </span>
          ) : null}
        </button>

        <div style={{ width: 1, height: 18, background: "var(--border-glass)" }} />

        {windowControls.map((item) => {
          const action = item.key === "minimize"
            ? () => void minimize()
            : item.key === "maximize"
              ? () => void maximize()
              : () => void closeApp();

          return (
            <button
              key={item.title}
              type="button"
              className="focus-ring interactive"
              onClick={action}
              aria-label={item.title}
              title={item.title}
              style={{
                width: 32,
                height: 32,
                borderRadius: "var(--r-md)",
                border: "1px solid var(--border-glass)",
                background: "rgba(255,255,255,0.03)",
                display: "grid",
                placeItems: "center",
                color: item.key === "close" ? "var(--status-danger)" : "var(--text-secondary)"
              }}
            >
              <Icon name={item.icon} size={14} />
            </button>
          );
        })}
      </div>
    </header>
  );
}
