import { useEffect, useMemo, useState } from "react";
import type { ReactNode } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import Tools from "./Tools";
import { useChatStore } from "../stores/chatStore";
import { useSystemStore } from "../stores/systemStore";
import { useUiStore } from "../stores/uiStore";
import type { AiPreferences, ErrorLogPayload, ExportPathPayload, LicenseInfo, ScheduledTask, UpdateStatus } from "../types/system";

const categories = [
  "General",
  "PulseModel",
  "License",
  "Scheduled Tasks",
  "Updates",
  "Data & Privacy",
  "Advanced",
  "About"
] as const;

type SettingsTab = (typeof categories)[number];

type LocalSettings = {
  launchAtStartup: boolean;
  minimizeToTray: boolean;
  anonymousAnalytics: boolean;
};

const settingsStorageKey = "pulseboost_ui_settings";

const taskCatalog: Array<{ id: string; type: string; title: string; subtitle: string }> = [
  { id: "daily_junk_clean", type: "clean_junk", title: "Daily Junk Clean", subtitle: "Run cleanup on a fixed interval to keep temporary storage under control." },
  { id: "weekly_startup_analysis", type: "analyze_startup", title: "Weekly Startup Analysis", subtitle: "Check for new startup overhead and keep boot impact visible." },
  { id: "daily_ram_optimization", type: "optimize_ram", title: "Daily RAM Optimization", subtitle: "Periodically free pressure on high-memory systems." }
];

function loadStoredSettings(): LocalSettings {
  try {
    const raw = localStorage.getItem(settingsStorageKey);
    if (!raw) {
      return {
        launchAtStartup: true,
        minimizeToTray: true,
        anonymousAnalytics: true
      };
    }
    const parsed = JSON.parse(raw) as Partial<LocalSettings>;
    return {
      launchAtStartup: parsed.launchAtStartup ?? true,
      minimizeToTray: parsed.minimizeToTray ?? true,
      anonymousAnalytics: parsed.anonymousAnalytics ?? true
    };
  } catch {
    return {
      launchAtStartup: true,
      minimizeToTray: true,
      anonymousAnalytics: true
    };
  }
}

function settingRow(title: string, subtitle: string, action: ReactNode) {
  return (
    <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "flex", justifyContent: "space-between", gap: "var(--sp-4)", alignItems: "center" }}>
      <div>
        <div className="font-display" style={{ fontSize: 18, fontWeight: 700, marginBottom: 6 }}>{title}</div>
        <div className="tiny" style={{ color: "var(--text-secondary)" }}>{subtitle}</div>
      </div>
      {action}
    </div>
  );
}

export default function Settings() {
  const [tab, setTab] = useState<SettingsTab>("General");
  const isPro = useUiStore((s) => s.isPro);
  const setPro = useUiStore((s) => s.setPro);
  const messages = useChatStore((s) => s.messages);
  const resetChat = useChatStore((s) => s.clear);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const [settings, setSettings] = useState<LocalSettings>(() => loadStoredSettings());
  const [scheduledTasks, setScheduledTasks] = useState<ScheduledTask[]>([]);
  const [loadingTasks, setLoadingTasks] = useState(false);
  const [savingTaskId, setSavingTaskId] = useState<string | null>(null);
  const [licenseInfo, setLicenseInfo] = useState<LicenseInfo | null>(null);
  const [loadingLicense, setLoadingLicense] = useState(false);
  const [licenseKeyInput, setLicenseKeyInput] = useState("");
  const [activatingLicense, setActivatingLicense] = useState(false);
  const [updateStatus, setUpdateStatus] = useState<UpdateStatus | null>(null);
  const [aiPreferences, setAiPreferences] = useState<AiPreferences | null>(null);
  const [loadingUpdateStatus, setLoadingUpdateStatus] = useState(false);
  const [aiApiKeyInput, setAiApiKeyInput] = useState("");
  const [loadingAiPreferences, setLoadingAiPreferences] = useState(false);
  const [savingAiPreferences, setSavingAiPreferences] = useState(false);
  const [errorEntries, setErrorEntries] = useState<Array<{ source: string; message: string }>>([]);
  const [loadingErrorLog, setLoadingErrorLog] = useState(false);
  const [exportingErrorLog, setExportingErrorLog] = useState(false);

  useEffect(() => {
    localStorage.setItem(settingsStorageKey, JSON.stringify(settings));
  }, [settings]);

  const loadScheduledTasks = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setLoadingTasks(true);
    try {
      const result = await tauriInvoke<ScheduledTask[] | { tasks?: ScheduledTask[] }>("get_scheduled_tasks");
      const tasks = Array.isArray(result) ? result : result.tasks ?? [];
      setScheduledTasks(tasks);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load scheduled tasks", "danger");
    } finally {
      setLoadingTasks(false);
    }
  };

  const loadLicenseInfo = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setLoadingLicense(true);
    try {
      const result = await tauriInvoke<LicenseInfo>("get_license_info");
      setLicenseInfo(result);
      setPro(Boolean(result.isPro));
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load license status", "danger");
    } finally {
      setLoadingLicense(false);
    }
  };

  const loadAiPreferences = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setLoadingAiPreferences(true);
    try {
      const result = await tauriInvoke<AiPreferences>("get_ai_preferences");
      setAiPreferences(result);
      setAiApiKeyInput(result.apiKeyMasked ?? "");
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load AI preferences", "danger");
    } finally {
      setLoadingAiPreferences(false);
    }
  };

  const saveAiPreferences = async (mode: "local" | "cloud", apiKey?: string) => {
    if (!isTauriRuntime()) {
      return;
    }
    setSavingAiPreferences(true);
    try {
      const result = await tauriInvoke<AiPreferences>("set_ai_preferences", {
        mode,
        apiKey: apiKey ?? aiApiKeyInput
      });
      setAiPreferences(result);
      pushNotification(mode === "cloud" ? "Cloud preference saved. Local fallback remains active when unavailable." : "Local AI mode enabled", "ok");
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to save AI preferences", "danger");
    } finally {
      setSavingAiPreferences(false);
    }
  };

  const loadUpdateStatus = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setLoadingUpdateStatus(true);
    try {
      const result = await tauriInvoke<UpdateStatus>("check_for_updates");
      setUpdateStatus(result);
      pushNotification(result.updateAvailable ? `Update available: ${result.latestVersion ?? "new version"}` : result.manifestTrusted ? "PulseBoost AI is up to date" : "Update manifest could not be trusted", result.updateAvailable ? "warn" : result.manifestTrusted ? "ok" : "danger");
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to check for updates", "danger");
    } finally {
      setLoadingUpdateStatus(false);
    }
  };

  const loadErrorLog = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setLoadingErrorLog(true);
    try {
      const result = await tauriInvoke<ErrorLogPayload>("get_error_log");
      setErrorEntries(result.entries ?? []);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load local error log", "danger");
    } finally {
      setLoadingErrorLog(false);
    }
  };

  const exportErrorLog = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setExportingErrorLog(true);
    try {
      const result = await tauriInvoke<ExportPathPayload>("export_error_log");
      if (result.path) {
        pushNotification(`Exported error log to ${result.path}`, "ok");
      } else {
        pushNotification(result.reason ?? "Unable to export error log", "danger");
      }
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to export error log", "danger");
    } finally {
      setExportingErrorLog(false);
    }
  };

  useEffect(() => {
    if (tab === "Scheduled Tasks") {
      void loadScheduledTasks();
    }
    if (tab === "License" && licenseInfo === null) {
      void loadLicenseInfo();
    }
    if (tab === "PulseModel" && aiPreferences === null) {
      void loadAiPreferences();
    }
    if (tab === "Updates" && updateStatus === null) {
      void loadUpdateStatus();
    }
    if (tab === "Data & Privacy" && errorEntries.length === 0) {
      void loadErrorLog();
    }
  }, [tab]);

  const taskById = useMemo(() => {
    const map = new Map<string, ScheduledTask>();
    scheduledTasks.forEach((task) => map.set(task.id, task));
    return map;
  }, [scheduledTasks]);

  const toggleLocal = (key: keyof LocalSettings) => {
    setSettings((current) => ({ ...current, [key]: !current[key] }));
  };

  const updateTask = async (taskId: string, enabled: boolean, taskType: string, intervalHours: number) => {
    setSavingTaskId(taskId);
    try {
      await tauriInvoke("set_scheduled_task", {
        taskId,
        enabled,
        taskType,
        intervalHours
      });
      pushNotification(`${enabled ? "Enabled" : "Updated"} scheduled task: ${taskId.replace(/_/g, " ")}`, "ok");
      await loadScheduledTasks();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to update scheduled task", "danger");
    } finally {
      setSavingTaskId(null);
    }
  };

  const activateLicense = async () => {
    if (!licenseKeyInput.trim()) {
      pushNotification("Enter a license key first", "warn");
      return;
    }
    setActivatingLicense(true);
    try {
      const result = await tauriInvoke<LicenseInfo>("activate_license", { key: licenseKeyInput.trim() });
      setLicenseInfo(result);
      setPro(Boolean(result.isPro));
      if (result.isPro) {
        pushNotification("License activated successfully", "ok");
        setLicenseKeyInput("");
      } else {
        pushNotification(result.reason === "invalid-license-key" ? "Invalid license key" : "License activation failed", "danger");
      }
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "License activation failed", "danger");
    } finally {
      setActivatingLicense(false);
    }
  };

  const trialPill = !licenseInfo
    ? <StatusPill status="neutral" label="License state unavailable" />
    : licenseInfo.isPro
      ? <StatusPill status="ok" label={`Pro active${licenseInfo.tierLabel ? ` - ${licenseInfo.tierLabel}` : ""}`} />
      : licenseInfo.isTrial
        ? <StatusPill status="warn" label={`Trial - ${licenseInfo.trialDaysLeft} days remaining`} />
        : <StatusPill status="danger" label="Free tier active" />;

  return (
    <div className="screen-enter" style={{ display: "grid", gridTemplateColumns: "240px minmax(0, 1fr)", gap: "var(--sp-4)", height: "100%" }}>
      <GlassCard>
        <SectionHeader title="Settings" subtitle="Product controls" />
        <div style={{ display: "grid", gap: "var(--sp-2)" }}>
          {categories.map((category, index) => (
            <button
              key={category}
              type="button"
              className="focus-ring interactive screen-enter"
              onClick={() => setTab(category)}
              style={{
                minHeight: 40,
                borderRadius: "var(--r-lg)",
                border: `1px solid ${tab === category ? "var(--border-accent)" : "var(--border-glass)"}`,
                background: tab === category
                  ? "linear-gradient(135deg, rgba(0,220,255,0.16), rgba(122,92,255,0.1))"
                  : "rgba(255,255,255,0.025)",
                color: tab === category ? "var(--text-primary)" : "var(--text-secondary)",
                cursor: "pointer",
                textAlign: "left",
                padding: "0 var(--sp-3)",
                animationDelay: `${index * 35}ms`
              }}
            >
              {category}
            </button>
          ))}
        </div>
      </GlassCard>

      <GlassCard className="h-full" style={{ minHeight: 0, overflow: "auto" }}>
        <SectionHeader title={tab} subtitle="Application-level controls, status, and product configuration" />

        {tab === "General" ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            {settingRow(
              "Launch at startup",
              "Open PulseBoost AI automatically when Windows starts.",
              <GlowButton label={settings.launchAtStartup ? "On" : "Off"} variant={settings.launchAtStartup ? "primary" : "ghost"} onClick={() => toggleLocal("launchAtStartup")} />
            )}
            {settingRow(
              "Minimize to tray",
              "Keep the app available in the tray instead of fully closing it.",
              <GlowButton label={settings.minimizeToTray ? "On" : "Off"} variant={settings.minimizeToTray ? "primary" : "ghost"} onClick={() => toggleLocal("minimizeToTray")} />
            )}
          </div>
        ) : null}

        {tab === "PulseModel" ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div className="badge-row">
              <StatusPill status="ok" label="PulseModel active" dot />
              <StatusPill status="info" label={`Messages ${messages.length}`} />
              <StatusPill status={aiPreferences?.mode === "cloud" ? "warn" : "ok"} label={aiPreferences?.mode === "cloud" ? "Cloud preferred" : "Local preferred"} />
              <GlowButton label="Refresh" variant="ghost" onClick={() => void loadAiPreferences()} loading={loadingAiPreferences} />
            </div>
            <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)" }}>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Session interactions</div>
              <div className="font-display" style={{ fontSize: 34, fontWeight: 800 }}>{messages.length}</div>
            </div>
            <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-3)" }}>
              <div>
                <div className="title-kicker" style={{ marginBottom: 8 }}>AI Mode</div>
                <div className="font-display" style={{ fontSize: 22, fontWeight: 800, marginBottom: 6 }}>
                  {aiPreferences?.mode === "cloud" ? "Cloud preferred" : "Local preferred"}
                </div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>
                  Cloud mode stores the preference and API key locally, but the app still falls back to local PulseModel when cloud routing is unavailable.
                </div>
              </div>
              <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                <GlowButton label="Use Local" variant={aiPreferences?.mode === "local" ? "primary" : "ghost"} onClick={() => void saveAiPreferences("local", aiApiKeyInput)} loading={savingAiPreferences} />
                <GlowButton label="Prefer Cloud" variant={aiPreferences?.mode === "cloud" ? "primary" : "ghost"} onClick={() => void saveAiPreferences("cloud", aiApiKeyInput)} loading={savingAiPreferences} />
              </div>
              <input
                value={aiApiKeyInput}
                onChange={(event) => {
                  setAiApiKeyInput(event.target.value);
                  setAiPreferences((current) => ({ ...(current ?? { ok: true, mode: "local", cloudConfigured: false }), cloudConfigured: event.target.value.trim().length > 0 }));
                }}
                placeholder="Heladev Cloud API key"
                className="focus-ring"
                style={{
                  minHeight: 42,
                  borderRadius: "var(--r-lg)",
                  border: "1px solid var(--border-glass-md)",
                  padding: "0 var(--sp-3)",
                  background: "rgba(255,255,255,0.03)",
                  color: "var(--text-primary)"
                }}
              />
              <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                <GlowButton label="Save AI Settings" onClick={() => void saveAiPreferences(aiPreferences?.mode === "cloud" ? "cloud" : "local", aiApiKeyInput)} loading={savingAiPreferences} />
                <StatusPill status={aiPreferences?.cloudConfigured ? "ok" : "neutral"} label={aiPreferences?.cloudConfigured ? "Cloud key configured" : "Cloud key missing"} />
              </div>
            </div>
            <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
              <GlowButton label="Reset Session Learning" variant="danger" onClick={() => resetChat()} />
              <GlowButton label="Export History" variant="ghost" />
            </div>
          </div>
        ) : null}

        {tab === "License" ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div className="badge-row">
              {trialPill}
              <GlowButton label="Refresh" variant="ghost" onClick={() => void loadLicenseInfo()} loading={loadingLicense} />
            </div>

            <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-3)" }}>
              <div>
                <div className="title-kicker" style={{ marginBottom: 8 }}>License status</div>
                <div className="font-display" style={{ fontSize: 24, fontWeight: 800, marginBottom: 6 }}>{licenseInfo?.tierLabel ?? (isPro ? "Pro" : "Free")}</div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>
                  {licenseInfo?.isPro
                    ? "Pro features are unlocked on this device."
                    : licenseInfo?.isTrial
                      ? `Trial access is active for ${licenseInfo.trialDaysLeft} more days.`
                      : "Free tier is active. Upgrade to unlock the full optimization surface."}
                </div>
              </div>
              <div className="surface-subtle" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)" }}>
                <div className="title-kicker" style={{ marginBottom: 6 }}>Stored key</div>
                <div className="font-mono" style={{ color: "var(--text-primary)" }}>{licenseInfo?.licenseKey || "No activated license key stored"}</div>
              </div>
            </div>

            {!licenseInfo?.isPro ? (
              <GlassCard glow="accent">
                <div className="title-kicker" style={{ marginBottom: 8 }}>Upgrade path</div>
                <div className="font-display" style={{ marginBottom: "var(--sp-2)", fontSize: 24, fontWeight: 800 }}>Activate PulseBoost Pro</div>
                <div className="tiny" style={{ color: "var(--text-secondary)", marginBottom: "var(--sp-3)" }}>
                  Unlock the full tweak catalog, unlimited benchmark history, scheduled optimization, and premium AI features.
                </div>
                <div style={{ display: "grid", gap: "var(--sp-2)" }}>
                  <input
                    value={licenseKeyInput}
                    onChange={(event) => setLicenseKeyInput(event.target.value)}
                    placeholder="PB-PRO-XXXX-XXXX-XXXX"
                    className="focus-ring"
                    style={{
                      minHeight: 42,
                      borderRadius: "var(--r-lg)",
                      border: "1px solid var(--border-glass-md)",
                      padding: "0 var(--sp-3)",
                      background: "rgba(255,255,255,0.03)",
                      color: "var(--text-primary)"
                    }}
                  />
                  <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                    <GlowButton label="Activate License" onClick={() => void activateLicense()} loading={activatingLicense} />
                    <GlowButton label="Refresh Status" variant="ghost" onClick={() => void loadLicenseInfo()} loading={loadingLicense} />
                  </div>
                </div>
              </GlassCard>
            ) : null}
          </div>
        ) : null}

        {tab === "Scheduled Tasks" ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div className="badge-row">
              <StatusPill status="info" label={`${scheduledTasks.filter((task) => task.enabled).length} enabled`} />
              <GlowButton label="Refresh" variant="ghost" onClick={() => void loadScheduledTasks()} loading={loadingTasks} />
            </div>

            {taskCatalog.map((task) => {
              const current = taskById.get(task.id);
              const enabled = current?.enabled ?? false;
              const intervalHours = current?.intervalHours ?? 24;
              return (
                <div key={task.id} className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-3)" }}>
                  <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", alignItems: "center" }}>
                    <div>
                      <div className="font-display" style={{ fontSize: 18, fontWeight: 700, marginBottom: 6 }}>{task.title}</div>
                      <div className="tiny" style={{ color: "var(--text-secondary)" }}>{task.subtitle}</div>
                    </div>
                    <GlowButton
                      label={enabled ? "Enabled" : "Disabled"}
                      variant={enabled ? "primary" : "ghost"}
                      onClick={() => void updateTask(task.id, !enabled, task.type, intervalHours)}
                      loading={savingTaskId === task.id}
                    />
                  </div>

                  <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                    {[12, 24, 48, 168].map((hours) => (
                      <button
                        key={hours}
                        type="button"
                        className="focus-ring interactive"
                        onClick={() => void updateTask(task.id, enabled, task.type, hours)}
                        style={{
                          minHeight: 30,
                          borderRadius: 999,
                          padding: "0 var(--sp-3)",
                          border: `1px solid ${intervalHours === hours ? "var(--border-accent)" : "var(--border-glass)"}`,
                          background: intervalHours === hours
                            ? "linear-gradient(135deg, rgba(0,220,255,0.16), rgba(122,92,255,0.1))"
                            : "rgba(255,255,255,0.03)",
                          color: intervalHours === hours ? "var(--text-primary)" : "var(--text-secondary)"
                        }}
                      >
                        {hours >= 168 ? "Weekly" : `${hours}h`}
                      </button>
                    ))}
                  </div>
                </div>
              );
            })}
          </div>
        ) : null}

        {tab === "Updates" ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div className="badge-row">
              {updateStatus ? (
                <>
                  <StatusPill status={updateStatus.updateAvailable ? "warn" : updateStatus.manifestTrusted ? "ok" : "danger"} label={updateStatus.updateAvailable ? `Update ${updateStatus.latestVersion ?? "available"}` : updateStatus.manifestTrusted ? "Up to date" : "Manifest untrusted"} />
                  <StatusPill status={updateStatus.manifestTrusted ? "ok" : "danger"} label={updateStatus.manifestTrusted ? "SHA256 verified policy" : "Missing SHA256 trust"} />
                </>
              ) : (
                <StatusPill status="neutral" label="Update status unavailable" />
              )}
              <GlowButton label="Check for updates" variant="ghost" onClick={() => void loadUpdateStatus()} loading={loadingUpdateStatus} />
            </div>
            <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-3)" }}>
              <div>
                <div className="title-kicker" style={{ marginBottom: 8 }}>Current version</div>
                <div className="font-mono">{updateStatus?.currentVersion ?? "1.0.0"}</div>
              </div>
              <div>
                <div className="title-kicker" style={{ marginBottom: 8 }}>Latest available</div>
                <div className="font-mono">{updateStatus?.latestVersion ?? "No update metadata loaded yet"}</div>
              </div>
              <div>
                <div className="title-kicker" style={{ marginBottom: 8 }}>Download URL</div>
                <div className="tiny" style={{ color: "var(--text-secondary)", wordBreak: "break-all" }}>
                  {updateStatus?.downloadUrl ?? "No download URL available"}
                </div>
              </div>
              <div>
                <div className="title-kicker" style={{ marginBottom: 8 }}>Expected SHA-256</div>
                <div className="font-mono tiny" style={{ color: updateStatus?.manifestTrusted ? "var(--text-primary)" : "var(--status-danger)", wordBreak: "break-all" }}>
                  {updateStatus?.sha256 ?? "No checksum in manifest"}
                </div>
              </div>
              {updateStatus?.lastError ? (
                <div>
                  <div className="title-kicker" style={{ marginBottom: 8 }}>Last updater error</div>
                  <div className="tiny" style={{ color: "var(--status-danger)" }}>{updateStatus.lastError}</div>
                </div>
              ) : null}
              {updateStatus?.downloadUrl ? (
                <GlowButton label="Open download link" variant="ghost" onClick={() => window.open(updateStatus.downloadUrl, "_blank", "noopener,noreferrer")} />
              ) : null}
            </div>
          </div>
        ) : null}

        {tab === "Data & Privacy" ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            {settingRow(
              "Anonymous analytics",
              "Local preference for whether non-identifying analytics are allowed.",
              <GlowButton label={settings.anonymousAnalytics ? "On" : "Off"} variant={settings.anonymousAnalytics ? "primary" : "ghost"} onClick={() => toggleLocal("anonymousAnalytics")} />
            )}
            <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-3)" }}>
              <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", gap: "var(--sp-3)" }}>
                <div>
                  <div className="font-display" style={{ fontSize: 18, fontWeight: 700, marginBottom: 6 }}>Local error log</div>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>Review recent frontend and backend failures stored locally, then export them if you need support.</div>
                </div>
                <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                  <GlowButton label="Refresh" variant="ghost" onClick={() => void loadErrorLog()} loading={loadingErrorLog} />
                  <GlowButton label="Export" variant="primary" onClick={() => void exportErrorLog()} loading={exportingErrorLog} />
                </div>
              </div>
              <div style={{ display: "grid", gap: "var(--sp-2)" }}>
                {errorEntries.length > 0 ? errorEntries.slice(-12).reverse().map((entry, index) => (
                  <div key={`${entry.source}-${index}`} className="surface-subtle" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "grid", gap: 6 }}>
                    <div className="badge-row">
                      <StatusPill status={entry.source === "gui_runtime" ? "info" : "warn"} label={entry.source.replace(/_/g, " ")} />
                    </div>
                    <div className="tiny" style={{ color: "var(--text-primary)", whiteSpace: "pre-wrap", wordBreak: "break-word" }}>{entry.message}</div>
                  </div>
                )) : (
                  <div className="surface-subtle" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)" }}>
                    <div className="tiny" style={{ color: "var(--text-secondary)" }}>No local error entries were found yet.</div>
                  </div>
                )}
              </div>
            </div>
          </div>
        ) : null}

        {tab === "Advanced" ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <GlassCard glow="warn">
              <SectionHeader
                title="Advanced Controls"
                subtitle="Specialist process, storage, network, startup, RAM, and thermal tools live here for power users."
              />
            </GlassCard>
            <Tools />
          </div>
        ) : null}

        {tab === "About" ? (
          <GlassCard glow="accent">
            <div className="title-kicker" style={{ marginBottom: 8 }}>PulseBoost AI</div>
            <div className="font-display" style={{ marginBottom: "var(--sp-2)", fontSize: 24, fontWeight: 800 }}>Premium desktop optimization suite</div>
            <div className="tiny" style={{ color: "var(--text-secondary)" }}>
              Real-time telemetry, local adaptive intelligence, tweak management, benchmarking, and game optimization in one Windows desktop shell.
            </div>
          </GlassCard>
        ) : null}
      </GlassCard>
    </div>
  );
}

