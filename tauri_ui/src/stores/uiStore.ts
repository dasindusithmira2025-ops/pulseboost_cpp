import { create } from "zustand";
import type { ScreenId, ToolsTab } from "../types/system";

interface UiStore {
  activeScreen: ScreenId;
  activeToolsTab: ToolsTab;
  sidebarCollapsed: boolean;
  searchQuery: string;
  isPro: boolean;
  onboardingDone: boolean;
  setActiveScreen: (screen: ScreenId) => void;
  setActiveToolsTab: (tab: ToolsTab) => void;
  navigateToTool: (tab: ToolsTab) => void;
  setSidebarCollapsed: (value: boolean) => void;
  setSearchQuery: (value: string) => void;
  setPro: (value: boolean) => void;
  setOnboardingDone: (value: boolean) => void;
}

const storedOnboarding = localStorage.getItem("pulseboost_onboarding") === "1";

export const useUiStore = create<UiStore>((set) => ({
  activeScreen: storedOnboarding ? "home" : "onboarding",
  activeToolsTab: "processes",
  sidebarCollapsed: false,
  searchQuery: "",
  isPro: false,
  onboardingDone: storedOnboarding,
  setActiveScreen: (activeScreen) => set({ activeScreen }),
  setActiveToolsTab: (activeToolsTab) => set({ activeToolsTab }),
  navigateToTool: (activeToolsTab) => set({ activeScreen: "settings", activeToolsTab }),
  setSidebarCollapsed: (sidebarCollapsed) => set({ sidebarCollapsed }),
  setSearchQuery: (searchQuery) => set({ searchQuery }),
  setPro: (isPro) => set({ isPro }),
  setOnboardingDone: (onboardingDone) => {
    localStorage.setItem("pulseboost_onboarding", onboardingDone ? "1" : "0");
    set({ onboardingDone });
  }
}));
