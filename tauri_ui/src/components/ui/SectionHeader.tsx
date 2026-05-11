import type { ReactNode } from "react";

interface SectionHeaderProps {
  title: string;
  subtitle?: string;
  action?: ReactNode;
  eyebrow?: string;
}

export default function SectionHeader({ title, subtitle, action, eyebrow = "PulseBoost AI" }: SectionHeaderProps) {
  return (
    <header
      style={{
        display: "flex",
        alignItems: "flex-start",
        justifyContent: "space-between",
        gap: "var(--sp-4)",
        paddingBottom: "var(--sp-3)",
        marginBottom: "var(--sp-4)",
        borderBottom: "1px solid rgba(255, 255, 255, 0.08)"
      }}
    >
      <div style={{ minWidth: 0 }}>
        <div className="title-kicker" style={{ marginBottom: 6 }}>
          {eyebrow}
        </div>
        <div className="font-display" style={{ fontSize: 20, fontWeight: 700, lineHeight: 1.1 }}>
          {title}
        </div>
        {subtitle ? (
          <div className="font-body" style={{ color: "var(--text-secondary)", fontSize: 12, marginTop: 6, maxWidth: 720 }}>
            {subtitle}
          </div>
        ) : null}
      </div>
      {action ? <div style={{ flexShrink: 0 }}>{action}</div> : null}
    </header>
  );
}
