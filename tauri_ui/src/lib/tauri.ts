import type { Event, UnlistenFn } from "@tauri-apps/api/event";
import { listen as tauriListenRaw } from "@tauri-apps/api/event";
import { invoke as tauriInvokeRaw } from "@tauri-apps/api/tauri";

function hasTauriIpc(): boolean {
  return (
    typeof window !== "undefined" &&
    "__TAURI_IPC__" in window &&
    typeof (window as { __TAURI_IPC__?: unknown }).__TAURI_IPC__ !== "undefined"
  );
}

export async function tauriInvoke<T>(
  command: string,
  args?: Record<string, unknown>
): Promise<T> {
  if (!hasTauriIpc()) {
    throw new Error(`Tauri unavailable for command: ${command}`);
  }
  const timeoutMs = 15000;
  let timeoutId = 0;
  const timeout = new Promise<never>((_, reject) => {
    timeoutId = window.setTimeout(() => {
      reject(new Error(`Command timed out: ${command}`));
    }, timeoutMs);
  });
  try {
    return await Promise.race([tauriInvokeRaw<T>(command, args), timeout]);
  } finally {
    if (timeoutId) {
      window.clearTimeout(timeoutId);
    }
  }
}

export async function tauriListen<T>(
  eventName: string,
  handler: (event: Event<T>) => void
): Promise<UnlistenFn> {
  if (!hasTauriIpc()) {
    return async () => {};
  }
  return tauriListenRaw<T>(eventName, handler);
}

export function isTauriRuntime(): boolean {
  return hasTauriIpc();
}
