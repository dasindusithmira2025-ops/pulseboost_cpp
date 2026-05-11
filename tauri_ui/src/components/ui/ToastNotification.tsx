import { useEffect } from "react";
import { useSystemStore } from "../../stores/systemStore";
import type { NotificationItem } from "../../types/system";

const borderByType: Record<NotificationItem["type"], string> = {
  info: "var(--accent)",
  ok: "var(--status-ok)",
  warn: "var(--status-warn)",
  danger: "var(--status-danger)"
};

export default function ToastNotification() {
  const notifications = useSystemStore((s) => s.notifications);
  const clear = useSystemStore((s) => s.clearNotification);

  useEffect(() => {
    const timers = notifications.map((n) => window.setTimeout(() => clear(n.id), 4200));
    return () => {
      timers.forEach((id) => window.clearTimeout(id));
    };
  }, [clear, notifications]);

  return (
    <div
      style={{
        position: "fixed",
        right: "var(--sp-5)",
        bottom: "calc(var(--sp-10) + 12px)",
        zIndex: 30,
        display: "flex",
        flexDirection: "column",
        gap: "var(--sp-2)",
        width: 340,
        pointerEvents: "none"
      }}
    >
      {notifications.map((n) => (
        <article
          key={n.id}
          className="glass-card shell-panel"
          style={{
            padding: "var(--sp-3) var(--sp-4)",
            borderLeft: `3px solid ${borderByType[n.type]}`,
            animation: "toastIn var(--dur-layout) var(--ease-out-premium) both",
            pointerEvents: "auto",
            overflow: "hidden"
          }}
        >
          <div className="title-kicker" style={{ marginBottom: 6, color: borderByType[n.type] }}>
            {n.type}
          </div>
          <div className="font-body" style={{ fontSize: 13, lineHeight: 1.45 }}>
            {n.message}
          </div>
          <div
            style={{
              marginTop: "var(--sp-3)",
              height: 3,
              borderRadius: 999,
              background: "rgba(255,255,255,0.06)",
              overflow: "hidden"
            }}
          >
            <div
              style={{
                height: "100%",
                width: "100%",
                background: borderByType[n.type],
                animation: "progressBar 4.2s linear forwards"
              }}
            />
          </div>
        </article>
      ))}
    </div>
  );
}
