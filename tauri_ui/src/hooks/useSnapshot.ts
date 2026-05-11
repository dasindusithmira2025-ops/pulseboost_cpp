import { useEffect } from "react";
import { isTauriRuntime, tauriListen } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type {
  ActionCompletePayload,
  ActionProgressPayload,
  SystemSnapshot
} from "../types/system";

function prettyActionName(action: string): string {
  return action.split("_").join(" ");
}

export function useSnapshot(): void {
  const setSnapshot = useSystemStore((s) => s.setSnapshot);
  const fetchSnapshot = useSystemStore((s) => s.fetchSnapshot);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const setActionProgress = useSystemStore((s) => s.setActionProgress);
  const setActionComplete = useSystemStore((s) => s.setActionComplete);
  const clearActionProgress = useSystemStore((s) => s.clearActionProgress);

  useEffect(() => {
    const tauri = isTauriRuntime();
    void fetchSnapshot();

    let cancelled = false;
    const unlisteners: Array<() => void> = [];

    const register = async <T,>(
      eventName: string,
      handler: (payload: T) => void
    ) => {
      const unlisten = await tauriListen<T>(eventName, (event) => {
        if (!cancelled) {
          handler(event.payload);
        }
      });
      if (!cancelled) {
        unlisteners.push(unlisten);
      } else {
        await unlisten();
      }
    };

    void register<SystemSnapshot>("snapshot_ready", (payload) => {
      setSnapshot(payload);
    });

    void register<ActionProgressPayload>("action_progress", (payload) => {
      setActionProgress(payload);
    });

    void register<ActionCompletePayload>("action_complete", (payload) => {
      setActionComplete(payload);
      pushNotification(
        payload.success
          ? `${prettyActionName(payload.action)} completed`
          : `${prettyActionName(payload.action)} failed`,
        payload.success ? "ok" : "danger"
      );
      window.setTimeout(() => clearActionProgress(payload.action), payload.success ? 1800 : 6000);
    });

    const timer = tauri
      ? null
      : window.setInterval(() => {
          if (!cancelled) {
            void fetchSnapshot();
          }
        }, 2000);

    return () => {
      cancelled = true;
      if (timer !== null) {
        window.clearInterval(timer);
      }
      for (const unlisten of unlisteners) {
        void unlisten();
      }
    };
  }, [clearActionProgress, fetchSnapshot, pushNotification, setActionComplete, setActionProgress, setSnapshot]);
}

