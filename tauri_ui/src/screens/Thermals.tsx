import GlassCard from "../components/ui/GlassCard";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { useSystemStore } from "../stores/systemStore";

function tempStatus(temp: number): "ok" | "warn" | "danger" {
  if (temp < 60) {
    return "ok";
  }
  if (temp <= 80) {
    return "warn";
  }
  return "danger";
}

export default function Thermals() {
  const snapshot = useSystemStore((s) => s.snapshot);
  const temp = snapshot?.temperatureCelsius ?? 0;
  const status = tempStatus(temp);

  const ringColor =
    status === "ok"
      ? "var(--status-ok)"
      : status === "warn"
        ? "var(--status-warn)"
        : "var(--status-danger)";

  return (
    <div className="screen-enter" style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow={status === "danger" ? "danger" : status === "warn" ? "warn" : "ok"}>
        <SectionHeader title="Thermals" subtitle="CPU package temperature and confirmed thermal telemetry" />
        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 320px", gap: "var(--sp-4)", alignItems: "center" }}>
          <div style={{ display: "grid", placeItems: "center", minHeight: 320, position: "relative" }}>
            <div
              style={{
                position: "absolute",
                width: 540,
                height: 280,
                borderRadius: "50%",
                filter: "blur(60px)",
                background:
                  status === "ok"
                    ? "rgba(0, 194, 255, 0.16)"
                    : status === "warn"
                      ? "rgba(255, 176, 32, 0.16)"
                      : "rgba(255, 71, 87, 0.16)"
              }}
            />
            <svg width={260} height={260} viewBox="0 0 260 260" style={{ position: "relative" }}>
              <circle cx="130" cy="130" r="102" fill="none" stroke="var(--border-glass-md)" strokeWidth="16" />
              <circle
                cx="130"
                cy="130"
                r="102"
                fill="none"
                stroke={ringColor}
                strokeWidth="16"
                strokeLinecap="round"
                strokeDasharray={`${(Math.min(100, temp) / 100) * 641} 999`}
                transform="rotate(-90 130 130)"
              />
              <text
                x="130"
                y="126"
                textAnchor="middle"
                className="font-mono"
                style={{ fill: "var(--text-primary)", fontSize: 48, fontWeight: 700 }}
              >
                {temp.toFixed(0)} C
              </text>
              <text
                x="130"
                y="154"
                textAnchor="middle"
                className="font-body"
                style={{ fill: "var(--text-secondary)", fontSize: 12 }}
              >
                CPU PACKAGE
              </text>
            </svg>
            <StatusPill status={status} label={status === "ok" ? "Cool" : status === "warn" ? "Warm" : "Critical"} />
          </div>

          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Thermal state</div>
              <div className="font-display" style={{ fontSize: 28, fontWeight: 800, lineHeight: 1.05, marginBottom: 10 }}>
                Keep thermal pressure visible before it becomes throttling.
              </div>
              <div className="muted">
                This screen only presents directly measured package temperature and confirmed fan data instead of inventing synthetic secondary values.
              </div>
            </div>
            <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
              <StatusPill status={status} label={`CPU ${temp.toFixed(1)} C`} />
              <StatusPill status="info" label={`Fan ${snapshot?.fanRpm ?? 0} RPM`} />
            </div>
          </div>
        </div>
      </GlassCard>

      <GlassCard>
        <SectionHeader title="Thermal Table" subtitle="Only directly measured values are shown as confirmed" />
        <div style={{ display: "grid", gap: "var(--sp-2)" }}>
          <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "grid", gridTemplateColumns: "1fr auto auto", alignItems: "center", gap: "var(--sp-3)" }}>
            <span>CPU</span>
            <span className="font-mono tiny">{temp.toFixed(1)} C</span>
            <span className="font-mono tiny">{snapshot?.fanRpm ?? 0} RPM</span>
          </div>
          <div className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)" }}>
            <span className="tiny">Separate GPU and drive temperatures remain hidden until dedicated telemetry is available.</span>
          </div>
        </div>
      </GlassCard>
    </div>
  );
}

