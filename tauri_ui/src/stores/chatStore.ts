import { create } from "zustand";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";

export interface ChatMessage {
  id: string;
  role: "user" | "assistant";
  content: string;
  timestamp: number;
  streaming?: boolean;
  actionId?: string;
  actionLabel?: string;
}

interface ChatStore {
  messages: ChatMessage[];
  isTyping: boolean;
  sessionId: string;
  sendMessage: (content: string) => Promise<void>;
  appendStreamChunk: (chunk: string) => void;
  finishStream: () => void;
  clear: () => void;
}

function makeId(): string {
  return `${Date.now()}-${Math.random().toString(36).slice(2, 9)}`;
}

export const useChatStore = create<ChatStore>((set, get) => ({
  messages: [],
  isTyping: false,
  sessionId: `session-${Date.now()}`,
  sendMessage: async (content) => {
    const userMessage: ChatMessage = {
      id: makeId(),
      role: "user",
      content,
      timestamp: Date.now()
    };
    const assistantMessage: ChatMessage = {
      id: makeId(),
      role: "assistant",
      content: "",
      timestamp: Date.now(),
      streaming: true
    };
    set((state) => ({
      messages: [...state.messages, userMessage, assistantMessage],
      isTyping: true
    }));

    try {
      if (!isTauriRuntime()) {
        throw new Error("AI chat requires Tauri runtime.");
      }
      await tauriInvoke<string>("ask_agent", { message: content });
    } catch (error) {
      const message = error instanceof Error ? error.message : "PulseModel request failed.";
      get().appendStreamChunk(`PulseModel error: ${message}`);
    } finally {
      get().finishStream();
    }
  },
  appendStreamChunk: (chunk) =>
    set((state) => {
      const messages = [...state.messages];
      const index = messages.length - 1;
      const last = messages[index];
      if (last?.role === "assistant") {
        messages[index] = {
          ...last,
          content: `${last.content}${chunk}`
        };
      }
      return { messages };
    }),
  finishStream: () =>
    set((state) => {
      const messages = [...state.messages];
      const index = messages.length - 1;
      const last = messages[index];
      if (last?.role === "assistant") {
        messages[index] = {
          ...last,
          content: last.content.trim(),
          streaming: false
        };
      }
      return { messages, isTyping: false };
    }),
  clear: () => set({ messages: [], isTyping: false })
}));
