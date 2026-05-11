import { useId, useMemo, useState } from "react";

interface LineChartProps {
  data: number[];
  labels?: string[];
  color?: string;
  secondaryData?: number[];
  secondaryColor?: string;
  height?: number;
}

function curvePath(points: Array<{ x: number; y: number }>): string {
  if (!points.length) {
    return "";
  }
  if (points.length === 1) {
    return `M ${points[0].x} ${points[0].y}`;
  }

  const path = [`M ${points[0].x} ${points[0].y}`];
  for (let i = 1; i < points.length; i += 1) {
    const prev = points[i - 1];
    const cur = points[i];
    const c1x = prev.x + (cur.x - prev.x) / 3;
    const c2x = cur.x - (cur.x - prev.x) / 3;
    path.push(`C ${c1x} ${prev.y}, ${c2x} ${cur.y}, ${cur.x} ${cur.y}`);
  }
  return path.join(" ");
}

export default function LineChart({
  data,
  labels = [],
  color = "var(--accent)",
  secondaryData,
  secondaryColor = "var(--accent2)",
  height = 210
}: LineChartProps) {
  const [hoverIndex, setHoverIndex] = useState<number | null>(null);
  const gradientId = useId().replace(/:/g, "");
  const width = 760;
  const pad = 24;

  const chart = useMemo(() => {
    const allValues = [...data, ...(secondaryData ?? [])];
    if (!allValues.length) {
      return {
        primaryPoints: [] as Array<{ x: number; y: number; value: number }>,
        secondaryPoints: [] as Array<{ x: number; y: number; value: number }>,
        primaryPath: "",
        secondaryPath: "",
        areaPath: "",
        min: 0,
        max: 0
      };
    }

    const minValue = Math.min(...allValues);
    const maxValue = Math.max(...allValues);
    const range = maxValue - minValue || 1;
    const usableW = width - pad * 2;
    const usableH = height - pad * 2;

    const buildPoints = (series: number[]) =>
      series.map((value, i) => {
        const x = pad + (i / Math.max(1, series.length - 1)) * usableW;
        const y = pad + (1 - (value - minValue) / range) * usableH;
        return { x, y, value };
      });

    const primaryPoints = buildPoints(data);
    const secondaryPoints = secondaryData?.length ? buildPoints(secondaryData) : [];
    const primaryPath = curvePath(primaryPoints);
    const secondaryPath = curvePath(secondaryPoints);
    const areaPath = primaryPoints.length
      ? `${primaryPath} L ${primaryPoints[primaryPoints.length - 1].x} ${height - pad} L ${primaryPoints[0].x} ${height - pad} Z`
      : "";

    return {
      primaryPoints,
      secondaryPoints,
      primaryPath,
      secondaryPath,
      areaPath,
      min: minValue,
      max: maxValue
    };
  }, [data, height, pad, secondaryData, width]);

  const activePrimary = hoverIndex !== null ? chart.primaryPoints[hoverIndex] : null;
  const activeSecondary = hoverIndex !== null ? chart.secondaryPoints[hoverIndex] : null;

  return (
    <div style={{ width: "100%", position: "relative" }}>
      <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", marginBottom: "var(--sp-3)", alignItems: "center" }}>
        <div className="badge-row">
          <span className="font-mono tiny" style={{ display: "inline-flex", alignItems: "center", gap: 6 }}>
            <span style={{ width: 8, height: 8, borderRadius: 999, background: color }} /> CPU
          </span>
          {secondaryData?.length ? (
            <span className="font-mono tiny" style={{ display: "inline-flex", alignItems: "center", gap: 6 }}>
              <span style={{ width: 8, height: 8, borderRadius: 999, background: secondaryColor }} /> RAM
            </span>
          ) : null}
        </div>
        <span className="font-mono tiny">{data.length} samples</span>
      </div>

      <svg
        width="100%"
        height={height}
        viewBox={`0 0 ${width} ${height}`}
        preserveAspectRatio="none"
        onMouseLeave={() => setHoverIndex(null)}
        onMouseMove={(e) => {
          const rect = (e.currentTarget as SVGSVGElement).getBoundingClientRect();
          const px = ((e.clientX - rect.left) / rect.width) * width;
          const idx = Math.round(((px - pad) / (width - pad * 2)) * Math.max(1, data.length - 1));
          setHoverIndex(Math.max(0, Math.min(data.length - 1, idx)));
        }}
        style={{ overflow: "visible" }}
      >
        <defs>
          <linearGradient id={`${gradientId}-fill`} x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stopColor={color} stopOpacity="0.32" />
            <stop offset="100%" stopColor={color} stopOpacity="0.02" />
          </linearGradient>
        </defs>
        <rect
          x={0}
          y={0}
          width={width}
          height={height}
          rx={18}
          fill="rgba(255,255,255,0.02)"
          stroke="rgba(255,255,255,0.08)"
        />
        {[0, 1, 2, 3, 4].map((i) => {
          const y = pad + ((height - pad * 2) / 4) * i;
          return (
            <line
              key={i}
              x1={pad}
              x2={width - pad}
              y1={y}
              y2={y}
              stroke="rgba(255,255,255,0.08)"
              strokeOpacity="0.55"
            />
          );
        })}
        <path d={chart.areaPath} fill={`url(#${gradientId}-fill)`} />
        <path d={chart.primaryPath} fill="none" stroke={color} strokeWidth={2.6} strokeLinecap="round" />
        {chart.secondaryPath ? (
          <path
            d={chart.secondaryPath}
            fill="none"
            stroke={secondaryColor}
            strokeWidth={2}
            strokeLinecap="round"
            strokeDasharray="7 5"
          />
        ) : null}
        {activePrimary ? (
          <>
            <line
              x1={activePrimary.x}
              x2={activePrimary.x}
              y1={pad}
              y2={height - pad}
              stroke="rgba(255,255,255,0.3)"
              strokeDasharray="4 5"
            />
            <circle cx={activePrimary.x} cy={activePrimary.y} r={4.5} fill={color} />
            {activeSecondary ? <circle cx={activeSecondary.x} cy={activeSecondary.y} r={4.5} fill={secondaryColor} /> : null}
          </>
        ) : null}
      </svg>
      {activePrimary ? (
        <div
          className="glass-card"
          style={{
            position: "absolute",
            left: `${(activePrimary.x / width) * 100}%`,
            top: 42,
            transform: "translateX(-50%)",
            padding: "var(--sp-2) var(--sp-3)",
            fontFamily: "JetBrains Mono, monospace",
            fontSize: 10,
            display: "grid",
            gap: 4,
            minWidth: 118,
            pointerEvents: "none"
          }}
        >
          <div className="tiny">{labels[hoverIndex ?? 0] ?? "--"}</div>
          <div style={{ color }}>CPU {activePrimary.value.toFixed(1)}</div>
          {activeSecondary ? <div style={{ color: secondaryColor }}>RAM {activeSecondary.value.toFixed(1)}</div> : null}
        </div>
      ) : null}
      <div
        style={{
          display: "flex",
          justifyContent: "space-between",
          marginTop: "var(--sp-2)",
          color: "var(--text-tertiary)",
          fontSize: 10,
          fontFamily: "JetBrains Mono, monospace"
        }}
      >
        <span>min {chart.min.toFixed(1)}</span>
        <span>max {chart.max.toFixed(1)}</span>
      </div>
    </div>
  );
}
