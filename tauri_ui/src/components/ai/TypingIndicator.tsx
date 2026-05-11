export default function TypingIndicator() {
  return (
    <div
      className="glass-card"
      style={{
        display: "inline-flex",
        alignItems: "center",
        gap: "var(--sp-3)",
        padding: "var(--sp-2) var(--sp-3)",
        borderRadius: "var(--r-xl)"
      }}
    >
      <span className="title-kicker" style={{ color: "var(--text-secondary)" }}>
        Thinking
      </span>
      <div style={{ display: "inline-flex", gap: "var(--sp-2)", alignItems: "center" }}>
        {[0, 1, 2].map((i) => (
          <span
            key={i}
            style={{
              width: 6,
              height: 6,
              borderRadius: "50%",
              background: "var(--accent)",
              animation: `pulseDot 900ms ease-in-out ${i * 120}ms infinite`
            }}
          />
        ))}
      </div>
    </div>
  );
}
