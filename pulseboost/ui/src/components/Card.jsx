export default function Card({
  children,
  className = "",
  elevated = false,
  statusColor,
  onClick,
}) {
  return (
    <div
      onClick={onClick}
      className={[
        "rounded-lg border border-border-default bg-surface",
        elevated ? "glass-panel" : "",
        onClick ? "cursor-pointer transition-colors hover:bg-surface-hover" : "",
        statusColor ? "border-l-[3px]" : "",
        className,
      ].join(" ")}
      style={statusColor ? { borderLeftColor: statusColor } : undefined}
    >
      {children}
    </div>
  );
}
