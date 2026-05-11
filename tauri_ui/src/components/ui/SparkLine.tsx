interface SparkLineProps {
  data: number[];
  color?: string;
  width?: number;
  height?: number;
}

function makePath(data: number[], width: number, height: number): string {
  if (!data.length) {
    return "";
  }
  const max = Math.max(...data);
  const min = Math.min(...data);
  const range = max - min || 1;
  return data
    .map((v, i) => {
      const x = (i / Math.max(1, data.length - 1)) * width;
      const y = height - ((v - min) / range) * height;
      return `${i === 0 ? "M" : "L"} ${x.toFixed(2)} ${y.toFixed(2)}`;
    })
    .join(" ");
}

export default function SparkLine({
  data,
  color = "var(--accent)",
  width = 60,
  height = 24
}: SparkLineProps) {
  return (
    <svg width={width} height={height} viewBox={`0 0 ${width} ${height}`}>
      <path
        d={makePath(data, width, height)}
        fill="none"
        stroke={color}
        strokeWidth="2"
        strokeLinecap="round"
      />
    </svg>
  );
}
