import type { CSSProperties, ReactNode } from "react";

type Variant = "primary" | "ghost" | "danger";

interface GlowButtonProps {
  label: string;
  variant?: Variant;
  icon?: ReactNode;
  onClick?: () => void;
  loading?: boolean;
  disabled?: boolean;
  title?: string;
}

function variantStyle(variant: Variant): CSSProperties {
  if (variant === "ghost") {
    return {
      border: "1px solid var(--border-glass-md)",
      background: "linear-gradient(180deg, rgba(255,255,255,0.045) 0%, rgba(255,255,255,0.02) 100%)",
      color: "var(--text-secondary)",
      boxShadow: "inset 0 1px 0 rgba(255,255,255,0.04)"
    };
  }
  if (variant === "danger") {
    return {
      border: "1px solid rgba(255, 93, 115, 0.34)",
      background: "linear-gradient(180deg, rgba(255, 93, 115, 0.18) 0%, rgba(255, 93, 115, 0.08) 100%)",
      color: "var(--text-primary)",
      boxShadow: "0 14px 30px rgba(255, 93, 115, 0.14)"
    };
  }
  return {
    border: "1px solid var(--border-accent)",
    background: "linear-gradient(135deg, rgba(0,220,255,0.24) 0%, rgba(122,92,255,0.18) 100%)",
    color: "var(--text-primary)",
    boxShadow: "0 16px 36px rgba(0, 220, 255, 0.18), inset 0 1px 0 rgba(255,255,255,0.08)"
  };
}

export default function GlowButton({
  label,
  variant = "primary",
  icon,
  onClick,
  loading,
  disabled,
  title
}: GlowButtonProps) {
  return (
    <button
      className="focus-ring interactive"
      title={title}
      type="button"
      disabled={disabled || loading}
      onClick={onClick}
      style={{
        ...variantStyle(variant),
        minHeight: 38,
        borderRadius: "var(--r-lg)",
        padding: "0 var(--sp-4)",
        display: "inline-flex",
        alignItems: "center",
        justifyContent: "center",
        gap: "var(--sp-2)",
        cursor: disabled || loading ? "not-allowed" : "pointer",
        opacity: disabled ? 0.45 : 1,
        fontSize: 13,
        fontWeight: 600,
        letterSpacing: "0.01em",
        whiteSpace: "nowrap"
      }}
    >
      {icon}
      <span>{loading ? "Working..." : label}</span>
    </button>
  );
}
