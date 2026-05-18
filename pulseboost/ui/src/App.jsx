import { useEffect, useMemo, useRef, useState } from "react";
import {
  FolderOpenIcon,
  HardDriveIcon,
  MinusIcon,
  RefreshCwIcon,
  SquareIcon,
  XIcon,
  ZapIcon,
} from "lucide-react";

import {
  applyGpuSetting,
  applyNetworkQos,
  applyTweak,
  createLocalSession,
  exportGameProfile,
  exportBenchmarkResult,
  fetchAudit,
  fetchBenchmarkResults,
  fetchBiosChecklist,
  fetchHealthHistory,
  fetchGameProfile,
  fetchGames,
  fetchGpuStats,
  fetchNetworkDiagnostics,
  fetchOptimizations,
  fetchSettings,
  fetchSuggestions,
  fetchState,
  fetchStatus,
  fetchTrustCenterStatus,
  fetchTweakCatalog,
  importGameProfile,
  refreshEntitlements,
  revertAuditEntry,
  revertTweak,
  rollbackAllTrustChanges,
  runBenchmark,
  runSettingsDataAction,
  saveGameProfile,
  setExpertMode,
  signOutSession,
  submitOptimizationDecision,
  toggleAdaptive,
  undoTrustCategory,
  updateSettingsPreferences,
} from "./api/client";
import Sidebar from "./components/Sidebar";
import StatusBadge from "./components/StatusBadge";
import OnboardingOverlay from "./components/OnboardingOverlay";
import { useWebSocket } from "./hooks/useWebSocket";
import AccountPage from "./pages/AccountPage";
import AuditPage from "./pages/AuditPage";
import BenchmarkPage from "./pages/BenchmarkPage";
import DashboardPage from "./pages/DashboardPage";
import GpuPage from "./pages/GpuPage";
import NetworkPage from "./pages/NetworkPage";
import PulseCorePage from "./pages/PulseCorePage";
import ProfilesPage from "./pages/ProfilesPage";
import SettingsPage from "./pages/SettingsPage";
import TrustPage from "./pages/TrustPage";
import useSystemStore from "./store/useSystemStore";
import { formatBytes, formatPercent } from "./utils/formatters";

const PAGE_META = {
  dashboard: "Realtime machine state, trust posture, and next best actions.",
  pulsecore: "Unified optimization intelligence, one-click boost, and PulseAI guidance.",
  network: "Protocol-aware diagnostics and honest network capability surfaces.",
  gpu: "Vendor-aware telemetry, advisory guidance, and safe control boundaries.",
  audit: "Append-only evidence for every system action and recovery event.",
  benchmark: "Before-and-after proof capture with verdict-driven summaries.",
  profiles: "Per-game optimization posture linked to local evidence and exports.",
  trust: "Safety, rollback, unsupported capability disclosure, and expert gating.",
  settings: "Desktop behavior, performance defaults, and safety preferences.",
  account: "Identity, entitlement posture, device activation, and plan surfaces.",
};

const runtimeLabel = (runtime) => (runtime === "electron" ? "Electron Desktop" : runtime === "pywebview" ? "PyWebView Desktop" : runtime || "Desktop");
const planLabel = (plan) => (!plan ? "FREE" : String(plan).toUpperCase());
const featureEnabled = (access, key) => Boolean(access?.[key]);

export default function App() {
  const { sendChat } = useWebSocket();
  const fileInputRef = useRef(null);
  const settingsImportRef = useRef(null);
  const previousGameRef = useRef("");
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false);
  const [desktopMeta, setDesktopMeta] = useState(null);
  const [benchmarkWorkload, setBenchmarkWorkload] = useState("Current Session");
  const [benchmarkNotes, setBenchmarkNotes] = useState("");
  const [benchmarkDuration, setBenchmarkDuration] = useState(6);
  const [selectedBenchmarkTweaks, setSelectedBenchmarkTweaks] = useState([]);
  const [benchmarkRunning, setBenchmarkRunning] = useState(false);
  const [selectedBenchmarkId, setSelectedBenchmarkId] = useState(null);
  const [selectedGameId, setSelectedGameId] = useState(null);
  const [selectedTweakId, setSelectedTweakId] = useState(null);
  const [pageLoading, setPageLoading] = useState(false);
  const [healthHistory, setHealthHistory] = useState([]);
  const [smartSuggestions, setSmartSuggestions] = useState([]);
  const [onboardingOpen, setOnboardingOpen] = useState(false);
  const [gameClosedToast, setGameClosedToast] = useState("");
  const [sidebarPulse, setSidebarPulse] = useState(false);
  const [settingsPayload, setSettingsPayload] = useState(null);
  const [settingsDataActionState, setSettingsDataActionState] = useState({
    busyAction: "",
    message: "",
    tone: "info",
  });

  const connected = useSystemStore((state) => state.connected);
  const healthScore = useSystemStore((state) => state.healthScore);
  const metrics = useSystemStore((state) => state.metrics);
  const sessionMode = useSystemStore((state) => state.sessionMode);
  const currentBottleneck = useSystemStore((state) => state.currentBottleneck);
  const frametime = useSystemStore((state) => state.frametime);
  const activeSession = useSystemStore((state) => state.activeSession);
  const adaptiveState = useSystemStore((state) => state.adaptive);
  const optimizations = useSystemStore((state) => state.optimizations);
  const processIntelligence = useSystemStore((state) => state.processIntelligence);
  const errorMessage = useSystemStore((state) => state.errorMessage);
  const foundationStatus = useSystemStore((state) => state.foundationStatus);
  const featureAccess = useSystemStore((state) => state.featureAccess);
  const authStatus = useSystemStore((state) => state.authStatus);
  const tweakCatalog = useSystemStore((state) => state.tweakCatalog);
  const auditEntries = useSystemStore((state) => state.auditEntries);
  const benchmarkResults = useSystemStore((state) => state.benchmarkResults);
  const networkDiagnostics = useSystemStore((state) => state.networkDiagnostics);
  const gpuStats = useSystemStore((state) => state.gpuStats);
  const biosChecklist = useSystemStore((state) => state.biosChecklist);
  const gameCatalog = useSystemStore((state) => state.gameCatalog);
  const currentGameProfile = useSystemStore((state) => state.currentGameProfile);
  const trustCenter = useSystemStore((state) => state.trustCenter);
  const activePage = useSystemStore((state) => state.activePage);
  const plan = useSystemStore((state) => state.plan);
  const setErrorMessage = useSystemStore((state) => state.setErrorMessage);
  const setFoundationStatus = useSystemStore((state) => state.setFoundationStatus);
  const setTweakCatalog = useSystemStore((state) => state.setTweakCatalog);
  const setAuditEntries = useSystemStore((state) => state.setAuditEntries);
  const setBenchmarkResults = useSystemStore((state) => state.setBenchmarkResults);
  const setNetworkDiagnostics = useSystemStore((state) => state.setNetworkDiagnostics);
  const setGpuStats = useSystemStore((state) => state.setGpuStats);
  const setBiosChecklist = useSystemStore((state) => state.setBiosChecklist);
  const setGameCatalog = useSystemStore((state) => state.setGameCatalog);
  const setCurrentGameProfile = useSystemStore((state) => state.setCurrentGameProfile);
  const setTrustCenter = useSystemStore((state) => state.setTrustCenter);
  const setActivePage = useSystemStore((state) => state.setActivePage);

  const runtimeIdentity = desktopMeta?.runtime || foundationStatus?.runtime || "pywebview";
  const machineName = foundationStatus?.machine?.name || "Primary Machine";
  const signedIn = Boolean(authStatus?.signed_in);
  const accountLinks = authStatus?.links || {};
  const settingsPreferences = settingsPayload?.preferences || null;
  const recommendationFeed = useMemo(() => [...(optimizations || []), ...(foundationStatus?.optimizations || [])], [foundationStatus?.optimizations, optimizations]);
  const statusCells = [
    { label: "CPU", value: formatPercent(metrics?.cpu_total) },
    { label: "Memory", value: formatPercent(metrics?.ram_percent) },
    { label: "Traffic", value: formatBytes(Number(metrics?.net_recv_bytes || 0) + Number(metrics?.net_sent_bytes || 0)) },
    { label: "Runtime", value: runtimeLabel(runtimeIdentity) },
  ];
  const hardwareSummary = useMemo(() => ({
    cpu: foundationStatus?.hardware_profile?.cpu_name || "Unknown CPU",
    gpu: gpuStats?.model || foundationStatus?.hardware_profile?.gpu_model || "Unknown GPU",
    ram: foundationStatus?.hardware_profile?.ram_total_bytes
      ? `${(Number(foundationStatus.hardware_profile.ram_total_bytes) / (1024 ** 3)).toFixed(1)} GB`
      : "Unknown RAM",
  }), [foundationStatus?.hardware_profile, gpuStats?.model]);

  const refreshStatus = async () => {
    const [status, trust] = await Promise.all([fetchStatus(), fetchTrustCenterStatus()]);
    setFoundationStatus(status);
    setTrustCenter(trust);
  };
  const refreshTweakData = async () => {
    const tweaks = await fetchTweakCatalog();
    setTweakCatalog(tweaks);
    if (!selectedTweakId && tweaks[0]) setSelectedTweakId(tweaks[0].id);
  };
  const refreshHealthHistory = async () => setHealthHistory(await fetchHealthHistory(7));
  const refreshSuggestions = async () => setSmartSuggestions(await fetchSuggestions());
  const refreshAuditData = async () => setAuditEntries(await fetchAudit());
  const refreshBenchmarkData = async () => {
    const results = await fetchBenchmarkResults();
    setBenchmarkResults(results);
    if (!selectedBenchmarkId && results[0]) setSelectedBenchmarkId(results[0].benchmark_id);
  };
  const refreshNetworkData = async () => setNetworkDiagnostics(await fetchNetworkDiagnostics());
  const refreshGpuData = async () => {
    const [stats, checklist] = await Promise.all([fetchGpuStats(), fetchBiosChecklist()]);
    setGpuStats(stats);
    setBiosChecklist(checklist);
  };
  const refreshSettingsData = async () => {
    const payload = await fetchSettings();
    setSettingsPayload(payload);
  };
  const refreshProfiles = async () => {
    const games = await fetchGames();
    setGameCatalog(games);
    const target = selectedGameId || games[0]?.game_id;
    if (target) {
      setSelectedGameId(target);
      setCurrentGameProfile(await fetchGameProfile(target));
    }
  };
  const loadActivePageData = async (page = activePage, silent = false) => {
    if (!silent) setPageLoading(true);
    try {
      if (["dashboard", "pulsecore", "benchmark"].includes(page)) await refreshTweakData();
      if (["dashboard", "audit"].includes(page)) await refreshAuditData();
      if (page === "pulsecore") await Promise.all([refreshHealthHistory(), refreshSuggestions()]);
      if (page === "benchmark") await refreshBenchmarkData();
      if (page === "network") await refreshNetworkData();
      if (page === "gpu") await refreshGpuData();
      if (page === "profiles") await refreshProfiles();
      if (["dashboard", "trust", "settings", "account"].includes(page)) await refreshStatus();
      if (["settings", "account"].includes(page)) await refreshSettingsData();
    } catch (error) {
      setErrorMessage(error.message);
    } finally {
      if (!silent) setPageLoading(false);
    }
  };

  useEffect(() => { loadActivePageData(activePage).catch((error) => setErrorMessage(error.message)); }, [activePage]);
  useEffect(() => {
    const intervalMs = activePage === "dashboard" ? 12000 : activePage === "pulsecore" ? 5000 : 15000;
    const interval = window.setInterval(() => loadActivePageData(activePage, true).catch((error) => setErrorMessage(error.message)), intervalMs);
    return () => window.clearInterval(interval);
  }, [activePage, selectedGameId]);
  useEffect(() => {
    let active = true;
    const desktopApi = window.pulseboostDesktop;
    if (!desktopApi?.isDesktop) return () => { active = false; };
    const loadMeta = () => desktopApi.getMeta().then((meta) => active && setDesktopMeta(meta)).catch(() => active && setDesktopMeta({ isDesktop: true }));
    loadMeta();
    window.addEventListener("pulseboost-desktop-ready", loadMeta);
    return () => { active = false; window.removeEventListener("pulseboost-desktop-ready", loadMeta); };
  }, []);

  useEffect(() => {
    const onboardingKey = "pulseboost.onboarding.completed.v2";
    const done = window.localStorage.getItem(onboardingKey) === "true";
    if (!done) {
      setOnboardingOpen(true);
    }
  }, []);

  useEffect(() => {
    const onGameClosed = () => {
      setGameClosedToast("Game session ended - settings restored.");
      window.setTimeout(() => setGameClosedToast(""), 4200);
    };
    window.addEventListener("pulseboost:game-closed", onGameClosed);
    return () => window.removeEventListener("pulseboost:game-closed", onGameClosed);
  }, []);

  useEffect(() => {
    const currentGame = activeSession?.game_name || "";
    const previousGame = previousGameRef.current;
    if (!previousGame && currentGame) {
      setSidebarPulse(true);
      window.setTimeout(() => setSidebarPulse(false), 320);
    }
    previousGameRef.current = currentGame;
  }, [activeSession?.game_name]);

  useEffect(() => {
    const desktopApi = window.pulseboostDesktop;
    if (!desktopApi?.onShortcut) return undefined;
    return desktopApi.onShortcut(({ action }) => {
      if (action === "boost_now") {
        setActivePage("pulsecore");
        runPulseBoostNow().catch((error) => setErrorMessage(error?.message || "Boost shortcut failed."));
      }
      if (action === "focus_chat") {
        setActivePage("pulsecore");
        window.setTimeout(() => {
          window.dispatchEvent(new CustomEvent("pulseboost:focus-chat-input"));
        }, 120);
      }
      if (action === "go_pulsecore") {
        setActivePage("pulsecore");
      }
      if (action === "dismiss_dialog") {
        setOnboardingOpen(false);
      }
    });
  }, []);

  const confirm = (message) => window.confirm(message);
  const requestApplyTweak = async (tweak) => {
    await applyTweak(tweak.id);
    await Promise.all([refreshStatus(), refreshTweakData(), refreshAuditData()]);
  };
  const requestRevertTweak = async (tweak) => {
    await revertTweak(tweak.id);
    await Promise.all([refreshStatus(), refreshTweakData(), refreshAuditData()]);
  };
  const handleAdaptiveToggle = async (enabled) => { await toggleAdaptive(enabled); await refreshStatus(); };
  const requestApplyNetworkQos = async (profileName, protocol) => {
    if (!featureEnabled(featureAccess, "advanced_network_controls")) return setActivePage("account");
    if (!confirm(`Apply "${profileName}" network profile?`)) return;
    await applyNetworkQos(profileName, protocol);
    await Promise.all([refreshNetworkData(), refreshStatus()]);
  };
  const requestApplyGpuSetting = async (setting) => {
    if (!featureEnabled(featureAccess, "advanced_gpu_controls")) return setActivePage("account");
    if (!confirm(`Apply "${setting.label}"?`)) return;
    await applyGpuSetting(setting.id, true, setting.risk === "moderate");
    await refreshGpuData();
  };
  const handleSaveCurrentProfile = async () => {
    if (!currentGameProfile?.game_id) return;
    setCurrentGameProfile(await saveGameProfile(currentGameProfile.game_id, currentGameProfile));
    await refreshProfiles();
  };
  const handleExportCurrentProfile = async (gameId = currentGameProfile?.game_id) => {
    if (!gameId) return;
    const data = await exportGameProfile(gameId);
    const blob = new Blob([data], { type: "application/json;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `${gameId}.pbprofile`;
    link.click();
    URL.revokeObjectURL(url);
  };
  const handleImportProfile = async (event) => {
    const file = event.target.files?.[0];
    if (!file) return;
    await importGameProfile(JSON.parse(await file.text()));
    await refreshProfiles();
    event.target.value = "";
  };
  const handleCreateProfile = () => {
    const gameId = `custom-${Date.now()}`;
    setSelectedGameId(gameId);
    setCurrentGameProfile({
      game_id: gameId,
      game_name: "Custom Profile",
      executable_path: "",
      recommended_tweaks: [],
      notes: "",
      history: {},
      recommendation_basis: {},
    });
  };
  const handleSettingsPreferenceChange = async (key, value) => {
    const next = {
      ...(settingsPreferences || {}),
      [key]: value,
    };
    setSettingsPayload((current) => ({
      ...(current || {}),
      preferences: next,
    }));
    await updateSettingsPreferences(next);
    if (key === "expertMode") {
      await refreshStatus();
      await refreshSettingsData();
    }
  };
  const handleExpertModeToggle = async (enabled) => {
    await setExpertMode(enabled);
    await Promise.all([refreshStatus(), refreshSettingsData()]);
  };
  const normalizeSettingsImportPayload = (rawPayload) => {
    if (!rawPayload || typeof rawPayload !== "object" || Array.isArray(rawPayload)) {
      throw new Error("Settings import file must contain a JSON object.");
    }
    if (rawPayload.preferences && typeof rawPayload.preferences === "object" && !Array.isArray(rawPayload.preferences)) {
      return rawPayload;
    }
    const settingsSection = rawPayload.settings;
    if (settingsSection && typeof settingsSection === "object" && !Array.isArray(settingsSection)) {
      const normalized = {};
      if (settingsSection.preferences && typeof settingsSection.preferences === "object" && !Array.isArray(settingsSection.preferences)) {
        normalized.preferences = settingsSection.preferences;
      }
      if (settingsSection.expert_mode && typeof settingsSection.expert_mode === "object" && !Array.isArray(settingsSection.expert_mode)) {
        normalized.expert_mode = settingsSection.expert_mode;
      }
      return Object.keys(normalized).length ? normalized : settingsSection;
    }
    return rawPayload;
  };
  const executeSettingsDataAction = async (action, payload = {}) => {
    setErrorMessage("");
    setSettingsDataActionState((current) => ({
      ...current,
      busyAction: action,
    }));
    try {
      const response = await runSettingsDataAction(action, payload);

      if (action === "export_all_data") {
        const exportPayload = response?.payload || {};
        const stamp = new Date().toISOString().replace(/[:.]/g, "-");
        const blob = new Blob([JSON.stringify(exportPayload, null, 2)], { type: "application/json;charset=utf-8" });
        const url = URL.createObjectURL(blob);
        const link = document.createElement("a");
        link.href = url;
        link.download = `pulseboost-export-${stamp}.json`;
        link.click();
        URL.revokeObjectURL(url);
        setSettingsDataActionState({
          busyAction: "",
          message: response?.empty_state
            ? "Export completed. Data store is currently in an empty state."
            : "Export completed successfully.",
          tone: "success",
        });
        return;
      }

      if (action === "import_settings") {
        await Promise.all([refreshStatus(), refreshSettingsData(), refreshAuditData()]);
        setSettingsDataActionState({
          busyAction: "",
          message: response?.empty_state
            ? "Import finished with an empty payload. Existing defaults remain in place."
            : "Settings imported successfully.",
          tone: "success",
        });
        return;
      }

      if (action === "clear_benchmark_history") {
        setSelectedBenchmarkId(null);
        await Promise.all([refreshBenchmarkData(), refreshAuditData()]);
        setSettingsDataActionState({
          busyAction: "",
          message: response?.empty_state
            ? "Benchmark history is already empty."
            : `Cleared ${response?.deleted_results || 0} benchmark result(s).`,
          tone: "success",
        });
        return;
      }

      if (action === "reset_all_settings") {
        await Promise.all([refreshStatus(), refreshSettingsData(), refreshAuditData()]);
        setSettingsDataActionState({
          busyAction: "",
          message: "Settings have been reset to safe defaults.",
          tone: "success",
        });
        return;
      }

      if (action === "export_report") {
        setSettingsDataActionState({
          busyAction: "",
          message: response?.path
            ? `Report exported: ${response.path}`
            : "Report exported successfully.",
          tone: "success",
        });
        return;
      }
    } catch (error) {
      const message = error?.message || "Settings data action failed.";
      setErrorMessage(message);
      setSettingsDataActionState({
        busyAction: "",
        message,
        tone: "error",
      });
    }
  };
  const handleSettingsDataAction = async (action) => {
    if (action === "import_settings") {
      settingsImportRef.current?.click();
      return;
    }
    await executeSettingsDataAction(action);
  };
  const handleSettingsDataImport = async (event) => {
    const file = event.target.files?.[0];
    if (!file) return;
    try {
      const parsed = JSON.parse(await file.text());
      const payload = normalizeSettingsImportPayload(parsed);
      await executeSettingsDataAction("import_settings", payload);
    } catch (error) {
      const message = error?.message || "Could not import settings from the selected file.";
      setErrorMessage(message);
      setSettingsDataActionState({
        busyAction: "",
        message,
        tone: "error",
      });
    } finally {
      event.target.value = "";
    }
  };
  const requestRevertAudit = async (entry) => {
    if (!confirm(`Revert "${entry.action}" for ${entry.target}?`)) return;
    await revertAuditEntry(entry.id);
    await Promise.all([refreshStatus(), refreshAuditData(), refreshTweakData()]);
  };
  const toggleBenchmarkTweak = (tweakId) => setSelectedBenchmarkTweaks((current) => current.includes(tweakId) ? current.filter((item) => item !== tweakId) : [...current, tweakId]);
  const requestRunBenchmark = async () => {
    if (!featureEnabled(featureAccess, "benchmark_history")) return setActivePage("account");
    if (!confirm("Run benchmark capture? Unsupported metrics will remain explicit.")) return;
    setBenchmarkRunning(true);
    try {
      const response = await runBenchmark({
        workload_name: benchmarkWorkload || activeSession?.game_name || machineName || "Current Session",
        tweak_set: selectedBenchmarkTweaks,
        duration_seconds: Number(benchmarkDuration) || 6,
        notes: benchmarkNotes,
        revert_after: true,
      });
      await Promise.all([refreshStatus(), refreshBenchmarkData(), refreshAuditData()]);
      if (response?.result?.benchmark_id) setSelectedBenchmarkId(response.result.benchmark_id);
    } finally {
      setBenchmarkRunning(false);
    }
  };
  const handleExportAudit = async () => {
    if (!featureEnabled(featureAccess, "audit_export")) return setActivePage("account");
    const response = await fetch("/api/actions/export");
    if (!response.ok) throw new Error("Could not export the audit log for the current plan.");
    const payload = await response.text();
    const blob = new Blob([payload], { type: "text/csv;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = "pulseboost-audit-log.csv";
    link.click();
    URL.revokeObjectURL(url);
  };
  const handleExportBenchmark = async (benchmarkId) => {
    if (!benchmarkId) return;
    const markdown = await exportBenchmarkResult(benchmarkId);
    const blob = new Blob([markdown], { type: "text/markdown;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `pulseboost-benchmark-${benchmarkId}.md`;
    link.click();
    URL.revokeObjectURL(url);
  };
  const handleLocalSignIn = async (payload) => {
    try {
      await createLocalSession(payload);
      await refreshStatus();
    } catch (error) {
      setErrorMessage(error?.message || "Could not start a local placeholder session.");
    }
  };
  const handleRefreshEntitlements = async () => {
    try {
      await refreshEntitlements();
      await refreshStatus();
    } catch (error) {
      setErrorMessage(error?.message || "Could not refresh entitlements.");
    }
  };
  const handleSignOut = async () => {
    try {
      await signOutSession();
      await refreshStatus();
    } catch (error) {
      setErrorMessage(error?.message || "Could not sign out.");
    }
  };
  const requestRollbackAll = async () => {
    await rollbackAllTrustChanges();
    await Promise.all([refreshStatus(), refreshTweakData(), refreshAuditData()]);
  };
  const requestPermissionUndo = async (category) => {
    await undoTrustCategory(category);
    await Promise.all([refreshStatus(), refreshTweakData(), refreshAuditData()]);
  };
  const runPulseBoostNow = async () => {
    const liveState = await fetchState();
    const latestOptimizations = liveState?.optimizations?.length
      ? liveState.optimizations
      : await fetchOptimizations(10);
    const pending = (latestOptimizations || []).find((item) => {
      const status = String(item?.status || "pending").toLowerCase();
      return !["dismissed", "done", "failed"].includes(status);
    });
    if (!pending) {
      return { status: "already_optimized", gain: 0 };
    }

    await submitOptimizationDecision(pending.id, "approve");
    await Promise.all([refreshStatus(), refreshTweakData(), refreshAuditData(), refreshSuggestions(), refreshHealthHistory()]);

    let estimatedGain = 0;
    try {
      const benchmark = await runBenchmark({
        workload_name: `boost_now_${Date.now()}`,
        tweak_set: [],
        duration_seconds: 2,
        notes: "PulseCore Boost Now estimate",
        revert_after: true,
      });
      const cpuDelta = Number(benchmark?.result?.cpu_delta);
      if (!Number.isNaN(cpuDelta) && cpuDelta < 0) {
        estimatedGain = Math.round(Math.abs(cpuDelta));
      }
      await refreshBenchmarkData();
      await Promise.all([refreshSuggestions(), refreshHealthHistory()]);
    } catch {
      estimatedGain = 0;
    }

    return { status: "boosted", gain: estimatedGain };
  };
  const handleSuggestionAction = (suggestion) => {
    const target = suggestion?.action?.target;
    if (target === "profiles") {
      setActivePage("profiles");
      return;
    }
    if (target === "audit") {
      setActivePage("audit");
      return;
    }
    if (target === "pulseai") {
      setActivePage("pulsecore");
      window.setTimeout(() => {
        window.dispatchEvent(new CustomEvent("pulseboost:focus-chat-input", { detail: { prompt: suggestion?.action?.prompt || "" } }));
      }, 120);
      return;
    }
    setActivePage("pulsecore");
  };
  const openExternalLink = async (url) => {
    if (!url) return;
    if (window.pulseboostDesktop?.openExternal) return window.pulseboostDesktop.openExternal(url);
    window.open(url, "_blank", "noopener,noreferrer");
  };

  const renderPage = () => {
    if (activePage === "dashboard") return <DashboardPage activeSession={activeSession} connected={connected} currentBottleneck={currentBottleneck} frametime={frametime} healthScore={healthScore} metrics={metrics} processIntelligence={processIntelligence} setActivePage={setActivePage} trustCenter={trustCenter} visibleOptimizations={recommendationFeed} />;
    if (activePage === "pulsecore") return <PulseCorePage activeSession={activeSession} auditEntries={auditEntries} foundationStatus={foundationStatus} gpuStats={gpuStats} healthHistory={healthHistory} healthScore={healthScore} metrics={metrics} onBoostNow={runPulseBoostNow} onOpenProfile={() => setActivePage("profiles")} onSuggestionAction={handleSuggestionAction} requestApplyTweak={requestApplyTweak} requestRevertTweak={requestRevertTweak} requestRollbackAll={requestRollbackAll} sendChat={sendChat} sessionMode={sessionMode} smartSuggestions={smartSuggestions} tweakCatalog={tweakCatalog} />;
    if (activePage === "network") return <NetworkPage featureAccess={featureAccess} networkDiagnostics={networkDiagnostics} refreshNetworkDiagnostics={refreshNetworkData} requestApplyNetworkQos={requestApplyNetworkQos} />;
    if (activePage === "gpu") return <GpuPage biosChecklist={biosChecklist} featureAccess={featureAccess} gpuStats={gpuStats} requestApplyGpuSetting={requestApplyGpuSetting} />;
    if (activePage === "audit") return <AuditPage auditEntries={auditEntries} featureAccess={featureAccess} handleExportAudit={handleExportAudit} requestRevertAudit={requestRevertAudit} />;
    if (activePage === "benchmark") return <BenchmarkPage benchmarkDuration={benchmarkDuration} benchmarkNotes={benchmarkNotes} benchmarkResults={benchmarkResults} benchmarkRunning={benchmarkRunning} benchmarkWorkload={benchmarkWorkload} onExportBenchmark={handleExportBenchmark} requestRunBenchmark={requestRunBenchmark} selectedBenchmark={benchmarkResults.find((item) => item.benchmark_id === selectedBenchmarkId) || benchmarkResults[0] || null} selectedBenchmarkTweaks={selectedBenchmarkTweaks} selectedTweakCatalog={tweakCatalog} setBenchmarkDuration={setBenchmarkDuration} setBenchmarkNotes={setBenchmarkNotes} setBenchmarkWorkload={setBenchmarkWorkload} setSelectedBenchmarkId={setSelectedBenchmarkId} toggleBenchmarkTweak={toggleBenchmarkTweak} />;
    if (activePage === "profiles") return <ProfilesPage currentGameProfile={currentGameProfile} featureAccess={featureAccess} fetchGameProfile={fetchGameProfile} fileInputRef={fileInputRef} gameCatalog={gameCatalog} handleCreateProfile={handleCreateProfile} handleExportCurrentProfile={handleExportCurrentProfile} handleSaveCurrentProfile={handleSaveCurrentProfile} refreshProfiles={refreshProfiles} selectedGameId={selectedGameId} setCurrentGameProfile={setCurrentGameProfile} setSelectedGameId={setSelectedGameId} />;
    if (activePage === "trust") return <TrustPage handleExpertModeToggle={handleExpertModeToggle} requestPermissionUndo={requestPermissionUndo} requestRollbackAll={requestRollbackAll} trustCenter={trustCenter} />;
    if (activePage === "account") return <AccountPage accountLinks={accountLinks} authStatus={authStatus} featureAccess={featureAccess} handleLocalSignIn={handleLocalSignIn} handleOpenCreateAccount={() => openExternalLink(accountLinks.create_account_url)} handleOpenManageSubscription={() => openExternalLink(accountLinks.manage_subscription_url)} handleOpenWebsiteSignIn={() => openExternalLink(accountLinks.sign_in_url)} handleRefreshEntitlements={handleRefreshEntitlements} handleSignOut={handleSignOut} handleExportReport={() => executeSettingsDataAction("export_report")} plan={plan} />;
    return <SettingsPage adaptiveState={adaptiveState} expertModeEnabled={Boolean(trustCenter?.expert_mode_state)} handleAdaptiveToggle={handleAdaptiveToggle} handleExpertModeToggle={handleExpertModeToggle} handleSettingsDataAction={handleSettingsDataAction} onPreferenceChange={handleSettingsPreferenceChange} preferences={settingsPreferences} settingsDataActionState={settingsDataActionState} />;
  };

  return (
    <div className="flex h-screen w-full flex-col overflow-hidden bg-app text-txt-primary">
      <OnboardingOverlay
        open={onboardingOpen}
        hardwareSummary={hardwareSummary}
        initialHealthScore={healthScore}
        activeGame={activeSession?.game_name}
        onFinish={() => {
          window.localStorage.setItem("pulseboost.onboarding.completed.v2", "true");
          setOnboardingOpen(false);
          setActivePage("pulsecore");
        }}
        onSkip={() => {
          window.localStorage.setItem("pulseboost.onboarding.completed.v2", "true");
          setOnboardingOpen(false);
        }}
      />
      <input ref={fileInputRef} type="file" accept=".json,.pbprofile" hidden onChange={handleImportProfile} />
      <input ref={settingsImportRef} type="file" accept=".json" hidden onChange={handleSettingsDataImport} />
      <header className="drag-region flex h-8 items-center justify-between border-b border-border-subtle bg-app px-3">
        <div className="flex items-center gap-2">
          <div className="flex h-4 w-4 items-center justify-center rounded bg-accent/20"><ZapIcon className="h-2.5 w-2.5 text-accent" /></div>
          <span className="text-xs font-semibold tracking-[0.16em] text-txt-tertiary">PulseBoost</span>
        </div>
        <div className="no-drag flex items-center gap-1">
          <button className="flex h-8 w-8 items-center justify-center text-txt-tertiary transition-colors hover:bg-surface-hover hover:text-txt-secondary" onClick={() => window.pulseboostDesktop?.minimizeWindow?.()} type="button"><MinusIcon className="h-3.5 w-3.5" /></button>
          <button className="flex h-8 w-8 items-center justify-center text-txt-tertiary transition-colors hover:bg-surface-hover hover:text-txt-secondary" onClick={() => window.pulseboostDesktop?.toggleMaximizeWindow?.()} type="button"><SquareIcon className="h-3 w-3" /></button>
          <button className="flex h-8 w-8 items-center justify-center text-txt-tertiary transition-colors hover:bg-error-muted hover:text-error" onClick={() => window.pulseboostDesktop?.closeWindow?.()} type="button"><XIcon className="h-3.5 w-3.5" /></button>
        </div>
      </header>
      <div className="flex flex-1 overflow-hidden">
        <Sidebar activePage={activePage} onNavigate={setActivePage} collapsed={sidebarCollapsed} onToggleCollapse={() => setSidebarCollapsed((current) => !current)} accountIdentity={authStatus?.identity} healthScore={healthScore} planLabel={planLabel(authStatus?.plan?.plan_tier || plan)} pulse={sidebarPulse} signedIn={signedIn} />
        <main className="flex flex-1 flex-col overflow-hidden">
          <div className="border-b border-border-subtle bg-app/80 px-6 py-4 backdrop-blur">
            <div className="mx-auto flex max-w-[1200px] flex-col gap-4">
              <div className="flex flex-col gap-3 xl:flex-row xl:items-start xl:justify-between">
                <div>
                  <p className="text-[10px] font-semibold uppercase tracking-[0.22em] text-txt-tertiary">Realtime Command Deck</p>
                  <h2 className="text-lg font-semibold text-txt-primary">{machineName}</h2>
                  <p className="text-sm text-txt-secondary">{PAGE_META[activePage] || "PulseBoost"}</p>
                </div>
                <div className="flex flex-wrap items-center gap-2">
                  <StatusBadge status={connected ? "LIVE" : "RECOVERY_MODE"} />
                  <StatusBadge status={signedIn ? "SIGNED_IN" : "SIGNED_OUT"} />
                  <StatusBadge status={featureEnabled(featureAccess, "benchmark_history") ? "PREMIUM" : "FREE_PLAN"} />
                  <button className="flex items-center gap-1.5 rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover" onClick={() => loadActivePageData(activePage)} type="button"><RefreshCwIcon className="h-3.5 w-3.5" /> Refresh</button>
                  <button className="flex items-center gap-1.5 rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover" onClick={() => window.pulseboostDesktop?.openDataDir?.()} type="button"><HardDriveIcon className="h-3.5 w-3.5" /> Data</button>
                  <button className="flex items-center gap-1.5 rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover" onClick={() => window.pulseboostDesktop?.openLogs?.()} type="button"><FolderOpenIcon className="h-3.5 w-3.5" /> Logs</button>
                </div>
              </div>
              <div className="grid gap-3 md:grid-cols-2 xl:grid-cols-4">
                {statusCells.map((cell) => (
                  <div key={cell.label} className="rounded-lg border border-border-default bg-surface-sunken px-4 py-3">
                    <div className="text-[10px] font-semibold uppercase tracking-[0.18em] text-txt-tertiary">{cell.label}</div>
                    <div className="numeric-tabular mt-1 text-lg font-semibold text-txt-primary">{cell.value}</div>
                  </div>
                ))}
              </div>
            </div>
          </div>
          <div className="flex-1 overflow-y-auto">
            <div className="mx-auto max-w-[1200px] space-y-4 p-6">
              {errorMessage ? <div className="rounded-lg border border-error/30 bg-error-muted px-4 py-3 text-sm text-error">{errorMessage}</div> : null}
              {pageLoading ? <div className="rounded-lg border border-border-default bg-surface p-3 text-xs uppercase tracking-[0.18em] text-txt-tertiary">Refreshing {activePage}...</div> : null}
              <div key={activePage} className="animate-page-enter">
                {renderPage()}
              </div>
            </div>
          </div>
        </main>
      </div>
      {gameClosedToast ? (
        <div className="pointer-events-none fixed bottom-5 right-5 z-[100] rounded-[12px] border border-border-default bg-surface px-4 py-3 text-sm text-txt-primary">
          {gameClosedToast}
        </div>
      ) : null}
    </div>
  );
}
