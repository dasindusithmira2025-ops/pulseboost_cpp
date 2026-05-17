import { useMemo, useState } from "react";
import {
  ChevronDownIcon,
  ChevronUpIcon,
  CpuIcon,
  Gamepad2Icon,
  GaugeIcon,
  HardDriveIcon,
  MemoryStickIcon,
  RotateCcwIcon,
  ShieldAlertIcon,
  TimerIcon,
  ZapIcon,
} from "lucide-react";

import AIPanel from "../components/AIPanel";
import Card from "../components/Card";
import SuggestionCard from "../components/SuggestionCard";
import SparklineChart from "../components/SparklineChart";
import { formatPercent } from "../utils/formatters";

const TAB_ORDER = ["Performance", "Memory", "Network"];

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function categoryForTweak(tweak) {
  const source = `${tweak?.id || ""} ${tweak?.name || ""} ${tweak?.category || ""}`.toLowerCase();
  if (source.includes("network")) return "Network";
  if (
    source.includes("memory")
    || source.includes("ram")
    || source.includes("sysmain")
    || source.includes("search")
    || source.includes("index")
    || source.includes("cortana")
  ) {
    return "Memory";
  }
  return "Performance";
}

function riskLevel(tweak) {
  if (tweak?.requires_admin) return "Advanced";
  const validity = String(tweak?.validity || "").toUpperCase();
  if (validity === "HARDWARE_SPECIFIC" || validity === "LEGACY") return "Moderate";
  return "Safe";
}

function riskBadgeClass(risk) {
  if (risk === "Advanced") return "border-red-500/50 bg-red-500/10 text-red-400";
  if (risk === "Moderate") return "border-amber-500/50 bg-amber-500/10 text-amber-300";
  return "border-green-500/50 bg-green-500/10 text-green-300";
}

function isTweakTimelineEntry(entry) {
  const module = String(entry?.module || "").toLowerCase();
  const action = String(entry?.action || "").toLowerCase();
  const tweakModules = new Set(["optimizer", "affinity", "process_priority", "registry", "services", "trust_center"]);
  const isKnownTweakModule = tweakModules.has(module);
  if (!isKnownTweakModule) return false;

  const hasSnapshotLink = Boolean(entry?.after_value && typeof entry.after_value === "object" && entry.after_value.revert_snapshot_id);
  const looksLikeTweakAction = action.includes("tweak")
    || action.startsWith("set_")
    || action.startsWith("restore_")
    || action.includes("rollback")
    || action.includes("undo")
    || action.includes("revert");
  return hasSnapshotLink || looksLikeTweakAction;
}

function isChangeAction(entry) {
  const action = String(entry?.action || "").toLowerCase();
  return action.includes("apply")
    || action.includes("revert")
    || action.startsWith("set_")
    || action.startsWith("restore_")
    || action.includes("rollback")
    || action.includes("undo");
}

function scoreTone(score) {
  if (score > 75) return "#22C55E";
  if (score >= 50) return "#F59E0B";
  return "#EF4444";
}

function HealthRing({ score }) {
  const safe = clamp(Math.round(score), 0, 100);
  const radius = 18;
  const circumference = 2 * Math.PI * radius;
  const offset = circumference - (safe / 100) * circumference;
  const color = scoreTone(safe);
  return (
    <div className="flex items-center gap-2">
      <svg viewBox="0 0 44 44" className="h-8 w-8 -rotate-90">
        <circle cx="22" cy="22" r={radius} stroke="#1E1E20" strokeWidth="4" fill="none" />
        <circle
          cx="22"
          cy="22"
          r={radius}
          stroke={color}
          strokeWidth="4"
          fill="none"
          strokeLinecap="round"
          strokeDasharray={circumference}
          strokeDashoffset={offset}
          style={{ transition: "stroke-dashoffset 200ms ease" }}
        />
      </svg>
      <span className="numeric-tabular text-sm font-semibold text-txt-primary">{safe}</span>
    </div>
  );
}

function ToggleSwitch({ checked, disabled, onClick }) {
  return (
    <button
      type="button"
      disabled={disabled}
      onClick={onClick}
      className={[
        "flex h-6 w-11 items-center rounded-full border px-1 transition-colors",
        checked ? "justify-end border-[#2D7FF9] bg-[#2D7FF9]/25" : "justify-start border-border-default bg-surface-sunken",
        disabled ? "cursor-not-allowed opacity-60" : "",
      ].join(" ")}
      aria-label={checked ? "Disable tweak" : "Enable tweak"}
    >
      <span className="h-4 w-4 rounded-full bg-white" />
    </button>
  );
}

function IntelCard({ icon: Icon, label, value, detail }) {
  return (
    <Card className="p-4">
      <div className="flex items-start justify-between gap-2">
        <div>
          <p className="text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">{label}</p>
          <p className="numeric-tabular mt-1 text-sm font-semibold text-txt-primary">{value}</p>
          {detail ? <p className="mt-1 text-xs text-txt-tertiary">{detail}</p> : null}
        </div>
        <Icon className="h-4 w-4 text-txt-secondary" />
      </div>
    </Card>
  );
}

function asGB(bytes) {
  if (bytes === null || bytes === undefined) return "n/a";
  return `${(Number(bytes) / (1024 ** 3)).toFixed(1)} GB`;
}

export default function PulseCorePage({
  activeSession,
  auditEntries,
  foundationStatus,
  gpuStats,
  healthScore,
  healthHistory,
  metrics,
  onBoostNow,
  onOpenProfile,
  onSuggestionAction,
  requestApplyTweak,
  requestRevertTweak,
  requestRollbackAll,
  sendChat,
  sessionMode,
  smartSuggestions,
  tweakCatalog,
}) {
  const [activeTab, setActiveTab] = useState("Performance");
  const [pendingTweaks, setPendingTweaks] = useState({});
  const [tweakErrors, setTweakErrors] = useState({});
  const [boostRunning, setBoostRunning] = useState(false);
  const [boostProgress, setBoostProgress] = useState(0);
  const [boostStep, setBoostStep] = useState("Ready");
  const [boostResult, setBoostResult] = useState(null);
  const [changesExpanded, setChangesExpanded] = useState(false);
  const [dismissedSuggestions, setDismissedSuggestions] = useState({});

  const groupedTweaks = useMemo(() => {
    const groups = { Performance: [], Memory: [], Network: [] };
    for (const tweak of tweakCatalog || []) {
      const category = categoryForTweak(tweak);
      groups[category].push({ ...tweak, risk: riskLevel(tweak) });
    }
    return groups;
  }, [tweakCatalog]);

  const appliedCounts = useMemo(() => ({
    Performance: groupedTweaks.Performance.filter((item) => item.applied).length,
    Memory: groupedTweaks.Memory.filter((item) => item.applied).length,
    Network: groupedTweaks.Network.filter((item) => item.applied).length,
  }), [groupedTweaks]);

  const appliedTweaksTotal = appliedCounts.Performance + appliedCounts.Memory + appliedCounts.Network;
  const cpuLoad = Number(metrics?.cpu_total || 0);
  const ramPercent = Number(metrics?.ram_percent || 0);
  const gpuTemp = Number(gpuStats?.temperature_c ?? metrics?.temperature ?? 0);
  const gpuNormalized = clamp(((gpuTemp - 30) / 70) * 100, 0, 100);
  const computedHealthScore = clamp(
    100 - cpuLoad * 0.3 - ramPercent * 0.3 - gpuNormalized * 0.2 + appliedTweaksTotal * 0.2,
    0,
    100,
  );
  const displayHealth = Number.isFinite(computedHealthScore) ? computedHealthScore : Number(healthScore || 0);
  const healthColor = scoreTone(displayHealth);

  const activeGame = activeSession?.game_name || null;
  const activeProfile = activeSession?.profile_name || activeSession?.profile || sessionMode || "normal";

  const recentTweakTimeline = useMemo(
    () => (auditEntries || [])
      .filter((entry) => isTweakTimelineEntry(entry))
      .slice(0, 10),
    [auditEntries],
  );

  const whatChanged = useMemo(
    () => recentTweakTimeline
      .filter((entry) => isChangeAction(entry))
      .slice(0, 8),
    [recentTweakTimeline],
  );

  const visibleSuggestions = useMemo(
    () => (smartSuggestions || []).filter((item) => !dismissedSuggestions[item.id]),
    [dismissedSuggestions, smartSuggestions],
  );

  const hardwareContextSummary = useMemo(() => {
    const profile = foundationStatus?.hardware_profile || {};
    const gpuName = gpuStats?.model || profile?.gpu_model || "GPU";
    const cpuName = profile?.cpu_name || "CPU";
    return `Using your ${gpuName} + ${cpuName} context`;
  }, [foundationStatus?.hardware_profile, gpuStats?.model]);

  const hardwareContextDetails = useMemo(() => {
    const profile = foundationStatus?.hardware_profile || {};
    const cpuName = profile?.cpu_name || "Unknown CPU";
    const gpuName = gpuStats?.model || profile?.gpu_model || "Unknown GPU";
    const ram = asGB(profile?.ram_total_bytes || metrics?.ram_total);
    const os = `${profile?.os_name || "Windows"} ${profile?.os_version || ""}`.trim();
    return `${gpuName} + ${cpuName} - ${ram} - ${os}`;
  }, [foundationStatus?.hardware_profile, gpuStats?.model, metrics?.ram_total]);

  const promptSuggestions = useMemo(() => {
    const gameName = activeGame || "gaming";
    return [
      `Why is my CPU at ${Math.round(cpuLoad)}%?`,
      `Optimize for ${gameName} now`,
      "What's my bottleneck?",
    ];
  }, [activeGame, cpuLoad]);

  const runBoost = async () => {
    if (boostRunning) return;
    setBoostRunning(true);
    setBoostProgress(0);
    setBoostResult(null);
    try {
      setBoostStep("Reading system state...");
      setBoostProgress(18);
      await new Promise((resolve) => window.setTimeout(resolve, 300));
      setBoostStep("Applying safe tweaks...");
      setBoostProgress(56);
      await new Promise((resolve) => window.setTimeout(resolve, 350));
      const result = await onBoostNow?.();
      setBoostStep("Calculating gain...");
      setBoostProgress(82);
      await new Promise((resolve) => window.setTimeout(resolve, 250));
      setBoostProgress(100);
      if (!result || result.status === "already_optimized") {
        setBoostResult({ title: "Already at peak - no changes needed", gain: 0 });
      } else {
        const gain = Number(result.gain || 0);
        setBoostResult({ title: `+${gain}% estimated gain`, gain });
      }
    } catch {
      setBoostResult({ title: "Boost cycle failed - check logs and retry.", gain: 0, error: true });
    } finally {
      setBoostRunning(false);
    }
  };

  const toggleTweak = async (tweak) => {
    const nextPending = { ...pendingTweaks, [tweak.id]: true };
    setPendingTweaks(nextPending);
    setTweakErrors((current) => ({ ...current, [tweak.id]: "" }));
    try {
      await new Promise((resolve) => window.setTimeout(resolve, 800));
      if (tweak.applied) {
        await requestRevertTweak?.(tweak);
      } else {
        await requestApplyTweak?.(tweak);
      }
    } catch (error) {
      setTweakErrors((current) => ({ ...current, [tweak.id]: error?.message || "Could not update this tweak." }));
    } finally {
      setPendingTweaks((current) => ({ ...current, [tweak.id]: false }));
    }
  };

  const executeSuggestionAction = (suggestion) => {
    onSuggestionAction?.(suggestion);
  };

  return (
    <div className="space-y-6">
      <section className="space-y-3">
        <div className="grid gap-3 md:grid-cols-2 xl:grid-cols-5">
          <IntelCard icon={CpuIcon} label="CPU" value={formatPercent(cpuLoad)} detail={gpuTemp ? `Temp ${Math.round(gpuTemp)}C` : "Temp unavailable"} />
          <IntelCard icon={MemoryStickIcon} label="RAM" value={`${asGB(metrics?.ram_used)} / ${asGB(metrics?.ram_total)}`} detail={formatPercent(ramPercent)} />
          <IntelCard icon={HardDriveIcon} label="GPU" value={`${gpuTemp ? `${Math.round(gpuTemp)}C` : "n/a"}`} detail={gpuStats?.memory_total_mb ? `${(Number(gpuStats.memory_used_mb || 0) / 1024).toFixed(1)} / ${(Number(gpuStats.memory_total_mb || 0) / 1024).toFixed(1)} GB VRAM` : "VRAM unavailable"} />
          <IntelCard icon={Gamepad2Icon} label="Active Game" value={activeGame || "No game detected"} detail={activeGame ? `Profile ${activeProfile}` : "Idle"} />
          <Card className="p-4">
            <div className="flex items-center justify-between">
              <div>
                <p className="text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">Health Score</p>
                <p className="numeric-tabular mt-1 text-sm font-semibold text-txt-primary">{Math.round(displayHealth)}/100</p>
              </div>
              <HealthRing score={displayHealth} />
            </div>
          </Card>
        </div>
        {activeGame ? (
          <div className="animate-page-enter rounded-[12px] border border-[#2D7FF9]/40 bg-[#2D7FF9]/10 px-4 py-3">
            <div className="flex flex-wrap items-center gap-2 text-sm">
              <span className="font-semibold text-[#DCEBFF]">Game Mode Active</span>
              <span className="text-txt-secondary">{activeGame}</span>
              <span className="text-txt-tertiary">Profile: {activeProfile}</span>
              <button type="button" className="ml-auto text-xs font-medium uppercase tracking-[0.12em] text-[#7EB2FF] hover:text-[#A8CCFF]" onClick={() => onOpenProfile?.()}>
                View Profile
              </button>
            </div>
          </div>
        ) : null}
      </section>

      <section className="grid gap-4 xl:grid-cols-[3fr_2fr]">
        <Card className="p-5">
          <div className="mb-4 flex flex-wrap items-center justify-between gap-3">
            <h2 className="text-lg font-semibold text-txt-primary">Smart Fix Panel</h2>
            <button
              type="button"
              onClick={() => {
                if (window.confirm("Revert all active tweaks?")) {
                  requestRollbackAll?.();
                }
              }}
              className="inline-flex items-center gap-1 rounded-[12px] border border-red-500/35 px-3 py-1.5 text-xs uppercase tracking-[0.12em] text-red-400 transition-colors hover:bg-red-500/10"
            >
              <RotateCcwIcon className="h-3.5 w-3.5" />
              Revert All
            </button>
          </div>
          <div className="mb-4 flex items-center gap-2">
            {TAB_ORDER.map((tab) => (
              <button
                key={tab}
                type="button"
                onClick={() => setActiveTab(tab)}
                className={[
                  "rounded-full border px-3 py-1 text-xs uppercase tracking-[0.12em] transition-colors",
                  activeTab === tab
                    ? "border-[#2D7FF9] bg-[#2D7FF9]/15 text-[#B9D5FF]"
                    : "border-border-default text-txt-tertiary hover:text-txt-secondary",
                ].join(" ")}
              >
                {tab} ({appliedCounts[tab]} applied)
              </button>
            ))}
          </div>
          <div className="space-y-3">
            {(groupedTweaks[activeTab] || []).map((tweak) => (
              <div key={tweak.id} className="rounded-[12px] border border-border-default bg-surface-sunken px-4 py-3">
                <div className="flex items-start justify-between gap-3">
                  <div className="min-w-0 flex-1">
                    <p className="truncate text-sm text-txt-primary">{tweak.name}</p>
                    <p className="mt-1 text-xs text-txt-tertiary">{tweak.impact || tweak.rationale || "No description available."}</p>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className={`rounded-full border px-2 py-0.5 text-[10px] uppercase tracking-[0.12em] ${riskBadgeClass(tweak.risk)}`}>
                      {tweak.risk}
                    </span>
                    <ToggleSwitch checked={Boolean(tweak.applied)} disabled={Boolean(pendingTweaks[tweak.id])} onClick={() => toggleTweak(tweak)} />
                  </div>
                </div>
                {pendingTweaks[tweak.id] ? (
                  <div className="mt-2 h-2 overflow-hidden rounded-full bg-[#111113]">
                    <div className="h-full w-full animate-skeleton bg-gradient-to-r from-[#111113] via-[#1A1A1C] to-[#111113]" />
                  </div>
                ) : null}
                {tweakErrors[tweak.id] ? (
                  <p className="mt-2 text-xs text-amber-400">{tweakErrors[tweak.id]}</p>
                ) : null}
              </div>
            ))}
            {!(groupedTweaks[activeTab] || []).length ? (
              <div className="rounded-[12px] border border-dashed border-border-default px-3 py-3 text-xs text-txt-tertiary">
                No tweaks available in this category.
              </div>
            ) : null}
          </div>
        </Card>

        <div className="space-y-4">
          <Card className="p-5">
            <h2 className="text-lg font-semibold text-txt-primary">Boost Panel</h2>
            <p className="mt-1 text-sm text-txt-secondary">One optimization cycle with a visible, auditable result.</p>
            {!boostRunning ? (
              <button
                type="button"
                onClick={runBoost}
                className="mt-4 inline-flex w-full items-center justify-center gap-2 rounded-[12px] bg-[#2D7FF9] px-4 py-3 text-sm font-semibold text-white transition-colors hover:bg-[#3E89FA]"
              >
                <ZapIcon className="h-4 w-4" />
                Boost Now
              </button>
            ) : (
              <div className="mt-4 space-y-2 rounded-[12px] border border-border-default bg-surface-sunken p-3">
                <div className="h-2 overflow-hidden rounded-full bg-[#111113]">
                  <div className="h-full rounded-full bg-[#2D7FF9] transition-all duration-200" style={{ width: `${boostProgress}%` }} />
                </div>
                <p className="text-xs text-txt-secondary">{boostStep}</p>
              </div>
            )}
            {boostResult ? (
              <div className="mt-3 rounded-[12px] border border-border-default bg-surface-sunken px-3 py-2 text-sm">
                <p className={boostResult.error ? "text-red-400" : "text-txt-primary"}>{boostResult.title}</p>
              </div>
            ) : null}
            <div className="mt-4 border-t border-border-subtle pt-3">
              <button
                type="button"
                onClick={() => setChangesExpanded((current) => !current)}
                className="flex w-full items-center justify-between text-xs uppercase tracking-[0.12em] text-txt-tertiary"
              >
                <span>What changed</span>
                {changesExpanded ? <ChevronUpIcon className="h-3.5 w-3.5" /> : <ChevronDownIcon className="h-3.5 w-3.5" />}
              </button>
              {changesExpanded ? (
                <div className="mt-2 space-y-2">
                  {whatChanged.length ? whatChanged.map((entry) => (
                    <div key={entry.id} className="rounded-[10px] border border-border-default bg-surface-sunken px-2 py-2">
                      <p className="text-xs text-txt-primary">{entry.action}</p>
                      <p className="mt-1 text-[11px] italic text-txt-tertiary">{entry.rationale || "No rationale recorded."}</p>
                    </div>
                  )) : (
                    <p className="text-xs text-txt-tertiary">No applied tweaks in this session yet.</p>
                  )}
                </div>
              ) : null}
            </div>
          </Card>

          <Card className="p-5">
            <div className="mb-3 flex items-center gap-2">
              <TimerIcon className="h-4 w-4 text-txt-secondary" />
              <h3 className="text-sm font-semibold text-txt-primary">Tweak Timeline</h3>
            </div>
            <div className="space-y-2">
              {recentTweakTimeline.length ? recentTweakTimeline.map((entry) => (
                <div key={entry.id} className="rounded-[10px] border border-border-default bg-surface-sunken px-3 py-2">
                  <div className="flex items-start justify-between gap-2">
                    <span className="text-xs text-txt-primary">{entry.action}</span>
                    <span className="numeric-tabular text-[11px] text-txt-tertiary">
                      {new Date(Number(entry.timestamp || 0) * 1000).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", hour12: false })}
                    </span>
                  </div>
                  <p className="mt-1 text-[11px] italic text-txt-tertiary">{entry.rationale || "No rationale provided."}</p>
                </div>
              )) : (
                <p className="text-xs text-txt-tertiary">No tweak timeline entries yet.</p>
              )}
            </div>
          </Card>

          <Card className="p-5">
            <div className="mb-2 flex items-center gap-2">
              <GaugeIcon className="h-4 w-4" style={{ color: healthColor }} />
              <h3 className="text-sm font-semibold text-txt-primary">PulseScore Timeline (7 days)</h3>
            </div>
            <SparklineChart data={healthHistory || []} height={110} />
          </Card>

          <div className="space-y-2">
            {visibleSuggestions.map((suggestion) => (
              <SuggestionCard
                key={suggestion.id}
                icon={suggestion.severity === "warning" ? ShieldAlertIcon : ZapIcon}
                text={suggestion.text}
                severity={suggestion.severity}
                action={{
                  label: suggestion.action?.label || "Fix This",
                  onClick: () => executeSuggestionAction(suggestion),
                }}
                onDismiss={() => setDismissedSuggestions((current) => ({ ...current, [suggestion.id]: true }))}
              />
            ))}
          </div>
        </div>
      </section>

      <Card className="p-5">
        <h2 className="mb-3 text-lg font-semibold text-txt-primary">PulseAI Chat</h2>
        <AIPanel
          sendChat={sendChat}
          contextSummary={hardwareContextSummary}
          contextDetails={hardwareContextDetails}
          suggestedPrompts={promptSuggestions}
        />
      </Card>
    </div>
  );
}
