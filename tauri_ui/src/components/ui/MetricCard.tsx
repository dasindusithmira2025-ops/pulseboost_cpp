import GlassCard from "./GlassCard";
import SparkLine from "./SparkLine";
import StatusPill from "./StatusPill";

interface MetricCardProps {
  label: string;
  value: number | string;
  unit?: string;
  delta?: number;
  icon?: string;
  trend?: "up" | "down" | "flat";
  glowColor?: "accent" | "ok" | "warn" | "danger" | "none";
  sparkline?: number[];
}

function accentColor(glowColor: MetricCardProps["glowColor"]): string {
  if (glowColor === "ok") {
    return "var(--status-ok)";
  }
  if (glowColor === "warn") {
    return "var(--status-warn)";
  }
  if (glowColor === "danger") {
    return "var(--status-danger)";
  }
  return glowColor === "none" ? "var(--text-tertiary)" : "var(--accent)";
}

export default function MetricCard({
  label,
  value,
  unit,
  delta = 0,
  icon,
  trend = "flat",
  glowColor = "none",
  sparkline = []
}: MetricCardProps) {
  const deltaSign = delta > 0 ? "+" : "";
  const deltaStatus = delta > 0 ? "warn" : delta < 0 ? "ok" : "neutral";
  const trendGlyph = trend === "up" ? "UP" : trend === "down" ? "DOWN" : "FLAT";
  const color = accentColor(glowColor);

  return (
    <GlassCard glow={glowColor} className="screen-enter" style={{ minHeight: 176 }}>
      <div
        style={{
          position: "absolute",
          top: 0,
          left: 0,
          right: 0,
          height: 2,
          background: `linear-gradient(90deg, ${color}, transparent 80%)`
        }}
      />

      <div
        style={{
          display: "flex",
          alignItems: "center",
          justifyContent: "space-between",
          marginBottom: "var(--sp-3)"
        }}
      >
        <div>
          <div className="title-kicker" style={{ marginBottom: 6 }}>
            {label}
          </div>
          <div className="font-mono tiny" style={{ color }}>
            {icon ?? label}
          </div>
        </div>
        <StatusPill status={deltaStatus} label={`${trendGlyph} ${deltaSign}${delta.toFixed(1)}%`} />
      </div>

      <div style={{ display: "flex", alignItems: "flex-end", gap: "var(--sp-2)", marginBottom: "var(--sp-3)" }}>
        <div className="font-display value-updated" style={{ fontSize: 34, fontWeight: 800, lineHeight: 0.95, letterSpacing: "-0.02em" }}>
          {value}
        </div>
        {unit ? (
          <span className="font-mono" style={{ fontSize: 11, color: "var(--text-tertiary)", marginBottom: 4 }}>
            {unit}
          </span>
        ) : null}
      </div>

      <div
        style={{
          padding: "var(--sp-2)",
          borderRadius: "var(--r-md)",
          border: "1px solid rgba(255,255,255,0.06)",
          background: "rgba(255,255,255,0.025)"
        }}
      >
        <SparkLine data={sparkline} color={color} />
      </div>
    </GlassCard>
  );
}
