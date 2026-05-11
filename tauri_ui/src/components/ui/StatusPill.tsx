type Status = "ok" | "warn" | "danger" | "info" | "neutral";

interface StatusPillProps {
  status: Status;
  label: string;
  dot?: boolean;
}

function tone(status: Status): { bg: string; border: string; text: string } {
  switch (status) {
    case "ok":
      return {
        bg: "linear-gradient(180deg, rgba(0, 229, 160, 0.16) 0%, rgba(0, 229, 160, 0.08) 100%)",
        border: "rgba(0, 229, 160, 0.32)",
        text: "var(--status-ok)"
      };
    case "warn":
      return {
        bg: "linear-gradient(180deg, rgba(255, 191, 71, 0.16) 0%, rgba(255, 191, 71, 0.08) 100%)",
        border: "rgba(255, 191, 71, 0.28)",
        text: "var(--status-warn)"
      };
    case "danger":
      return {
        bg: "linear-gradient(180deg, rgba(255, 93, 115, 0.16) 0%, rgba(255, 93, 115, 0.08) 100%)",
        border: "rgba(255, 93, 115, 0.28)",
        text: "var(--status-danger)"
      };
    case "info":
      return {
        bg: "linear-gradient(180deg, rgba(0, 220, 255, 0.18) 0%, rgba(0, 220, 255, 0.08) 100%)",
        border: "rgba(0, 220, 255, 0.28)",
        text: "var(--text-accent)"
      };
    default:
      return {
        bg: "linear-gradient(180deg, rgba(255,255,255,0.05) 0%, rgba(255,255,255,0.025) 100%)",
        border: "var(--border-glass)",
        text: "var(--text-secondary)"
      };
  }
}

export default function StatusPill({ status, label, dot = false }: StatusPillProps) {
  const t = tone(status);
  return (
    <span
      style={{
        display: "inline-flex",
        alignItems: "center",
        gap: "var(--sp-2)",
        minHeight: 25,
        padding: "0 var(--sp-3)",
        borderRadius: 999,
        border: `1px solid ${t.border}`,
        background: t.bg,
        color: t.text,
        fontFamily: "JetBrains Mono, monospace",
        fontSize: 10,
        letterSpacing: "0.08em",
        textTransform: "uppercase",
        boxShadow: "inset 0 1px 0 rgba(255,255,255,0.04)"
      }}
    >
      {dot ? (
        <span
          className="status-dot"
          style={{ background: t.text, width: 6, height: 6, animationDuration: "1000ms" }}
        />
      ) : null}
      {label}
    </span>
  );
}
