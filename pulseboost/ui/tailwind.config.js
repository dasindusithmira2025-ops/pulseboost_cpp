export default {
  content: ["./index.html", "./src/**/*.{js,jsx}"],
  theme: {
    extend: {
      colors: {
        app: "#0A0A0B",
        surface: {
          DEFAULT: "#111113",
          elevated: "#141416",
          hover: "#17171A",
          active: "#1A1A1D",
          sunken: "#0D0D0F",
        },
        nav: {
          DEFAULT: "#0C0C0D",
          active: "#141418",
        },
        border: {
          default: "#1E1E20",
          subtle: "#17171A",
          strong: "#27272B",
        },
        txt: {
          primary: "#F7F6F3",
          secondary: "#C7C5BF",
          tertiary: "#8B8A86",
          disabled: "#62615E",
        },
        accent: {
          DEFAULT: "#2D7FF9",
          hover: "#448DFA",
          muted: "rgba(45,127,249,0.16)",
        },
        success: {
          DEFAULT: "#22C55E",
          muted: "rgba(34,197,94,0.12)",
        },
        warning: {
          DEFAULT: "#F59E0B",
          muted: "rgba(245,158,11,0.12)",
        },
        error: {
          DEFAULT: "#EF4444",
          muted: "rgba(239,68,68,0.12)",
        },
        info: {
          DEFAULT: "#2D7FF9",
          muted: "rgba(45,127,249,0.12)",
        },
        premium: {
          DEFAULT: "#2D7FF9",
          muted: "rgba(45,127,249,0.12)",
        },
      },
      fontFamily: {
        sans: ["Geist", "-apple-system", "BlinkMacSystemFont", "Segoe UI", "sans-serif"],
        mono: ["Geist Mono", "ui-monospace", "SFMono-Regular", "monospace"],
      },
    },
  },
  plugins: [],
};
