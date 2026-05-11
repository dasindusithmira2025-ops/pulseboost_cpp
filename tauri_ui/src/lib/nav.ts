import type { ScreenId } from "../types/system";

export interface NavItem {
  id: ScreenId;
  icon: "home" | "optimizations" | "boost" | "games" | "backup" | "ai" | "settings";
  label: string;
  description: string;
}

export const primaryNavItems: NavItem[] = [
  { id: "home", icon: "home", label: "Home", description: "See what matters right now" },
  { id: "optimizations", icon: "optimizations", label: "Optimizations", description: "Apply safe gaming-first improvements" },
  { id: "boost_up", icon: "boost", label: "Boost-Up", description: "Run prep, cleanup, and repair actions" },
  { id: "games", icon: "games", label: "Games", description: "Detect, optimize, and launch your library" },
  { id: "backup", icon: "backup", label: "Backup", description: "Create snapshots and recover safely" },
  { id: "ai", icon: "ai", label: "AI", description: "Ask PulseBoost AI for guided help" }
];

export const secondaryNavItems: NavItem[] = [
  { id: "settings", icon: "settings", label: "Settings", description: "Product and account configuration" }
];

const navItemMap: Partial<Record<ScreenId, NavItem>> = {
  onboarding: {
    id: "onboarding",
    icon: "home",
    label: "Onboarding",
    description: "First-run setup"
  }
};

for (const item of [...primaryNavItems, ...secondaryNavItems]) {
  navItemMap[item.id] = item;
}

export function getScreenLabel(screen: ScreenId): string {
  return navItemMap[screen]?.label ?? "PulseBoost";
}
