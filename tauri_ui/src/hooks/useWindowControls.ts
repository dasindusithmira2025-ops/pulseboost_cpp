import { appWindow } from "@tauri-apps/api/window";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";

export function useWindowControls() {
  const startDragging = async () => {
    if (isTauriRuntime()) {
      await appWindow.startDragging();
    }
  };

  const minimize = async () => {
    if (isTauriRuntime()) {
      await appWindow.minimize();
    }
  };

  const maximize = async () => {
    if (isTauriRuntime()) {
      await appWindow.toggleMaximize();
    }
  };

  const closeApp = async () => {
    if (isTauriRuntime()) {
      await appWindow.close();
    }
  };

  const refreshAll = async () => {
    if (isTauriRuntime()) {
      try {
        await tauriInvoke("refresh_all");
      } catch {
        // UI fallback refresh event still runs below.
      }
    }
    window.dispatchEvent(new CustomEvent("pulseboost-refresh"));
  };

  return { startDragging, minimize, maximize, closeApp, refreshAll };
}
