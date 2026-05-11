import { useMemo } from "react";

interface StreamingTextProps {
  content: string;
  streaming?: boolean;
}

export default function StreamingText({ content, streaming }: StreamingTextProps) {
  const lines = useMemo(() => content.split("\n"), [content]);
  return (
    <div style={{ lineHeight: 1.6, fontSize: 14 }}>
      {lines.map((line, idx) => (
        <div key={idx}>{line}</div>
      ))}
      {streaming ? (
        <span className="cursor-blink" style={{ marginLeft: 2 }}>
          ▋
        </span>
      ) : null}
    </div>
  );
}
