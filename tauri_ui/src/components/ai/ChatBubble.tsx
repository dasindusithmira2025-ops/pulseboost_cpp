import { format } from "date-fns";
import GlowButton from "../ui/GlowButton";
import StreamingText from "./StreamingText";

interface ChatBubbleProps {
  role: "user" | "assistant";
  content: string;
  timestamp: number;
  streaming?: boolean;
  actionId?: string;
  actionLabel?: string;
  onAction?: (actionId: string) => void;
}

export default function ChatBubble({
  role,
  content,
  timestamp,
  streaming,
  actionId,
  actionLabel,
  onAction
}: ChatBubbleProps) {
  const user = role === "user";

  return (
    <div
      style={{
        display: "flex",
        justifyContent: user ? "flex-end" : "flex-start",
        marginBottom: "var(--sp-3)"
      }}
    >
      <div style={{ maxWidth: "78%", minWidth: user ? 0 : 240 }}>
        <div className="title-kicker" style={{ marginBottom: 6, textAlign: user ? "right" : "left" }}>
          {user ? "You" : streaming ? "PulseModel streaming" : "PulseModel"}
        </div>
        <article
          className="screen-enter"
          style={{
            background: user
              ? "linear-gradient(135deg, rgba(0,220,255,0.18) 0%, rgba(122,92,255,0.12) 100%)"
              : "linear-gradient(180deg, rgba(255,255,255,0.055) 0%, rgba(255,255,255,0.028) 100%)",
            border: `1px solid ${user ? "rgba(0,220,255,0.24)" : "var(--border-glass)"}`,
            borderLeft: user ? "1px solid rgba(0,220,255,0.24)" : "3px solid var(--accent)",
            borderRadius: user ? "var(--r-xl) var(--r-xl) var(--r-md) var(--r-xl)" : "var(--r-xl) var(--r-xl) var(--r-xl) var(--r-md)",
            backdropFilter: "var(--blur-md)",
            WebkitBackdropFilter: "var(--blur-md)",
            padding: "var(--sp-3) var(--sp-4)",
            boxShadow: user ? "0 14px 36px rgba(0,220,255,0.12)" : "0 12px 32px rgba(0,0,0,0.18)"
          }}
        >
          <StreamingText content={content} streaming={streaming} />
          {!streaming && actionId ? (
            <div style={{ marginTop: "var(--sp-3)", display: "flex", justifyContent: user ? "flex-end" : "flex-start" }}>
              <GlowButton
                label={actionLabel ?? "Apply"}
                variant="primary"
                onClick={() => onAction?.(actionId)}
              />
            </div>
          ) : null}
        </article>
        <div
          className="font-mono"
          style={{
            color: "var(--text-tertiary)",
            fontSize: 10,
            marginTop: "var(--sp-1)",
            paddingLeft: user ? 0 : "var(--sp-1)",
            textAlign: user ? "right" : "left"
          }}
        >
          {format(timestamp, "HH:mm:ss")}
        </div>
      </div>
    </div>
  );
}
