import Card from "../components/Card";

const SECTIONS = [
  { id: "general", label: "General" },
  { id: "session", label: "Session" },
  { id: "performance", label: "Performance" },
  { id: "safety", label: "Safety" },
  { id: "expert", label: "Expert" },
  { id: "shortcuts", label: "Shortcuts" },
  { id: "data", label: "Data" },
];

function Toggle({ enabled, onChange }) {
  return (
    <button
      onClick={onChange}
      className={`flex h-5 w-9 shrink-0 items-center rounded-full px-0.5 transition-colors ${enabled ? "justify-end bg-accent" : "justify-start bg-surface-active"}`}
      type="button"
    >
      <div className="h-4 w-4 rounded-full bg-white shadow-sm" />
    </button>
  );
}

function SettingRow({ label, description, children }) {
  return (
    <div className="flex items-center justify-between border-b border-border-subtle py-3 last:border-0">
      <div className="mr-4 min-w-0 flex-1">
        <div className="text-sm text-txt-primary">{label}</div>
        {description ? <div className="mt-0.5 text-xs text-txt-tertiary">{description}</div> : null}
      </div>
      {children}
    </div>
  );
}

export default function SettingsPage({
  adaptiveState,
  expertModeEnabled,
  handleAdaptiveToggle,
  handleExpertModeToggle,
  handleSettingsDataAction,
  onPreferenceChange,
  preferences,
  settingsDataActionState,
}) {
  const settings = preferences || {};
  const activeSection = settings.activeSection || "general";

  const setSection = (sectionId) => onPreferenceChange?.("activeSection", sectionId);
  const togglePreference = (key, nextValue = !settings[key]) => onPreferenceChange?.(key, nextValue);
  const busyDataAction = settingsDataActionState?.busyAction || "";
  const dataActionInFlight = Boolean(busyDataAction);
  const dataActionMessage = settingsDataActionState?.message || "";
  const dataActionTone = settingsDataActionState?.tone || "info";
  const dataActionToneClass = dataActionTone === "error" ? "text-error" : dataActionTone === "success" ? "text-success" : "text-txt-secondary";

  return (
    <div className="space-y-5">
      <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
        Settings
      </h1>

      <div className="flex gap-5">
        <nav className="w-40 shrink-0 space-y-0.5">
          {SECTIONS.map((section) => (
            <button
              key={section.id}
              onClick={() => setSection(section.id)}
              className={`w-full rounded-md px-3 py-2 text-left text-sm transition-colors ${
                activeSection === section.id
                  ? "bg-nav-active text-txt-primary"
                  : "text-txt-secondary hover:bg-surface-hover hover:text-txt-primary"
              }`}
              type="button"
            >
              {section.label}
            </button>
          ))}
        </nav>

        <div className="min-w-0 flex-1">
          {activeSection === "general" ? (
            <Card className="p-5">
              <h3 className="mb-1 text-[15px] font-semibold text-txt-primary">General</h3>
              <p className="mb-4 text-xs text-txt-tertiary">
                Application startup and notification preferences saved to the local PulseBoost database.
              </p>
              <SettingRow label="Start with Windows" description="Launch PulseBoost when Windows starts">
                <Toggle enabled={Boolean(settings.startWithWindows)} onChange={() => togglePreference("startWithWindows")} />
              </SettingRow>
              <SettingRow label="Minimize to tray" description="Keep running in system tray when closed">
                <Toggle enabled={Boolean(settings.minimizeToTray)} onChange={() => togglePreference("minimizeToTray")} />
              </SettingRow>
              <SettingRow label="Check for updates" description="Automatically check for new versions">
                <Toggle enabled={Boolean(settings.checkUpdates)} onChange={() => togglePreference("checkUpdates")} />
              </SettingRow>
              <SettingRow label="Update channel">
                <select
                  className="rounded-md border border-border-default bg-surface-sunken px-3 py-1 text-sm text-txt-primary focus:border-accent focus:outline-none"
                  value={settings.updateChannel || "stable"}
                  onChange={(event) => togglePreference("updateChannel", event.target.value)}
                >
                  <option value="stable">Stable</option>
                  <option value="beta">Beta</option>
                </select>
              </SettingRow>
            </Card>
          ) : null}

          {activeSection === "session" ? (
            <Card className="p-5">
              <h3 className="mb-1 text-[15px] font-semibold text-txt-primary">Session</h3>
              <p className="mb-4 text-xs text-txt-tertiary">
                Session tracking and automation controls.
              </p>
              <SettingRow label="Session duration tracking" description="Track how long optimization sessions run">
                <Toggle enabled={Boolean(settings.sessionTracking)} onChange={() => togglePreference("sessionTracking")} />
              </SettingRow>
              <SettingRow label="Auto-optimize on session start" description="Apply recommended tweaks when a session begins">
                <Toggle
                  enabled={Boolean(settings.autoOptimize)}
                  onChange={() => {
                    const next = !settings.autoOptimize;
                    togglePreference("autoOptimize", next);
                    handleAdaptiveToggle?.(next);
                  }}
                />
              </SettingRow>
              <SettingRow label="Session summary on close" description="Show optimization summary when ending a session">
                <Toggle enabled={Boolean(settings.sessionSummary)} onChange={() => togglePreference("sessionSummary")} />
              </SettingRow>
            </Card>
          ) : null}

          {activeSection === "performance" ? (
            <Card className="p-5">
              <h3 className="mb-1 text-[15px] font-semibold text-txt-primary">Performance</h3>
              <p className="mb-4 text-xs text-txt-tertiary">
                Telemetry and monitoring configuration.
              </p>
              <SettingRow label="Telemetry polling interval">
                <select
                  className="rounded-md border border-border-default bg-surface-sunken px-3 py-1 text-sm text-txt-primary focus:border-accent focus:outline-none"
                  value={settings.telemetryPollingInterval || "1 second"}
                  onChange={(event) => togglePreference("telemetryPollingInterval", event.target.value)}
                >
                  <option value="1 second">1 second</option>
                  <option value="2 seconds">2 seconds</option>
                  <option value="5 seconds">5 seconds</option>
                </select>
              </SettingRow>
              <SettingRow label="Background monitoring" description="Continue monitoring when PulseBoost is minimized">
                <Toggle enabled={Boolean(settings.backgroundMonitoring)} onChange={() => togglePreference("backgroundMonitoring")} />
              </SettingRow>
            </Card>
          ) : null}

          {activeSection === "safety" ? (
            <Card className="p-5">
              <h3 className="mb-1 text-[15px] font-semibold text-txt-primary">Safety</h3>
              <p className="mb-4 text-xs text-txt-tertiary">
                Recovery and protection settings.
              </p>
              <SettingRow label="Auto-snapshot before batch operations">
                <Toggle enabled={Boolean(settings.autoSnapshot)} onChange={() => togglePreference("autoSnapshot")} />
              </SettingRow>
              <SettingRow label="Revert on failure" description="Automatically revert if an optimization fails">
                <Toggle enabled={Boolean(settings.revertOnFailure)} onChange={() => togglePreference("revertOnFailure")} />
              </SettingRow>
              <SettingRow label="Admin elevation prompt" description="Ask before requesting administrator access">
                <Toggle enabled={Boolean(settings.adminPrompt)} onChange={() => togglePreference("adminPrompt")} />
              </SettingRow>
              <SettingRow label="Dry-run by default" description="Simulate changes without applying them">
                <Toggle enabled={Boolean(settings.dryRunDefault)} onChange={() => togglePreference("dryRunDefault")} />
              </SettingRow>
            </Card>
          ) : null}

          {activeSection === "expert" ? (
            <Card className="p-5">
              <h3 className="mb-1 text-[15px] font-semibold text-txt-primary">Expert</h3>
              <p className="mb-4 text-xs text-txt-tertiary">
                Advanced features for power users.
              </p>
              <SettingRow label="Expert mode" description="Show advanced optimizations with higher risk">
                <Toggle
                  enabled={Boolean(expertModeEnabled)}
                  onChange={() => {
                    const next = !expertModeEnabled;
                    togglePreference("expertMode", next);
                    handleExpertModeToggle?.(next);
                  }}
                />
              </SettingRow>
              <SettingRow label="Show experimental tweaks" description="Include unvalidated optimizations">
                <Toggle enabled={Boolean(settings.experimentalTweaks)} onChange={() => togglePreference("experimentalTweaks")} />
              </SettingRow>
              <SettingRow label="Verbose logging" description="Detailed logs for troubleshooting">
                <Toggle enabled={Boolean(settings.verboseLogging)} onChange={() => togglePreference("verboseLogging")} />
              </SettingRow>
            </Card>
          ) : null}

          {activeSection === "data" ? (
            <Card className="p-5">
              <h3 className="mb-1 text-[15px] font-semibold text-txt-primary">Data</h3>
              <p className="mb-4 text-xs text-txt-tertiary">
                Manage local exports, imports, and settings-reset actions with auditable backend handling.
              </p>
              {dataActionMessage ? <p className={`mb-3 text-xs ${dataActionToneClass}`}>{dataActionMessage}</p> : null}
              <div className="space-y-3">
                {[
                  {
                    id: "export_all_data",
                    title: "Export all data",
                    copy: "Download all settings, profiles, and history as JSON.",
                    action: "Export",
                  },
                  {
                    id: "import_settings",
                    title: "Import settings",
                    copy: "Restore from a previous export.",
                    action: "Import",
                  },
                  {
                    id: "clear_benchmark_history",
                    title: "Clear benchmark history",
                    copy: "Remove all benchmark results.",
                    action: "Clear",
                  },
                  {
                    id: "reset_all_settings",
                    title: "Reset all settings",
                    copy: "Restore factory defaults.",
                    action: "Reset",
                  },
                ].map((row) => (
                  <div key={row.title} className="flex items-center justify-between border-t border-border-subtle py-2 first:border-0">
                    <div>
                      <div className="text-sm text-txt-primary">{row.title}</div>
                      <div className="text-xs text-txt-tertiary">{row.copy}</div>
                    </div>
                    <button
                      className={`rounded-md border border-border-default px-3 py-1.5 text-xs transition-colors ${dataActionInFlight ? "text-txt-disabled" : "text-txt-secondary hover:bg-surface-hover hover:text-txt-primary"}`}
                      disabled={dataActionInFlight || !handleSettingsDataAction}
                      onClick={() => handleSettingsDataAction?.(row.id)}
                      type="button"
                    >
                      {busyDataAction === row.id ? "Running..." : row.action}
                    </button>
                  </div>
                ))}
              </div>
            </Card>
          ) : null}

          {activeSection === "shortcuts" ? (
            <Card className="p-5">
              <h3 className="mb-1 text-[15px] font-semibold text-txt-primary">Keyboard Shortcuts</h3>
              <p className="mb-4 text-xs text-txt-tertiary">
                Global shortcuts work from anywhere in the desktop app.
              </p>
              <div className="space-y-2">
                {[
                  { combo: "Ctrl+Shift+B", action: "Trigger Boost Now" },
                  { combo: "Ctrl+Shift+A", action: "Focus PulseAI chat input" },
                  { combo: "Ctrl+Shift+H", action: "Jump to PulseCore page" },
                  { combo: "Escape", action: "Dismiss active dialog" },
                ].map((item) => (
                  <div key={item.combo} className="flex items-center justify-between border-b border-border-subtle py-2 last:border-0">
                    <span className="numeric-tabular rounded bg-surface-sunken px-2 py-1 text-xs text-txt-primary">{item.combo}</span>
                    <span className="text-xs text-txt-secondary">{item.action}</span>
                  </div>
                ))}
              </div>
            </Card>
          ) : null}
        </div>
      </div>
    </div>
  );
}
