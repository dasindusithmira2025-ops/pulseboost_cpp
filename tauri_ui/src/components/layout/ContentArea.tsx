import type { ReactNode } from "react";
import { useUiStore } from "../../stores/uiStore";
import type { ScreenId } from "../../types/system";

interface ContentAreaProps {
  screens: Record<ScreenId, ReactNode>;
}

export default function ContentArea({ screens }: ContentAreaProps) {
  const active = useUiStore((s) => s.activeScreen);

  return (
    <main
      className="glass-card shell-panel screen-enter"
      style={{
        width: "100%",
        height: "100%",
        overflow: "hidden",
        position: "relative"
      }}
    >
      <div
        style={{
          position: "relative",
          zIndex: 1,
          width: "100%",
          height: "100%",
          overflow: "auto",
          padding: "var(--sp-4)"
        }}
      >
        {screens[active]}
      </div>
    </main>
  );
}
