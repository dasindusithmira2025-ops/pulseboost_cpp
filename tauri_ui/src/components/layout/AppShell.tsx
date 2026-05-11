import type { ReactNode } from "react";
import ContentArea from "./ContentArea";
import Sidebar from "./Sidebar";
import StatusBar from "./StatusBar";
import TopBar from "./TopBar";
import ActionProgressOverlay from "../ui/ActionProgressOverlay";
import ToastNotification from "../ui/ToastNotification";
import type { ScreenId } from "../../types/system";

interface AppShellProps {
  screens: Record<ScreenId, ReactNode>;
}

export default function AppShell({ screens }: AppShellProps) {
  return (
    <div className="app-root">
      <div className="app-bg" />

      <div className="shell">
        <Sidebar />
        <TopBar />
        <div className="shell-main">
          <ContentArea screens={screens} />
        </div>
        <StatusBar />
      </div>

      <ActionProgressOverlay />
      <ToastNotification />
    </div>
  );
}
