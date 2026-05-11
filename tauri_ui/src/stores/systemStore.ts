import { create } from "zustand";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import type {
  ActionCompletePayload,
  ActionProgressItem,
  ActionProgressPayload,
  ActionProof,
  NotificationItem,
  SystemSnapshot
} from "../types/system";

interface SystemStore {
  snapshot: SystemSnapshot | null;
  isLoading: boolean;
  error: string | null;
  notifications: NotificationItem[];
  actionProgress: Record<string, ActionProgressItem>;
  lastProof: ActionProof | null;
  history: {
    cpu: number[];
    ram: number[];
    disk: number[];
    net: number[];
    labels: string[];
  };
  setSnapshot: (snapshot: SystemSnapshot) => void;
  fetchSnapshot: () => Promise<void>;
  pushNotification: (message: string, type?: NotificationItem["type"]) => void;
  clearNotification: (id: string) => void;
  setActionProgress: (payload: ActionProgressPayload) => void;
  setActionComplete: (payload: ActionCompletePayload) => void;
  clearActionProgress: (action: string) => void;
  clearLastProof: () => void;
}

const maxHistoryPoints = 120;

function nextHistory(prev: SystemStore["history"], snapshot: SystemSnapshot) {
  const label = new Date(snapshot.timestamp).toLocaleTimeString([], {
    hour12: false,
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });

  const cpu = [...prev.cpu, snapshot.cpuPercent].slice(-maxHistoryPoints);
  const ram = [...prev.ram, snapshot.ramPercent].slice(-maxHistoryPoints);
  const disk = [...prev.disk, snapshot.diskPercent].slice(-maxHistoryPoints);
  const net = [...prev.net, snapshot.netDownloadKbps / 1024].slice(-maxHistoryPoints);
  const labels = [...prev.labels, label].slice(-maxHistoryPoints);

  return { cpu, ram, disk, net, labels };
}

export const useSystemStore = create<SystemStore>((set, get) => ({
  snapshot: null,
  isLoading: true,
  error: null,
  notifications: [],
  actionProgress: {},
  lastProof: null,
  history: {
    cpu: [],
    ram: [],
    disk: [],
    net: [],
    labels: []
  },
  setSnapshot: (snapshot) =>
    set((state) => ({
      snapshot,
      isLoading: false,
      error: null,
      history: nextHistory(state.history, snapshot)
    })),
  fetchSnapshot: async () => {
    const hadSnapshot = get().snapshot !== null;
    set({ isLoading: !hadSnapshot, error: null });
    try {
      if (!isTauriRuntime()) {
        throw new Error("Tauri runtime unavailable. Launch via Tauri shell.");
      }
      const snapshot = await tauriInvoke<SystemSnapshot>("get_snapshot");
      get().setSnapshot(snapshot);
    } catch (error) {
      const message = error instanceof Error ? error.message : "Failed to fetch snapshot";
      set({ isLoading: false, error: message });
      get().pushNotification(message, "danger");
    }
  },
  pushNotification: (message, type = "info") =>
    set((state) => ({
      notifications: [
        ...state.notifications,
        {
          id: `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`,
          message,
          type,
          createdAt: Date.now()
        }
      ].slice(-15)
    })),
  clearNotification: (id) =>
    set((state) => ({
      notifications: state.notifications.filter((n) => n.id !== id)
    })),
  setActionProgress: (payload) =>
    set((state) => ({
      actionProgress: {
        ...state.actionProgress,
        [payload.action]: {
          action: payload.action,
          percent: Math.max(0, Math.min(100, payload.percent)),
          message: payload.message,
          startedAt: state.actionProgress[payload.action]?.startedAt ?? Date.now(),
          completedAt: undefined,
          success: undefined,
          result: undefined
        }
      }
    })),
  setActionComplete: (payload) =>
    set((state) => {
      const proofSource = payload.result as { proof?: unknown } | null;
      const proof = Array.isArray(proofSource?.proof)
        ? proofSource?.proof.filter((entry): entry is string => typeof entry === "string")
        : [];
      return {
        actionProgress: {
          ...state.actionProgress,
          [payload.action]: {
            action: payload.action,
            percent: payload.success ? 100 : state.actionProgress[payload.action]?.percent ?? 100,
            message: payload.success ? "Completed" : "Failed",
            startedAt: state.actionProgress[payload.action]?.startedAt ?? Date.now(),
            completedAt: Date.now(),
            success: payload.success,
            result: payload.result
          }
        },
        lastProof: proof.length > 0
          ? {
              action: payload.action,
              lines: proof,
              capturedAt: Date.now()
            }
          : state.lastProof
      };
    }),
  clearActionProgress: (action) =>
    set((state) => {
      const next = { ...state.actionProgress };
      delete next[action];
      return { actionProgress: next };
    }),
  clearLastProof: () => set({ lastProof: null })
}));
