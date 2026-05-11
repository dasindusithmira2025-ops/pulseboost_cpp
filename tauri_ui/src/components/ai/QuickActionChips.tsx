interface QuickActionChipsProps {
  chips: string[];
  onSelect: (chip: string) => void;
}

export default function QuickActionChips({ chips, onSelect }: QuickActionChipsProps) {
  return (
    <div
      style={{
        display: "flex",
        gap: "var(--sp-2)",
        flexWrap: "wrap"
      }}
    >
      {chips.map((chip, index) => (
        <button
          key={chip}
          type="button"
          className="focus-ring interactive screen-enter"
          onClick={() => onSelect(chip)}
          style={{
            borderRadius: 999,
            border: "1px solid var(--border-glass-md)",
            background: index === 0
              ? "linear-gradient(135deg, rgba(0,220,255,0.16), rgba(122,92,255,0.12))"
              : "linear-gradient(180deg, rgba(255,255,255,0.045), rgba(255,255,255,0.02))",
            color: index === 0 ? "var(--text-primary)" : "var(--text-secondary)",
            minHeight: 32,
            padding: "0 var(--sp-3)",
            cursor: "pointer",
            fontSize: 12,
            fontWeight: 500,
            animationDelay: `${index * 45}ms`
          }}
        >
          {chip}
        </button>
      ))}
    </div>
  );
}
