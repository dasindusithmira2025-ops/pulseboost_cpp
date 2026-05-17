import { useMemo, useState } from "react";

function toneFromScore(score) {
  if (score >= 75) return "#22C55E";
  if (score >= 50) return "#F59E0B";
  return "#EF4444";
}

function formatPointTimestamp(timestamp) {
  if (!timestamp) return "Unknown";
  return new Date(Number(timestamp) * 1000).toLocaleDateString();
}

function buildSmoothPath(points) {
  if (points.length < 2) return "";
  let path = `M ${points[0].x} ${points[0].y}`;
  for (let index = 0; index < points.length - 1; index += 1) {
    const current = points[index];
    const next = points[index + 1];
    const controlX = (current.x + next.x) / 2;
    path += ` C ${controlX} ${current.y}, ${controlX} ${next.y}, ${next.x} ${next.y}`;
  }
  return path;
}

export default function SparklineChart({ data = [], width = 520, height = 120, color }) {
  const [hoverIndex, setHoverIndex] = useState(null);
  const normalized = useMemo(() => {
    const points = (data || [])
      .map((entry) => ({
        timestamp: Number(entry.timestamp || 0),
        score: Number(entry.score ?? entry.health_score ?? 0),
      }))
      .filter((entry) => Number.isFinite(entry.timestamp) && Number.isFinite(entry.score))
      .sort((a, b) => a.timestamp - b.timestamp);

    if (!points.length) return [];
    const xSpan = Math.max(points[points.length - 1].timestamp - points[0].timestamp, 1);
    const mapped = points.map((point) => {
      const x = ((point.timestamp - points[0].timestamp) / xSpan) * width;
      const y = height - (Math.max(0, Math.min(100, point.score)) / 100) * height;
      return { ...point, x, y };
    });
    return mapped;
  }, [data, height, width]);

  if (!normalized.length) {
    return (
      <div className="rounded-[12px] border border-dashed border-border-default px-3 py-4 text-xs text-txt-tertiary">
        Health history will appear after enough samples are collected.
      </div>
    );
  }

  const latestScore = normalized[normalized.length - 1].score;
  const stroke = color || toneFromScore(latestScore);
  const path = buildSmoothPath(normalized);
  const areaPath = `${path} L ${width} ${height} L 0 ${height} Z`;
  const activePoint = hoverIndex === null ? null : normalized[hoverIndex];

  return (
    <div className="relative">
      <svg
        width={width}
        height={height}
        viewBox={`0 0 ${width} ${height}`}
        className="w-full"
        onMouseLeave={() => setHoverIndex(null)}
      >
        <defs>
          <linearGradient id="health-sparkline-fill" x1="0" x2="0" y1="0" y2="1">
            <stop offset="0%" stopColor={stroke} stopOpacity="0.3" />
            <stop offset="100%" stopColor={stroke} stopOpacity="0" />
          </linearGradient>
        </defs>
        <path d={areaPath} fill="url(#health-sparkline-fill)" />
        <path d={path} fill="none" stroke={stroke} strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
        {normalized.map((point, index) => (
          <circle
            key={`${point.timestamp}-${index}`}
            cx={point.x}
            cy={point.y}
            r={hoverIndex === index ? 3 : 2}
            fill={stroke}
            opacity={hoverIndex === null || hoverIndex === index ? 1 : 0.5}
            onMouseMove={() => setHoverIndex(index)}
          />
        ))}
      </svg>
      {activePoint ? (
        <div
          className="pointer-events-none absolute rounded-md border border-border-default bg-surface px-2 py-1 text-[11px] text-txt-secondary"
          style={{
            left: `${Math.min(width - 120, Math.max(0, activePoint.x - 36))}px`,
            top: `${Math.max(0, activePoint.y - 36)}px`,
          }}
        >
          <div className="numeric-tabular text-txt-primary">{Math.round(activePoint.score)}/100</div>
          <div>{formatPointTimestamp(activePoint.timestamp)}</div>
        </div>
      ) : null}
    </div>
  );
}

