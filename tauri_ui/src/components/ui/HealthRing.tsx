interface HealthRingProps {
  score: number;
  size?: number;
}

function arcColor(score: number): string {
  if (score >= 75) {
    return "url(#healthGradientGood)";
  }
  if (score >= 45) {
    return "url(#healthGradientWarn)";
  }
  return "url(#healthGradientDanger)";
}

function tone(score: number): string {
  if (score >= 75) {
    return "var(--status-ok)";
  }
  if (score >= 45) {
    return "var(--status-warn)";
  }
  return "var(--status-danger)";
}

export default function HealthRing({ score, size = 160 }: HealthRingProps) {
  const clamped = Math.max(0, Math.min(100, score));
  const radius = size / 2 - 14;
  const circumference = Math.PI * 2 * radius;
  const dash = (clamped / 100) * circumference;

  return (
    <div className="screen-enter" style={{ width: size, height: size + 42, textAlign: "center", position: "relative" }}>
      <div
        style={{
          position: "absolute",
          inset: size * 0.18,
          borderRadius: "50%",
          background: `radial-gradient(circle, rgba(255,255,255,0.04), transparent 65%), radial-gradient(circle, ${tone(clamped)}22, transparent 72%)`,
          filter: "blur(4px)",
          pointerEvents: "none"
        }}
      />
      <svg width={size} height={size} viewBox={`0 0 ${size} ${size}`} style={{ position: "relative", zIndex: 1 }}>
        <defs>
          <linearGradient id="healthGradientGood" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" stopColor="var(--accent)" />
            <stop offset="100%" stopColor="var(--status-ok)" />
          </linearGradient>
          <linearGradient id="healthGradientWarn" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" stopColor="var(--accent)" />
            <stop offset="100%" stopColor="var(--status-warn)" />
          </linearGradient>
          <linearGradient id="healthGradientDanger" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" stopColor="var(--status-warn)" />
            <stop offset="100%" stopColor="var(--status-danger)" />
          </linearGradient>
        </defs>
        <circle
          cx={size / 2}
          cy={size / 2}
          r={radius}
          fill="none"
          stroke="rgba(255,255,255,0.08)"
          strokeWidth={14}
        />
        <circle
          cx={size / 2}
          cy={size / 2}
          r={radius}
          fill="none"
          stroke={arcColor(clamped)}
          strokeWidth={14}
          strokeLinecap="round"
          strokeDasharray={`${dash} ${circumference - dash}`}
          transform={`rotate(-90 ${size / 2} ${size / 2})`}
          style={{
            transition: "stroke-dasharray 1200ms var(--ease-out-premium)",
            filter: "drop-shadow(0 0 12px rgba(0, 220, 255, 0.2))"
          }}
        />
        <circle
          cx={size / 2}
          cy={size / 2}
          r={radius - 22}
          fill="rgba(6, 11, 22, 0.76)"
          stroke="rgba(255,255,255,0.05)"
          strokeWidth={1}
        />
        <text
          x="50%"
          y="48%"
          fill="var(--text-primary)"
          textAnchor="middle"
          dominantBaseline="middle"
          className="font-display"
          style={{ fontSize: 38, fontWeight: 800, letterSpacing: "-0.03em" }}
        >
          {clamped.toFixed(0)}
        </text>
        <text
          x="50%"
          y="61%"
          fill="var(--text-tertiary)"
          textAnchor="middle"
          className="font-mono"
          style={{ fontSize: 10, letterSpacing: "0.18em", textTransform: "uppercase" }}
        >
          HEALTH
        </text>
      </svg>
      <div
        className="font-mono"
        style={{
          marginTop: -2,
          fontSize: 10,
          letterSpacing: "0.16em",
          color: "var(--text-tertiary)"
        }}
      >
        LIVE SCORE
      </div>
    </div>
  );
}
