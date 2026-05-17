import {
  BarChart3Icon,
  BoltIcon,
  ChevronLeftIcon,
  ChevronRightIcon,
  CpuIcon,
  FileTextIcon,
  Gamepad2Icon,
  GlobeIcon,
  LayoutDashboardIcon,
  SettingsIcon,
  ShieldIcon,
  UserIcon,
} from "lucide-react";

const NAV_ITEMS = [
  { id: "dashboard", label: "Dashboard", icon: LayoutDashboardIcon },
  { id: "pulsecore", label: "PulseCore", icon: BoltIcon },
  { id: "network", label: "Network", icon: GlobeIcon },
  { id: "gpu", label: "GPU", icon: CpuIcon },
  { id: "audit", label: "Audit Log", icon: FileTextIcon },
  { id: "benchmark", label: "Benchmark", icon: BarChart3Icon },
  { id: "profiles", label: "Game Profiles", icon: Gamepad2Icon },
  { id: "trust", label: "Trust Center", icon: ShieldIcon },
];

export default function Sidebar({
  activePage,
  onNavigate,
  collapsed,
  onToggleCollapse,
  accountIdentity,
  healthScore,
  planLabel,
  pulse,
  signedIn,
}) {
  const safeScore = Math.max(0, Math.min(100, Number(healthScore || 0)));
  const ringDegrees = Math.round((safeScore / 100) * 360);
  const ringColor = safeScore > 75 ? "#22C55E" : safeScore >= 50 ? "#F59E0B" : "#EF4444";

  return (
    <aside
      className={[
        "flex h-full flex-col border-r border-border-subtle bg-nav transition-all duration-200",
        pulse ? "border-2 border-[#2D7FF9]" : "",
        collapsed ? "w-14" : "w-[220px]",
      ].join(" ")}
    >
      <div className="px-3 pb-2 pt-4">
        <div
          className={[
            "rounded-[12px] border border-border-subtle bg-surface-sunken px-3 py-2",
            collapsed ? "text-center" : "",
          ].join(" ")}
        >
          <div className="flex items-center gap-2">
            <div
              className="relative flex h-5 w-5 items-center justify-center rounded-full"
              style={{ background: `conic-gradient(#2D7FF9 ${ringDegrees}deg, #1E1E20 ${ringDegrees}deg)` }}
            >
              <div className="h-3.5 w-3.5 rounded-full bg-surface-sunken" />
            </div>
            <div className="text-[10px] font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              PulseBoost
            </div>
          </div>
          {!collapsed ? (
            <div className="mt-1 text-xs text-txt-secondary">
              Realtime workstation shell
            </div>
          ) : null}
        </div>
      </div>

      <nav className="flex-1 space-y-0.5 overflow-y-auto px-2 py-2">
        {NAV_ITEMS.map((item) => {
          const Icon = item.icon;
          const isActive = activePage === item.id;
          return (
            <button
              key={item.id}
              onClick={() => onNavigate(item.id)}
              className={[
                "group relative flex w-full items-center gap-2.5 rounded-md px-2.5 py-2 text-[13px] transition-colors",
                isActive
                  ? "bg-[#111113] text-txt-primary"
                  : "text-txt-secondary hover:bg-surface-hover hover:text-txt-primary",
                collapsed ? "justify-center" : "",
              ].join(" ")}
              title={collapsed ? item.label : undefined}
              type="button"
            >
              {isActive ? (
                <span className="absolute left-0 top-1/2 h-5 w-[2px] -translate-y-1/2 rounded-r bg-[#2D7FF9]" />
              ) : null}
              <Icon className={`h-4 w-4 shrink-0 ${isActive ? "text-accent" : ""}`} />
              {!collapsed ? <span>{item.label}</span> : null}
            </button>
          );
        })}
      </nav>

      <div className="space-y-0.5 border-t border-border-subtle px-2 pb-3 pt-2">
        <button
          onClick={() => onNavigate("pulsecore")}
          className={[
            "mb-1 flex w-full items-center gap-2 rounded-md border border-border-default bg-surface-sunken px-2 py-2 transition-colors hover:bg-surface-hover",
            collapsed ? "justify-center" : "",
          ].join(" ")}
          title="Your PC Health Score - click to see history"
          type="button"
        >
          <div
            className="relative flex h-8 w-8 items-center justify-center rounded-full"
            style={{ background: `conic-gradient(${ringColor} ${ringDegrees}deg, #1E1E20 ${ringDegrees}deg)` }}
          >
            <div className="flex h-6 w-6 items-center justify-center rounded-full bg-surface-sunken text-[10px] text-txt-primary numeric-tabular">
              {Math.round(safeScore)}
            </div>
          </div>
          {!collapsed ? (
            <div className="text-left">
              <div className="text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">Health Score</div>
              <div className="numeric-tabular text-xs text-txt-primary">{Math.round(safeScore)}/100</div>
            </div>
          ) : null}
        </button>

        <button
          onClick={() => onNavigate("settings")}
          className={[
            "flex w-full items-center gap-2.5 rounded-md px-2.5 py-2 text-[13px] transition-colors",
            activePage === "settings"
              ? "bg-nav-active text-txt-primary"
              : "text-txt-secondary hover:bg-surface-hover hover:text-txt-primary",
            collapsed ? "justify-center" : "",
          ].join(" ")}
          title={collapsed ? "Settings" : undefined}
          type="button"
        >
          <SettingsIcon className={`h-4 w-4 shrink-0 ${activePage === "settings" ? "text-accent" : ""}`} />
          {!collapsed ? <span>Settings</span> : null}
        </button>

        <button
          onClick={() => onNavigate("account")}
          className={[
            "flex w-full items-center gap-2.5 rounded-md px-2.5 py-2 transition-colors hover:bg-surface-hover",
            collapsed ? "justify-center" : "",
          ].join(" ")}
          type="button"
        >
          <div className="flex h-7 w-7 shrink-0 items-center justify-center rounded-full bg-accent/20">
            <UserIcon className="h-3.5 w-3.5 text-accent" />
          </div>
          {!collapsed ? (
            <div className="min-w-0 flex-1 text-left">
              <div className="truncate text-xs text-txt-primary">
                {signedIn
                  ? accountIdentity?.display_name || accountIdentity?.email || "Account"
                  : "Sign in"}
              </div>
              <div className="text-[10px] font-semibold uppercase tracking-[0.18em] text-accent">
                {planLabel}
              </div>
            </div>
          ) : null}
        </button>

        <button
          onClick={onToggleCollapse}
          className="flex w-full items-center justify-center rounded-md py-1.5 text-txt-tertiary transition-colors hover:bg-surface-hover hover:text-txt-secondary"
          type="button"
        >
          {collapsed ? <ChevronRightIcon className="h-4 w-4" /> : <ChevronLeftIcon className="h-4 w-4" />}
        </button>
      </div>
    </aside>
  );
}
