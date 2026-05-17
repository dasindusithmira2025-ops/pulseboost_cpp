import { useEffect, useRef } from "react";

import useSystemStore from "../store/useSystemStore";

const DEFAULT_URL = `${window.location.protocol === "https:" ? "wss" : "ws"}://${window.location.host}/ws`;

export function useWebSocket(url = DEFAULT_URL) {
  const socketRef = useRef(null);
  const reconnectTimerRef = useRef(null);
  const setConnected = useSystemStore((state) => state.setConnected);
  const setSystemUpdate = useSystemStore((state) => state.setSystemUpdate);
  const appendChatToken = useSystemStore((state) => state.appendChatToken);
  const finishChatStream = useSystemStore((state) => state.finishChatStream);
  const setErrorMessage = useSystemStore((state) => state.setErrorMessage);

  useEffect(() => {
    let cancelled = false;

    function connect() {
      if (cancelled) return;
      const socket = new WebSocket(url);
      socketRef.current = socket;

      socket.onopen = () => {
        setConnected(true);
        setErrorMessage("");
        socket.send(JSON.stringify({ type: "state_request" }));
      };

      socket.onmessage = (event) => {
        let payload;
        try {
          payload = JSON.parse(event.data);
        } catch {
          setErrorMessage("PulseBoost received an invalid live update frame.");
          return;
        }
        if (payload.type === "system_update" || payload.type === "full_state") {
          setSystemUpdate(payload);
        }
        if (payload.type === "chat_token") {
          appendChatToken(payload.token);
        }
        if (payload.type === "chat_done") {
          finishChatStream();
        }
        if (payload.type === "optimization_decision_result" && payload.result && payload.result.success === false) {
          setErrorMessage(payload.result.error || payload.result.thought?.abort_reason || "Optimization decision failed.");
        }
        if (payload.type === "system_error") {
          setErrorMessage(payload.message || "PulseBoost encountered an internal error.");
        }
        if (payload.type === "game_closed") {
          window.dispatchEvent(new CustomEvent("pulseboost:game-closed", { detail: payload }));
        }
      };

      socket.onclose = () => {
        setConnected(false);
        if (!cancelled) {
          reconnectTimerRef.current = window.setTimeout(connect, 1500);
        }
      };

      socket.onerror = () => {
        setConnected(false);
      };
    }

    connect();
    return () => {
      cancelled = true;
      window.clearTimeout(reconnectTimerRef.current);
      socketRef.current?.close();
    };
  }, [url, appendChatToken, finishChatStream, setConnected, setErrorMessage, setSystemUpdate]);

  function sendJson(payload) {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(JSON.stringify(payload));
    }
  }

  return {
    sendChat(message, history) {
      sendJson({ type: "chat", message, history });
    },
    sendOptimizationDecision(optimizationId, decision) {
      sendJson({ type: "optimization_decision", optimization_id: optimizationId, decision });
    },
  };
}
