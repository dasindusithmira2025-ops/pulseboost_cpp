import type { CSSProperties } from "react";

interface IconProps {
  name:
    | "home"
    | "optimizations"
    | "boost"
    | "games"
    | "backup"
    | "ai"
    | "settings"
    | "refresh"
    | "notifications"
    | "minimize"
    | "maximize"
    | "close";
  size?: number;
  strokeWidth?: number;
  style?: CSSProperties;
}

function iconPath(name: IconProps["name"]) {
  switch (name) {
    case "home":
      return <path d="M4 4h7v7H4zM13 4h7v4h-7zM13 10h7v10h-7zM4 13h7v7H4z" />;
    case "optimizations":
      return <path d="M12 3l7 4v5c0 5-3.2 8-7 9-3.8-1-7-4-7-9V7l7-4zm0 5l-2.2 4H13l-2 4 5.2-6H13l2-2z" />;
    case "boost":
      return <path d="M12 3l2.1 5.4L20 9l-4.2 3.6L17 18l-5-3-5 3 1.2-5.4L4 9l5.9-.6L12 3z" />;
    case "games":
      return <path d="M7 8h10a3 3 0 012.9 2.3l1.2 4.7a2 2 0 01-3.5 1.7L15 14h-6l-2.6 4.7A2 2 0 013 15l1.2-4.7A3 3 0 017 8zm1.5 2.5v3m-1.5-1.5h3m7-1h.01M18.5 13h.01" />;
    case "backup":
      return <path d="M5 7h14v12H5zM8 7V5h8v2M9 12h6M12 9v6" />;
    case "ai":
      return <path d="M9 4h6a3 3 0 013 3v6a3 3 0 01-3 3h-1l-3 3-3-3H9a3 3 0 01-3-3V7a3 3 0 013-3zm-1 6h8m-8 3h5" />;
    case "settings":
      return <path d="M12 3l1.3 2.6 2.9.5-.9 2.8 2 2-2 2 .9 2.8-2.9.5L12 21l-1.3-2.6-2.9-.5.9-2.8-2-2 2-2-.9-2.8 2.9-.5L12 3zm0 6a3 3 0 100 6 3 3 0 000-6z" />;
    case "refresh":
      return <path d="M20 6v5h-5m-6 7a7 7 0 011.5-13.8A7 7 0 0119 9" />;
    case "notifications":
      return <path d="M12 4a4 4 0 014 4v2.5c0 .9.3 1.8.9 2.5l1.1 1.3H6l1.1-1.3c.6-.7.9-1.6.9-2.5V8a4 4 0 014-4zm-2 13a2 2 0 004 0" />;
    case "minimize":
      return <path d="M6 12h12" />;
    case "maximize":
      return <path d="M6 6h12v12H6z" />;
    case "close":
      return <path d="M7 7l10 10M17 7L7 17" />;
    default:
      return null;
  }
}

export default function Icon({ name, size = 18, strokeWidth = 1.8, style }: IconProps) {
  return (
    <svg
      viewBox="0 0 24 24"
      width={size}
      height={size}
      fill="none"
      stroke="currentColor"
      strokeWidth={strokeWidth}
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden
      style={style}
    >
      {iconPath(name)}
    </svg>
  );
}
