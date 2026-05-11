import Icon from "../ui/Icon";
import { primaryNavItems, secondaryNavItems } from "../../lib/nav";
import { useSystemStore } from "../../stores/systemStore";
import { useUiStore } from "../../stores/uiStore";
import type { NavItem } from "../../lib/nav";

function healthTone(score: number): string {
  if (score >= 75) {
    return "var(--status-ok)";
  }
  if (score >= 45) {
    return "var(--status-warn)";
  }
  return "var(--status-danger)";
}

export default function Sidebar() {
  const activeScreen = useUiStore((s) => s.activeScreen);
  const setActiveScreen = useUiStore((s) => s.setActiveScreen);
  const collapsed = useUiStore((s) => s.sidebarCollapsed);
  const setCollapsed = useUiStore((s) => s.setSidebarCollapsed);
  const isPro = useUiStore((s) => s.isPro);
  const snapshot = useSystemStore((s) => s.snapshot);

  const health = snapshot?.healthScore ?? 0;
  const cpu = snapshot?.cpuPercent ?? 0;
  const ram = snapshot?.ramPercent ?? 0;

  const renderNavButton = (item: NavItem, accent = false) => {
    const active = item.id === activeScreen;
    return (
      <button
        key={item.id}
        type="button"
        className="focus-ring interactive"
        onClick={() => setActiveScreen(item.id)}
        title={collapsed ? item.label : undefined}
        style={{
          width: "100%",
          minHeight: 46,
          marginBottom: "var(--sp-2)",
          borderRadius: "var(--r-lg)",
          border: `1px solid ${active ? "var(--border-accent)" : "transparent"}`,
          background: active
            ? accent
              ? "linear-gradient(90deg, rgba(0, 220, 255, 0.2), rgba(122, 92, 255, 0.12))"
              : "linear-gradient(90deg, rgba(255,255,255,0.07), rgba(255,255,255,0.02))"
            : "transparent",
          color: active ? "var(--text-primary)" : "var(--text-secondary)",
          display: "grid",
          gridTemplateColumns: collapsed ? "1fr" : "34px minmax(0, 1fr)",
          alignItems: "center",
          gap: collapsed ? 0 : "var(--sp-3)",
          padding: collapsed ? 0 : "0 var(--sp-3)",
          boxShadow: active ? "0 10px 24px rgba(0, 220, 255, 0.1)" : "none",
          position: "relative"
        }}
      >
        {active ? (
          <span
            style={{
              position: "absolute",
              left: 0,
              top: 9,
              bottom: 9,
              width: 3,
              borderRadius: 999,
              background: "linear-gradient(180deg, var(--accent), var(--accent2))"
            }}
          />
        ) : null}
        <span
          aria-hidden
          style={{
            width: 26,
            height: 26,
            display: "grid",
            placeItems: "center",
            borderRadius: 8,
            justifySelf: collapsed ? "center" : "start",
            background: active ? "rgba(255,255,255,0.08)" : "rgba(255,255,255,0.04)",
            color: active ? "var(--text-accent)" : "var(--text-muted)"
          }}
        >
          <Icon name={item.icon} size={15} />
        </span>
        {!collapsed ? (
          <span style={{ minWidth: 0, display: "grid", gap: 2, textAlign: "left" }}>
            <span className="font-display nav-item-label" style={{ fontSize: 13, fontWeight: active ? 700 : 600 }}>
              {item.label}
            </span>
            <span className="tiny nav-item-label">{item.description}</span>
          </span>
        ) : null}
      </button>
    );
  };

  return (
    <aside
      className="glass-card shell-panel"
      style={{
        gridColumn: 1,
        gridRow: "1 / 4",
        width: collapsed ? 78 : 248,
        transition: "width 240ms var(--ease-out-premium)",
        display: "flex",
        flexDirection: "column",
        padding: "var(--sp-3)",
        overflow: "hidden",
        position: "relative"
      }}
    >
      <div style={{ display: "flex", alignItems: "center", gap: "var(--sp-3)", padding: "var(--sp-2)", marginBottom: "var(--sp-3)" }}>
        <div
          style={{
            width: 36,
            height: 36,
            borderRadius: 13,
            background: "linear-gradient(145deg, var(--accent), var(--accent2))",
            display: "grid",
            placeItems: "center",
            color: "#04111f",
            fontSize: 13,
            fontWeight: 900,
            fontFamily: "var(--font-display)",
            letterSpacing: "0.08em",
            boxShadow: "0 14px 34px rgba(0, 220, 255, 0.24)"
          }}
        >
          PB
        </div>
        {!collapsed ? (
          <div style={{ minWidth: 0 }}>
            <div className="title-kicker">PC optimization suite</div>
            <div className="font-display" style={{ fontWeight: 800, fontSize: 16, lineHeight: 1.1 }}>
              PulseBoost AI
            </div>
          </div>
        ) : null}
      </div>

      {!collapsed ? (
        <div
          className="surface-panel"
          style={{
            padding: "var(--sp-3)",
            borderRadius: "var(--r-lg)",
            marginBottom: "var(--sp-3)",
            display: "grid",
            gap: "var(--sp-2)"
          }}
        >
          <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: "var(--sp-2)" }}>
            <div className="title-kicker">System state</div>
            <span
              style={{
                padding: "4px 8px",
                borderRadius: 999,
                border: `1px solid ${isPro ? "rgba(0,229,160,0.24)" : "var(--border-glass)"}`,
                background: isPro ? "rgba(0,229,160,0.12)" : "rgba(255,255,255,0.03)",
                color: isPro ? "var(--status-ok)" : "var(--text-secondary)",
                fontSize: 10,
                fontWeight: 700,
                letterSpacing: "0.08em",
                textTransform: "uppercase"
              }}
            >
              {isPro ? "Pro" : "Free"}
            </span>
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: "var(--sp-2)" }}>
            <span className="status-dot" style={{ background: healthTone(health) }} />
            <span className="font-display" style={{ fontSize: 15, fontWeight: 700 }}>
              Health {health.toFixed(0)}
            </span>
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: "var(--sp-2)" }}>
            <div>
              <div className="tiny">CPU</div>
              <div className="font-mono" style={{ fontSize: 12 }}>{cpu.toFixed(0)}%</div>
            </div>
            <div>
              <div className="tiny">RAM</div>
              <div className="font-mono" style={{ fontSize: 12 }}>{ram.toFixed(0)}%</div>
            </div>
          </div>
        </div>
      ) : null}

      <div style={{ flex: 1, overflowY: "auto", paddingRight: 2 }}>
        {!collapsed ? <div className="title-kicker" style={{ margin: "0 0 var(--sp-2) var(--sp-2)" }}>Core journey</div> : null}
        {primaryNavItems.map((item) => renderNavButton(item, item.id !== "ai"))}

        {!collapsed ? <div className="title-kicker" style={{ margin: "var(--sp-3) 0 var(--sp-2) var(--sp-2)" }}>Product and recovery</div> : null}
        {secondaryNavItems.map((item) => renderNavButton(item))}
      </div>

      <div className="surface-panel" style={{ padding: collapsed ? "var(--sp-2)" : "var(--sp-3)", borderRadius: "var(--r-lg)", marginTop: "var(--sp-2)" }}>
        <div style={{ display: "flex", alignItems: "center", justifyContent: collapsed ? "center" : "space-between", gap: "var(--sp-2)" }}>
          {!collapsed ? (
            <div>
              <div className="title-kicker">Assistant state</div>
              <div className="font-display" style={{ fontSize: 13, fontWeight: 700 }}>
                Local model ready
              </div>
            </div>
          ) : null}
          <span className="status-dot" style={{ background: "var(--status-ok)" }} />
        </div>
      </div>

      <button
        type="button"
        className="focus-ring interactive"
        onClick={() => setCollapsed(!collapsed)}
        style={{
          marginTop: "var(--sp-2)",
          height: 34,
          borderRadius: "var(--r-lg)",
          border: "1px solid var(--border-glass)",
          background: "rgba(255, 255, 255, 0.03)",
          color: "var(--text-secondary)",
          cursor: "pointer",
          fontFamily: "var(--font-mono)",
          fontSize: 12,
          letterSpacing: "0.08em"
        }}
      >
        {collapsed ? "Expand" : "Collapse"}
      </button>
    </aside>
  );
}
