import { lazy, Suspense, useEffect } from "react";
import type { ReactNode } from "react";
import AppShell from "./components/layout/AppShell";
import Home from "./screens/Dashboard";
import { useSnapshot } from "./hooks/useSnapshot";
import { useSystemStore } from "./stores/systemStore";
import { useUiStore } from "./stores/uiStore";
import { isTauriRuntime, tauriInvoke } from "./lib/tauri";
import type { LicenseInfo, ScreenId } from "./types/system";

const AiAssistant = lazy(() => import("./screens/AiChat"));
const Optimizations = lazy(() => import("./screens/TweakEngine"));
const BoostUp = lazy(() => import("./screens/BoostUp"));
const Games = lazy(() => import("./screens/GameOptimizer"));
const Backup = lazy(() => import("./screens/HealthHistory"));
const Settings = lazy(() => import("./screens/Settings"));
const Onboarding = lazy(() => import("./screens/Onboarding"));

function deferredScreen(node: ReactNode) {
  return (
    <Suspense
      fallback={(
        <div className="screen-enter">
          <div className="glass-card" style={{ padding: "var(--sp-6)" }}>
            <div className="font-display" style={{ fontSize: 20 }}>Loading screen...</div>
            <div className="tiny">Preparing the next workspace</div>
          </div>
        </div>
      )}
    >
      {node}
    </Suspense>
  );
}

export default function App() {
  useSnapshot();
  const fetchSnapshot = useSystemStore((s) => s.fetchSnapshot);
  const isLoading = useSystemStore((s) => s.isLoading);
  const error = useSystemStore((s) => s.error);
  const hasSnapshot = useSystemStore((s) => s.snapshot !== null);
  const onboardingDone = useUiStore((s) => s.onboardingDone);
  const setPro = useUiStore((s) => s.setPro);

  useEffect(() => {
    const handler = () => {
      void fetchSnapshot();
    };
    window.addEventListener("pulseboost-refresh", handler);
    return () => {
      window.removeEventListener("pulseboost-refresh", handler);
    };
  }, [fetchSnapshot]);

  useEffect(() => {
    if (!isTauriRuntime()) {
      return;
    }
    void tauriInvoke<LicenseInfo>("get_license_info")
      .then((license) => setPro(Boolean(license.isPro)))
      .catch(() => {
        setPro(false);
      });
  }, [setPro]);

  const screens: Record<ScreenId, ReactNode> = {
    home: <Home />,
    optimizations: deferredScreen(<Optimizations />),
    boost_up: deferredScreen(<BoostUp />),
    games: deferredScreen(<Games />),
    ai: deferredScreen(<AiAssistant />),
    backup: deferredScreen(<Backup />),
    settings: deferredScreen(<Settings />),
    onboarding: deferredScreen(<Onboarding />)
  };

  if (isLoading && onboardingDone) {
    return (
      <div className="app-root">
        <div className="app-bg" />
        <div style={{ position: "relative", zIndex: 1, height: "100%", display: "grid", placeItems: "center" }}>
          <div className="glass-card" style={{ padding: "var(--sp-6)" }}>
            <div className="font-display" style={{ fontSize: 20 }}>Loading PulseBoost UI...</div>
            <div className="tiny">Connecting to telemetry stream</div>
          </div>
        </div>
      </div>
    );
  }

  if (error && onboardingDone && !hasSnapshot) {
    return (
      <div className="app-root">
        <div className="app-bg" />
        <div style={{ position: "relative", zIndex: 1, height: "100%", display: "grid", placeItems: "center" }}>
          <div className="glass-card" style={{ padding: "var(--sp-6)" }}>
            <div className="font-display" style={{ fontSize: 20 }}>UI Error State</div>
            <div className="tiny">{error}</div>
          </div>
        </div>
      </div>
    );
  }

  return <AppShell screens={screens} />;
}
