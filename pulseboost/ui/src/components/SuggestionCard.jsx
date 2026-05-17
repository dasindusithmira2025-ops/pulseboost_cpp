import { useMemo, useState } from "react";
import { XIcon } from "lucide-react";

function toneClass(severity) {
  if (severity === "warning") return "border-l-[#F59E0B]";
  return "border-l-[#2D7FF9]";
}

export default function SuggestionCard({ icon: Icon, text, action, severity = "info", onDismiss }) {
  const [dismissed, setDismissed] = useState(false);
  const className = useMemo(() => toneClass(severity), [severity]);

  const handleDismiss = () => {
    setDismissed(true);
    window.setTimeout(() => {
      onDismiss?.();
    }, 150);
  };

  return (
    <article
      className={[
        "overflow-hidden rounded-[12px] border border-border-default border-l-2 bg-surface-elevated transition-all duration-200 ease-out",
        className,
        dismissed ? "max-h-0 opacity-0" : "max-h-40 opacity-100",
      ].join(" ")}
    >
      <div className="flex items-start gap-3 p-4">
        <div className="mt-0.5 shrink-0 text-txt-secondary">
          {Icon ? <Icon className="h-4 w-4" /> : null}
        </div>
        <div className="min-w-0 flex-1">
          <p className="text-sm text-txt-primary">{text}</p>
          {action?.label ? (
            <button
              type="button"
              onClick={action.onClick}
              className="mt-2 text-xs font-medium uppercase tracking-[0.12em] text-accent hover:text-accent-hover"
            >
              {action.label}
            </button>
          ) : null}
        </div>
        <button
          type="button"
          onClick={handleDismiss}
          className="rounded-md p-1 text-txt-tertiary transition-colors hover:bg-surface-hover hover:text-txt-secondary"
          aria-label="Dismiss suggestion"
        >
          <XIcon className="h-3.5 w-3.5" />
        </button>
      </div>
    </article>
  );
}

