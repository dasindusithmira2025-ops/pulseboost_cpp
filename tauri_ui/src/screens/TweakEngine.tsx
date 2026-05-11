import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { OptimizationPreset, SystemAdvisorItem, Tweak } from "../types/system";

type TweakPayload = {
  ok: boolean;
  tweaks: Tweak[];
  appliedCount: number;
  availableCount: number;
};

type PresetPayload = {
  ok: boolean;
  presets: OptimizationPreset[];
};

const categories = ["all", "windows", "gaming", "network", "ram", "gpu", "privacy", "power"] as const;
type Category = (typeof categories)[number];

function impactStatus(impact: Tweak["impact"]): "danger" | "warn" | "ok" {
  if (impact === "high") {
    return "danger";
  }
  if (impact === "medium") {
    return "warn";
  }
  return "ok";
}

function riskStatus(risk: Tweak["risk"]): "danger" | "warn" | "ok" {
  if (risk === "advanced") {
    return "danger";
  }
  if (risk === "moderate") {
    return "warn";
  }
  return "ok";
}

export default function TweakEngine() {
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const fetchSnapshot = useSystemStore((s) => s.fetchSnapshot);
  const lastProof = useSystemStore((s) => s.lastProof);
  const [payload, setPayload] = useState<TweakPayload | null>(null);
  const [presets, setPresets] = useState<OptimizationPreset[]>([]);
  const [advisorItems, setAdvisorItems] = useState<SystemAdvisorItem[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [advisorLoading, setAdvisorLoading] = useState(true);
  const [applyingId, setApplyingId] = useState<string | null>(null);
  const [batchAction, setBatchAction] = useState<string | null>(null);
  const [search, setSearch] = useState("");
  const [category, setCategory] = useState<Category>("all");

  const loadCritical = async () => {
    setIsLoading(true);
    try {
      const [tweaks, presetPayload] = await Promise.all([
        tauriInvoke<TweakPayload>("list_tweaks"),
        tauriInvoke<PresetPayload>("get_optimization_presets")
      ]);
      setPayload(tweaks);
      setPresets(presetPayload.presets ?? []);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load optimizations", "danger");
    } finally {
      setIsLoading(false);
    }
  };

  const loadAdvisor = async () => {
    setAdvisorLoading(true);
    try {
      const advisor = await tauriInvoke<{ ok: boolean; items: SystemAdvisorItem[] }>("get_system_advisor_summary");
      setAdvisorItems(
        (advisor.items ?? []).filter((item) => item.actionable && item.category !== "gpu" && item.category !== "bios").slice(0, 4)
      );
    } catch {
      setAdvisorItems([]);
    } finally {
      setAdvisorLoading(false);
    }
  };

  const load = async () => {
    await loadCritical();
    void loadAdvisor();
  };

  useEffect(() => {
    void load();
  }, []);

  const filtered = useMemo(() => {
    const tweaks = payload?.tweaks ?? [];
    return tweaks.filter((tweak) => {
      const matchesCategory = category === "all" || tweak.category === category;
      const q = search.trim().toLowerCase();
      const matchesSearch =
        q.length === 0 ||
        tweak.name.toLowerCase().includes(q) ||
        tweak.description.toLowerCase().includes(q) ||
        tweak.detailedInfo.toLowerCase().includes(q);
      return matchesCategory && matchesSearch;
    }).slice(0, 80);
  }, [category, payload?.tweaks, search]);

  const highImpactAvailable = useMemo(
    () => (payload?.tweaks ?? []).filter((tweak) => tweak.isApplicable && !tweak.isApplied && tweak.impact === "high").length,
    [payload?.tweaks]
  );

  const handleApply = async (id: string, revert = false) => {
    setApplyingId(id);
    try {
      const result = await tauriInvoke<{ ok: boolean; requiresRestart?: boolean; error?: string }>(
        revert ? "revert_tweak" : "apply_tweak",
        { id }
      );
      if (!result.ok) {
        throw new Error(result.error ?? `${revert ? "Revert" : "Apply"} failed`);
      }
      pushNotification(
        `${revert ? "Reverted" : "Applied"} ${id.replace(/_/g, " ")}${result.requiresRestart ? " - restart recommended" : ""}`,
        "ok"
      );
      await Promise.all([loadCritical(), fetchSnapshot()]);
      void loadAdvisor();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Tweak action failed", "danger");
    } finally {
      setApplyingId(null);
    }
  };

  const handleBatch = async (action: "apply_safe_tweaks" | "apply_high_impact_tweaks" | "revert_all_tweaks", label: string) => {
    setBatchAction(action);
    try {
      const result = await tauriInvoke<{ ok: boolean; successCount?: number; failureCount?: number; restartCount?: number }>(action);
      pushNotification(
        `${label}: ${result.successCount ?? 0} changed${(result.restartCount ?? 0) > 0 ? `, ${result.restartCount} restart` : ""}${(result.failureCount ?? 0) > 0 ? `, ${result.failureCount} failed` : ""}`,
        result.ok ? "ok" : "warn"
      );
      await Promise.all([loadCritical(), fetchSnapshot()]);
      void loadAdvisor();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Batch tweak action failed", "danger");
    } finally {
      setBatchAction(null);
    }
  };

  const handlePreset = async (presetId: string, label: string) => {
    setBatchAction(presetId);
    try {
      await tauriInvoke("apply_optimization_preset", { presetId });
      pushNotification(`${label} preset applied`, "ok");
      await Promise.all([loadCritical(), fetchSnapshot()]);
      void loadAdvisor();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Preset apply failed", "danger");
    } finally {
      setBatchAction(null);
    }
  };

  return (
    <div style={{ display: "grid", gap: "var(--sp-4)" }}>
      <GlassCard glow="accent">
        <SectionHeader
          title="Optimizations"
          subtitle="Start with curated presets, then drop into individual reversible tweaks only when you need the detail."
          eyebrow="Primary workflow"
          action={
            <div className="badge-row">
              <StatusPill status="info" label={`Applied ${payload?.appliedCount ?? 0}`} />
              <StatusPill status="neutral" label={`Available ${payload?.availableCount ?? 0}`} />
              <StatusPill status="warn" label={`High impact ${highImpactAvailable}`} />
            </div>
          }
        />

        <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) 320px", gap: "var(--sp-4)", alignItems: "stretch" }}>
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            <div className="title-kicker">Search and presets</div>
            <div style={{ display: "flex", gap: "var(--sp-3)", flexWrap: "wrap" }}>
              <input
                value={search}
                onChange={(event) => setSearch(event.target.value)}
                placeholder="Search tweaks, registry keys, service names, or descriptions"
                className="focus-ring input-shell"
                style={{
                  minWidth: 320,
                  flex: 1,
                  minHeight: 40,
                  borderRadius: "var(--r-xl)",
                  border: "1px solid var(--border-glass)",
                  padding: "0 var(--sp-4)",
                  background: "transparent"
                }}
              />
              <GlowButton label="Refresh" variant="ghost" onClick={() => void load()} loading={isLoading} />
            </div>

            <div style={{ display: "grid", gridTemplateColumns: "repeat(3, minmax(0, 1fr))", gap: "var(--sp-3)" }}>
              {presets.map((preset) => (
                <div key={preset.id} className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                  <div className="badge-row">
                    <StatusPill status={preset.impact === "high" ? "danger" : preset.impact === "medium" ? "warn" : "ok"} label={preset.impact} />
                    {preset.recommended ? <StatusPill status="info" label="recommended" /> : null}
                    {preset.proOnly ? <StatusPill status="warn" label="pro" /> : null}
                  </div>
                  <div className="font-display" style={{ fontSize: 18, fontWeight: 800 }}>{preset.label}</div>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>{preset.description}</div>
                  <GlowButton
                    label={`Apply ${preset.label}`}
                    variant={preset.recommended ? "primary" : "ghost"}
                    onClick={() => void handlePreset(preset.id, preset.label)}
                    loading={batchAction === preset.id}
                  />
                </div>
              ))}
            </div>

            <div className="badge-row">
              {categories.map((entry) => (
                <button
                  key={entry}
                  type="button"
                  className="focus-ring interactive"
                  onClick={() => setCategory(entry)}
                  style={{
                    minHeight: 30,
                    borderRadius: 999,
                    padding: "0 var(--sp-3)",
                    border: `1px solid ${category === entry ? "var(--border-accent)" : "var(--border-glass)"}`,
                    background: category === entry
                      ? "linear-gradient(135deg, rgba(0,220,255,0.18), rgba(122,92,255,0.12))"
                      : "rgba(255,255,255,0.03)",
                    color: category === entry ? "var(--text-primary)" : "var(--text-secondary)",
                    textTransform: "capitalize"
                  }}
                >
                  {entry.replace(/_/g, " ")}
                </button>
              ))}
            </div>
          </div>

          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-xl)", display: "grid", gap: "var(--sp-3)" }}>
            <div>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Optimization model</div>
              <div className="font-display" style={{ fontSize: 22, fontWeight: 800, lineHeight: 1.05, marginBottom: 8 }}>
                Apply the highest-return preset first, then tune individual settings only if you need more control.
              </div>
              <div className="muted">
                Restore-point thinking, reversibility, and explicit impact are surfaced here instead of hidden inside low-level utility screens.
              </div>
            </div>
            <div className="badge-row">
              <StatusPill status="ok" label="Revertable" />
              <StatusPill status="info" label="Preset-first" />
              <StatusPill status="warn" label="Restart surfaced" />
            </div>
            <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
              <GlowButton
                label="Apply All Safe"
                onClick={() => void handleBatch("apply_safe_tweaks", "Applied safe tweaks")}
                loading={batchAction === "apply_safe_tweaks"}
              />
              <GlowButton
                label="Apply High Impact"
                variant="ghost"
                onClick={() => void handleBatch("apply_high_impact_tweaks", "Applied high impact tweaks")}
                loading={batchAction === "apply_high_impact_tweaks"}
              />
              <GlowButton
                label="Revert All"
                variant="ghost"
                onClick={() => void handleBatch("revert_all_tweaks", "Reverted tweaks")}
                loading={batchAction === "revert_all_tweaks"}
              />
            </div>
          </div>
        </div>
      </GlassCard>

      {lastProof ? (
        <GlassCard glow="ok">
          <SectionHeader title="Proof of Work" subtitle="What the last optimization actually changed" eyebrow="Evidence" />
          <div style={{ display: "grid", gap: "var(--sp-2)" }}>
            <StatusPill status="ok" label={lastProof.action.replace(/_/g, " ")} />
            {lastProof.lines.map((line) => (
              <div key={line} className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", color: "var(--text-secondary)" }}>
                {line}
              </div>
            ))}
          </div>
        </GlassCard>
      ) : null}

      {advisorLoading ? (
        <GlassCard>
          <div className="skeleton" style={{ height: 160 }} />
        </GlassCard>
      ) : null}

      {advisorItems.length > 0 ? (
        <GlassCard glow="warn">
          <SectionHeader
            title="Recommended Next Improvements"
            subtitle="High-signal advisor findings that map cleanly to optimization actions"
            eyebrow="Recommendations"
          />
          <div style={{ display: "grid", gridTemplateColumns: "repeat(2, minmax(0, 1fr))", gap: "var(--sp-3)" }}>
            {advisorItems.map((item) => (
              <div key={item.id} className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                <div className="badge-row">
                  <StatusPill status={item.impact === "high" ? "danger" : item.impact === "medium" ? "warn" : "ok"} label={item.impact} />
                  <StatusPill status="info" label={item.category} />
                </div>
                <div className="font-display" style={{ fontSize: 17, fontWeight: 800 }}>{item.title}</div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>{item.description}</div>
                {item.actionId ? (
                  <GlowButton
                    label={item.actionLabel ?? "Apply"}
                    variant="ghost"
                    onClick={() => item.actionId ? void handleApply(item.actionId) : undefined}
                    loading={applyingId === item.actionId}
                  />
                ) : null}
              </div>
            ))}
          </div>
        </GlassCard>
      ) : null}

      {isLoading && !payload ? (
        <GlassCard>
          <div className="skeleton" style={{ height: 220 }} />
        </GlassCard>
      ) : null}

      <div style={{ display: "grid", gap: "var(--sp-3)" }}>
        {filtered.map((tweak, index) => (
          <GlassCard
            key={tweak.id}
            glow={tweak.isApplied ? "ok" : impactStatus(tweak.impact)}
            className="screen-enter"
            style={{ animationDelay: `${Math.min(index, 8) * 35}ms`, contentVisibility: "auto", containIntrinsicSize: "280px" }}
          >
            <div style={{ display: "grid", gap: "var(--sp-3)" }}>
              <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-4)", alignItems: "flex-start" }}>
                <div style={{ display: "grid", gap: "var(--sp-2)", minWidth: 0 }}>
                  <div className="badge-row">
                    <StatusPill status={impactStatus(tweak.impact)} label={tweak.impact} />
                    <StatusPill status={riskStatus(tweak.risk)} label={tweak.risk} />
                    <StatusPill status="info" label={tweak.category} />
                    {tweak.requiresRestart ? <StatusPill status="warn" label="restart" /> : null}
                    {tweak.isApplied ? <StatusPill status="ok" label="applied" dot /> : null}
                  </div>
                  <div className="font-display" style={{ fontSize: 20, fontWeight: 800, lineHeight: 1.05 }}>
                    {tweak.name}
                  </div>
                  <div style={{ color: "var(--text-secondary)", lineHeight: 1.6, maxWidth: 900 }}>{tweak.description}</div>
                </div>

                <div style={{ display: "flex", gap: "var(--sp-2)", flexShrink: 0 }}>
                  {tweak.isApplied ? (
                    <GlowButton
                      label="Revert"
                      variant="ghost"
                      onClick={() => void handleApply(tweak.id, true)}
                      loading={applyingId === tweak.id}
                    />
                  ) : (
                    <GlowButton
                      label="Apply"
                      onClick={() => void handleApply(tweak.id)}
                      loading={applyingId === tweak.id}
                      disabled={!tweak.isApplicable}
                    />
                  )}
                </div>
              </div>

              <div
                className="surface-panel"
                style={{
                  padding: "var(--sp-3)",
                  borderRadius: "var(--r-lg)",
                  display: "grid",
                  gap: "var(--sp-2)"
                }}
              >
                <div className="title-kicker">Change scope</div>
                <div className="tiny" style={{ color: "var(--text-muted)", wordBreak: "break-word" }}>
                  {tweak.detailedInfo}
                </div>
              </div>

              {!tweak.isApplicable && tweak.notApplicableReason ? (
                <div className="tiny" style={{ color: "var(--status-warn)" }}>
                  Not applicable: {tweak.notApplicableReason}
                </div>
              ) : null}
            </div>
          </GlassCard>
        ))}
      </div>
    </div>
  );
}
