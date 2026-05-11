import { lazy, Suspense } from "react";
import GlassCard from "../components/ui/GlassCard";
import SectionHeader from "../components/ui/SectionHeader";
import { useUiStore } from "../stores/uiStore";
import type { ToolsTab } from "../types/system";

const ProcessManager = lazy(() => import("./ProcessManager"));
const StorageAnalyzer = lazy(() => import("./StorageAnalyzer"));
const NetworkMonitor = lazy(() => import("./NetworkMonitor"));
const StartupManager = lazy(() => import("./StartupManager"));
const RamOptimizer = lazy(() => import("./RamOptimizer"));
const Thermals = lazy(() => import("./Thermals"));

const toolsTabs: Array<{ id: ToolsTab; label: string; description: string }> = [
  { id: "processes", label: "Processes", description: "Inspect load and control individual apps" },
  { id: "storage", label: "Storage", description: "Clean, scan, and reclaim disk space" },
  { id: "network", label: "Network", description: "Diagnose latency and network tuning" },
  { id: "startup", label: "Startup", description: "Cut boot delay and manage launch items" },
  { id: "ram", label: "RAM", description: "Recover memory and inspect pressure" },
  { id: "thermals", label: "Thermals", description: "Watch confirmed heat and cooling data" }
];

function ActiveToolView({ tab }: { tab: ToolsTab }) {
  switch (tab) {
    case "processes":
      return <ProcessManager />;
    case "storage":
      return <StorageAnalyzer />;
    case "network":
      return <NetworkMonitor />;
    case "startup":
      return <StartupManager />;
    case "ram":
      return <RamOptimizer />;
    case "thermals":
      return <Thermals />;
    default:
      return null;
  }
}

export default function Tools() {
  const activeToolsTab = useUiStore((s) => s.activeToolsTab);
  const setActiveToolsTab = useUiStore((s) => s.setActiveToolsTab);
  const activeMeta = toolsTabs.find((tab) => tab.id === activeToolsTab) ?? toolsTabs[0];

  return (
    <div style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow="accent">
        <SectionHeader
          title="Advanced Controls"
          subtitle="Specialist controls stay available here, but they no longer compete with the main optimization workflow."
        />
        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 320px", gap: "var(--sp-4)" }}>
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div className="title-kicker">Advanced control</div>
            <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.02 }}>
              Open detailed controls only when you need them.
            </div>
            <div className="muted">
              The main product now stays focused on home, optimizations, Boost-Up, games, backup, and AI.
            </div>
          </div>

          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
            <div className="title-kicker">Current tool</div>
            <div className="font-display" style={{ fontSize: 20, fontWeight: 800 }}>{activeMeta.label}</div>
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>{activeMeta.description}</div>
          </div>
        </div>

        <div className="badge-row" style={{ marginTop: "var(--sp-4)" }}>
          {toolsTabs.map((tab) => {
            const active = tab.id === activeToolsTab;
            return (
              <button
                key={tab.id}
                type="button"
                className="focus-ring interactive"
                onClick={() => setActiveToolsTab(tab.id)}
                style={{
                  minHeight: 34,
                  borderRadius: 999,
                  padding: "0 var(--sp-4)",
                  border: `1px solid ${active ? "var(--border-accent)" : "var(--border-glass)"}`,
                  background: active
                    ? "linear-gradient(135deg, rgba(0,220,255,0.16), rgba(122,92,255,0.12))"
                    : "rgba(255,255,255,0.03)",
                  color: active ? "var(--text-primary)" : "var(--text-secondary)",
                  fontWeight: active ? 700 : 600
                }}
              >
                {tab.label}
              </button>
            );
          })}
        </div>
      </GlassCard>

      <Suspense
        fallback={(
          <GlassCard>
            <div className="skeleton" style={{ height: 320 }} />
          </GlassCard>
        )}
      >
        <ActiveToolView tab={activeToolsTab} />
      </Suspense>
    </div>
  );
}

