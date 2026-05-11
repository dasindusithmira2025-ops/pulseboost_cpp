import { useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { useCommands } from "../hooks/useCommands";
import { useSystemStore } from "../stores/systemStore";

export default function StartupManager() {
  const items = useSystemStore((s) => s.snapshot?.startupItems ?? []);
  const { run } = useCommands();
  const [selected, setSelected] = useState<Record<string, boolean>>({});

  const selectedCount = Object.values(selected).filter(Boolean).length;
  const bootDelay = items.reduce((acc, item) => {
    if (item.impact === "high" && item.enabled) {
      return acc + 2.8;
    }
    if (item.impact === "medium" && item.enabled) {
      return acc + 1.2;
    }
    return acc;
  }, 6.2);

  const selectedItems = items.filter((item) => selected[item.name]);

  const selectedPotentialSavings = useMemo(() => selectedItems.reduce((acc, item) => {
    if (item.impact === "high") {
      return acc + 2.8;
    }
    if (item.impact === "medium") {
      return acc + 1.2;
    }
    return acc + 0.4;
  }, 0), [selectedItems]);

  const disableSelected = async () => {
    for (const item of selectedItems) {
      if (item.enabled) {
        await run("toggle_startup_item", { name: item.name, enabled: false });
      }
    }
  };

  const delaySelected = async () => {
    for (const item of selectedItems) {
      await run("delay_startup_item", { name: item.name, seconds: 10 });
    }
  };

  return (
    <div className="screen-enter" style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow="accent">
        <SectionHeader title="Startup" subtitle={`${items.length} items | estimated boot delay ${bootDelay.toFixed(1)}s`} />
        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 320px", gap: "var(--sp-4)" }}>
          <div>
            <div className="title-kicker" style={{ marginBottom: 8 }}>Startup control</div>
            <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.05, marginBottom: 10 }}>
              Trim boot overhead without losing visibility into what starts with Windows.
            </div>
            <div className="muted">
              Review impact, toggle item state from live telemetry data, and batch-delay noisy items when you want a faster boot path.
            </div>
          </div>
          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
            <StatusPill status="info" label={`${selectedCount} selected`} />
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>
              Potential savings from selected items: {selectedPotentialSavings.toFixed(1)}s
            </div>
          </div>
        </div>
      </GlassCard>

      <GlassCard>
        <div style={{ display: "grid", gap: "var(--sp-2)" }}>
          {items.map((item, index) => (
            <div
              key={item.name}
              className="surface-panel screen-enter"
              style={{
                padding: "var(--sp-3)",
                borderRadius: "var(--r-lg)",
                display: "grid",
                gridTemplateColumns: "auto minmax(0, 1.8fr) minmax(0, 1.2fr) auto auto auto",
                gap: "var(--sp-3)",
                alignItems: "center",
                animationDelay: `${Math.min(index, 8) * 35}ms`
              }}
            >
              <button
                type="button"
                className="focus-ring interactive"
                onClick={() => setSelected((prev) => ({ ...prev, [item.name]: !prev[item.name] }))}
                style={{
                  width: 22,
                  height: 22,
                  borderRadius: 8,
                  border: `1px solid ${(selected[item.name] ?? false) ? "var(--border-accent)" : "var(--border-glass)"}`,
                  background: (selected[item.name] ?? false)
                    ? "linear-gradient(135deg, rgba(0,220,255,0.18), rgba(122,92,255,0.1))"
                    : "rgba(255,255,255,0.03)",
                  color: (selected[item.name] ?? false) ? "var(--text-primary)" : "var(--text-tertiary)",
                  fontSize: 11,
                  fontWeight: 700
                }}
              >
                {(selected[item.name] ?? false) ? "x" : ""}
              </button>
              <div style={{ minWidth: 0 }}>
                <div className="font-display nav-item-label" style={{ fontSize: 15, fontWeight: 700 }}>{item.name}</div>
                <div className="tiny nav-item-label">{item.publisher || item.location}</div>
              </div>
              <div className="tiny nav-item-label" style={{ color: "var(--text-secondary)" }}>{item.location}</div>
              <StatusPill status={item.enabled ? "ok" : "neutral"} label={item.enabled ? "Enabled" : "Disabled"} />
              <StatusPill status={item.impact === "high" ? "danger" : item.impact === "medium" ? "warn" : "ok"} label={item.impact} />
              <div style={{ display: "flex", gap: "var(--sp-2)", justifyContent: "flex-end" }}>
                <GlowButton label={item.enabled ? "Disable" : "Enable"} variant="ghost" onClick={() => void run("toggle_startup_item", { name: item.name, enabled: !item.enabled })} />
                <GlowButton label="Delay" variant="ghost" onClick={() => void run("delay_startup_item", { name: item.name, seconds: 10 })} />
              </div>
            </div>
          ))}
        </div>
      </GlassCard>

      {selectedCount > 0 ? (
        <GlassCard glow="warn">
          <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: "var(--sp-3)", flexWrap: "wrap" }}>
            <div>
              <div className="font-display" style={{ fontSize: 18, fontWeight: 700 }}>{selectedCount} items selected</div>
              <div className="tiny" style={{ color: "var(--text-secondary)" }}>Use batch actions to speed up cleanup of high-impact startup noise.</div>
            </div>
            <div style={{ display: "flex", gap: "var(--sp-2)" }}>
              <GlowButton label="Disable Selected" variant="danger" onClick={() => void disableSelected()} />
              <GlowButton label="Delay Selected" onClick={() => void delaySelected()} />
            </div>
          </div>
        </GlassCard>
      ) : null}
    </div>
  );
}

