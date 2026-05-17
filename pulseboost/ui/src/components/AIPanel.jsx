import { useEffect, useMemo, useRef, useState } from "react";
import { ChevronDownIcon, ChevronUpIcon, SendIcon, Trash2Icon } from "lucide-react";

import useSystemStore from "../store/useSystemStore";

function formatMessageTime(timestamp) {
  if (!timestamp) return "";
  return new Date(timestamp).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", hour12: false });
}

function escapeHtml(text) {
  return String(text)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

function renderInlineMarkdown(text) {
  return escapeHtml(text)
    .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
    .replace(/`([^`]+)`/g, '<code class="rounded bg-[#141416] px-1 py-0.5 font-mono text-[12px] text-[#D6E7FF]">$1</code>');
}

function parseMarkdownToHtml(content) {
  const chunks = String(content || "").split(/```([\s\S]*?)```/g);
  return chunks
    .map((chunk, index) => {
      if (index % 2 === 1) {
        return `<pre class="my-2 overflow-x-auto rounded-[12px] border border-border-default bg-[#101113] p-3 text-xs text-[#D6E7FF]"><code>${escapeHtml(chunk.trim())}</code></pre>`;
      }
      const lines = chunk
        .split("\n")
        .map((line) => line.trim())
        .filter(Boolean);
      if (!lines.length) return "";
      const bulletsOnly = lines.every((line) => line.startsWith("- ") || line.startsWith("* "));
      if (bulletsOnly) {
        const items = lines
          .map((line) => `<li>${renderInlineMarkdown(line.slice(2))}</li>`)
          .join("");
        return `<ul class="my-1 list-disc space-y-1 pl-4">${items}</ul>`;
      }
      return lines.map((line) => `<p class="my-1 whitespace-pre-wrap">${renderInlineMarkdown(line)}</p>`).join("");
    })
    .join("");
}

function TypingIndicator() {
  return (
    <div className="flex items-center gap-1.5 py-1">
      <span className="h-1.5 w-1.5 animate-pulse-dot rounded-full bg-[#2D7FF9]" style={{ animationDelay: "0ms" }} />
      <span className="h-1.5 w-1.5 animate-pulse-dot rounded-full bg-[#2D7FF9]" style={{ animationDelay: "120ms" }} />
      <span className="h-1.5 w-1.5 animate-pulse-dot rounded-full bg-[#2D7FF9]" style={{ animationDelay: "240ms" }} />
    </div>
  );
}

export default function AIPanel({
  sendChat,
  contextSummary = "Using your hardware context",
  contextDetails = "",
  suggestedPrompts = [],
}) {
  const textareaRef = useRef(null);
  const [message, setMessage] = useState("");
  const [contextExpanded, setContextExpanded] = useState(false);
  const chatMessages = useSystemStore((state) => state.chatMessages);
  const chatStreaming = useSystemStore((state) => state.chatStreaming);
  const currentChatToken = useSystemStore((state) => state.currentChatToken);
  const addUserMessage = useSystemStore((state) => state.addUserMessage);
  const startChatStream = useSystemStore((state) => state.startChatStream);
  const clearChat = useSystemStore((state) => state.clearChat);

  useEffect(() => {
    const handler = (event) => {
      const prompt = String(event?.detail?.prompt || "").trim();
      if (prompt) {
        setMessage(prompt);
      }
      window.setTimeout(() => textareaRef.current?.focus(), 30);
    };
    window.addEventListener("pulseboost:focus-chat-input", handler);
    return () => window.removeEventListener("pulseboost:focus-chat-input", handler);
  }, []);

  const hasMessages = chatMessages.length > 0;
  const visibleMessages = useMemo(() => chatMessages.slice(-40), [chatMessages]);
  const prompts = suggestedPrompts.slice(0, 3);

  function submit(nextMessage) {
    const trimmed = String(nextMessage || "").trim();
    if (!trimmed) return;
    addUserMessage(trimmed);
    startChatStream();
    sendChat(trimmed, chatMessages);
    setMessage("");
  }

  function clearChatHistory() {
    if (!hasMessages) return;
    if (!window.confirm("Clear all PulseAI chat messages?")) return;
    clearChat();
  }

  return (
    <section className="flex h-[320px] flex-col gap-3">
      <div className="flex items-center justify-between gap-3">
        <button
          type="button"
          className="inline-flex items-center gap-2 rounded-full border border-border-default bg-surface-sunken px-3 py-1 text-[11px] text-txt-secondary"
          onClick={() => setContextExpanded((current) => !current)}
        >
          <span>{contextSummary}</span>
          {contextExpanded ? <ChevronUpIcon className="h-3.5 w-3.5" /> : <ChevronDownIcon className="h-3.5 w-3.5" />}
        </button>
        <button
          type="button"
          onClick={clearChatHistory}
          className="rounded-md border border-border-default p-1.5 text-txt-tertiary transition-colors hover:bg-surface-hover hover:text-txt-secondary"
          aria-label="Clear chat"
        >
          <Trash2Icon className="h-3.5 w-3.5" />
        </button>
      </div>
      {contextExpanded && contextDetails ? (
        <div className="rounded-[12px] border border-border-default bg-surface-sunken px-3 py-2 text-xs text-txt-secondary">
          {contextDetails}
        </div>
      ) : null}

      {!hasMessages && !chatStreaming ? (
        <div className="flex flex-wrap gap-2">
          {prompts.map((prompt) => (
            <button
              key={prompt}
              type="button"
              onClick={() => submit(prompt)}
              className="rounded-full border border-border-default px-3 py-1 text-xs text-txt-tertiary transition-colors hover:border-[#2D7FF9] hover:text-[#2D7FF9]"
            >
              {prompt}
            </button>
          ))}
        </div>
      ) : null}

      <div className="min-h-0 flex-1 overflow-y-auto rounded-[12px] border border-border-default bg-surface-sunken p-3">
        {!hasMessages && !chatStreaming ? (
          <div className="text-sm text-txt-tertiary">
            Ask PulseAI about thermals, bottlenecks, latency, or game-specific optimization.
          </div>
        ) : null}
        <div className="space-y-3">
          {visibleMessages.map((item, index) => (
            <div key={`${item.role}-${item.timestamp || index}`} className={item.role === "user" ? "ml-auto max-w-[78%]" : "max-w-[78%]"}>
              <div
                className={[
                  "rounded-[12px] border px-3 py-2 text-sm",
                  item.role === "user"
                    ? "border-[#2D7FF9] bg-[#2D7FF9]/15 text-[#EAF3FF]"
                    : "border-border-default bg-[#111214] text-txt-primary",
                ].join(" ")}
                dangerouslySetInnerHTML={{ __html: parseMarkdownToHtml(item.content) }}
              />
              <div className="mt-1 text-[11px] text-txt-tertiary">{formatMessageTime(item.timestamp)}</div>
            </div>
          ))}
          {chatStreaming ? (
            <div className="max-w-[78%]">
              <div className="rounded-[12px] border border-border-default bg-[#111214] px-3 py-2 text-sm text-txt-primary">
                {currentChatToken ? (
                  <div dangerouslySetInnerHTML={{ __html: parseMarkdownToHtml(currentChatToken) }} />
                ) : (
                  <TypingIndicator />
                )}
              </div>
              <div className="mt-1 text-[11px] text-txt-tertiary">{formatMessageTime(Date.now())}</div>
            </div>
          ) : null}
        </div>
      </div>

      <form
        className="flex items-end gap-2 rounded-[12px] border border-border-default bg-surface px-2 py-2"
        onSubmit={(event) => {
          event.preventDefault();
          submit(message);
        }}
      >
        <textarea
          ref={textareaRef}
          className="min-h-[36px] max-h-[120px] min-w-0 flex-1 resize-none overflow-y-auto bg-transparent px-2 py-1 text-sm text-txt-primary placeholder:text-txt-tertiary focus:outline-none"
          value={message}
          onChange={(event) => setMessage(event.target.value)}
          onKeyDown={(event) => {
            if (event.key === "Enter" && !event.shiftKey) {
              event.preventDefault();
              submit(message);
            }
          }}
          placeholder="Ask PulseAI anything about your system..."
        />
        <button
          className="rounded-[10px] bg-[#2D7FF9] p-2 text-white transition-colors hover:bg-[#3E89FA]"
          type="submit"
          aria-label="Send message"
        >
          <SendIcon className="h-3.5 w-3.5" />
        </button>
      </form>
    </section>
  );
}

