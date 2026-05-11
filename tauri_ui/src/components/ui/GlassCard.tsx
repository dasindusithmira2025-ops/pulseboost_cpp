import type { CSSProperties, PropsWithChildren, ReactNode } from "react";

type GlowType = "accent" | "ok" | "warn" | "danger" | "none";

interface GlassCardProps extends PropsWithChildren {
  className?: string;
  glow?: GlowType;
  onClick?: () => void;
  header?: ReactNode;
  style?: CSSProperties;
}

function glowStyle(glow: GlowType): CSSProperties {
  switch (glow) {
    case "accent":
      return {
        boxShadow: "0 0 0 1px rgba(0, 220, 255, 0.14), 0 20px 56px rgba(0, 220, 255, 0.12)",
        borderColor: "var(--border-accent)"
      };
    case "ok":
      return {
        boxShadow: "0 0 0 1px rgba(0, 229, 160, 0.16), 0 18px 52px rgba(0, 229, 160, 0.1)",
        borderColor: "rgba(0, 229, 160, 0.26)"
      };
    case "warn":
      return {
        boxShadow: "0 0 0 1px rgba(255, 191, 71, 0.16), 0 18px 52px rgba(255, 191, 71, 0.1)",
        borderColor: "rgba(255, 191, 71, 0.24)"
      };
    case "danger":
      return {
        boxShadow: "0 0 0 1px rgba(255, 93, 115, 0.16), 0 18px 52px rgba(255, 93, 115, 0.12)",
        borderColor: "rgba(255, 93, 115, 0.24)"
      };
    default:
      return {};
  }
}

export default function GlassCard({
  children,
  className,
  glow = "none",
  onClick,
  header,
  style
}: GlassCardProps) {
  return (
    <section
      className={`glass-card interactive ${className ?? ""}`.trim()}
      style={{
        padding: "var(--sp-4)",
        position: "relative",
        overflow: "hidden",
        ...glowStyle(glow),
        ...style
      }}
      onClick={onClick}
      aria-label="glass-card"
    >
      <div
        style={{
          position: "absolute",
          inset: 0,
          background:
            glow === "accent"
              ? "linear-gradient(135deg, rgba(0, 220, 255, 0.08), transparent 36%)"
              : glow === "ok"
                ? "linear-gradient(135deg, rgba(0, 229, 160, 0.07), transparent 34%)"
                : glow === "warn"
                  ? "linear-gradient(135deg, rgba(255, 191, 71, 0.08), transparent 34%)"
                  : glow === "danger"
                    ? "linear-gradient(135deg, rgba(255, 93, 115, 0.08), transparent 34%)"
                    : "linear-gradient(135deg, rgba(255, 255, 255, 0.03), transparent 30%)",
          pointerEvents: "none"
        }}
      />
      <div style={{ position: "relative", zIndex: 1 }}>
        {header}
        {children}
      </div>
    </section>
  );
}
