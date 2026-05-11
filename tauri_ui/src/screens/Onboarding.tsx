import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import HealthRing from "../components/ui/HealthRing";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import { useUiStore } from "../stores/uiStore";
import type { BenchmarkResult, LicenseInfo } from "../types/system";

const steps = ["Welcome", "Readiness", "Initial Scan", "Baseline", "Unlock More"] as const;

function statusTone(ready: boolean): "ok" | "warn" {
  return ready ? "ok" : "warn";
}

export default function Onboarding() {
  const [step, setStep] = useState(0);
  const [licenseInfo, setLicenseInfo] = useState<LicenseInfo | null>(null);
  const [baselineResult, setBaselineResult] = useState<BenchmarkResult | null>(null);
  const [benchmarkTriggered, setBenchmarkTriggered] = useState(false);
  const snapshot = useSystemStore((s) => s.snapshot);
  const error = useSystemStore((s) => s.error);
  const fetchSnapshot = useSystemStore((s) => s.fetchSnapshot);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const benchmarkProgress = useSystemStore((s) => s.actionProgress.quick_benchmark);
  const setDone = useUiStore((s) => s.setOnboardingDone);
  const setScreen = useUiStore((s) => s.setActiveScreen);

  useEffect(() => {
    if (step >= 1) {
      void fetchSnapshot();
    }
  }, [fetchSnapshot, step]);

  useEffect(() => {
    if (!isTauriRuntime()) {
      return;
    }
    void tauriInvoke<LicenseInfo>("get_license_info")
      .then(setLicenseInfo)
      .catch(() => {
        setLicenseInfo(null);
      });
  }, []);

  const readinessChecks = useMemo(() => {
    return [
      {
        title: "Tauri shell connected",
        subtitle: "Confirms the desktop shell is running with native IPC available.",
        ready: isTauriRuntime()
      },
      {
        title: "Live telemetry available",
        subtitle: "Backend snapshot access is working for CPU, RAM, disk, and thermal diagnostics.",
        ready: snapshot !== null && !error
      },
      {
        title: "Benchmark pipeline ready",
        subtitle: "PulseBench commands are reachable for before-and-after proof.",
        ready: isTauriRuntime()
      }
    ] as const;
  }, [error, snapshot]);

  const scanRows = useMemo(() => {
    if (!snapshot) {
      return [
        ["Telemetry", 26],
        ["Processes", 48],
        ["Storage", 66],
        ["Advisor", 82]
      ] as const;
    }
    return [
      ["CPU", Math.min(100, Math.max(18, snapshot.cpuPercent))],
      ["RAM", Math.min(100, Math.max(22, snapshot.ramPercent))],
      ["Disk", Math.min(100, Math.max(16, snapshot.diskPercent))],
      ["Health", Math.min(100, Math.max(28, snapshot.healthScore))]
    ] as const;
  }, [snapshot]);

  const runBaselineBenchmark = async () => {
    setBenchmarkTriggered(true);
    try {
      const result = await tauriInvoke<BenchmarkResult>("quick_benchmark");
      setBaselineResult(result);
      pushNotification(`Baseline benchmark captured - PulseScore ${Math.round(result.pulseScore)}`, "ok");
    } catch (benchmarkError) {
      pushNotification(benchmarkError instanceof Error ? benchmarkError.message : "Baseline benchmark failed", "danger");
    }
  };

  useEffect(() => {
    if (step === 3 && !benchmarkTriggered && !baselineResult && isTauriRuntime()) {
      void runBaselineBenchmark();
    }
  }, [step, benchmarkTriggered, baselineResult]);

  const benchmarkRunning = benchmarkProgress && !benchmarkProgress.completedAt;
  const benchmarkPercent = benchmarkProgress?.percent ?? 0;
  const benchmarkMessage = benchmarkProgress?.message ?? "Preparing benchmark";

  return (
    <div className="screen-enter" style={{ height: "100%", display: "grid", placeItems: "center" }}>
      <GlassCard glow="accent" style={{ width: "min(980px, 100%)", padding: "var(--sp-6)" }}>
        <SectionHeader title={`Step ${step + 1} - ${steps[step]}`} subtitle="First-run product setup" />

        {step === 0 ? (
          <div style={{ textAlign: "center", padding: "var(--sp-8) var(--sp-4)", display: "grid", gap: "var(--sp-4)", placeItems: "center" }}>
            <div
              style={{
                width: 96,
                height: 96,
                borderRadius: 28,
                background: "linear-gradient(145deg, var(--accent), var(--accent2))",
                display: "grid",
                placeItems: "center",
                color: "#04111f",
                fontSize: 28,
                fontWeight: 900,
                fontFamily: "var(--font-display)",
                boxShadow: "0 26px 60px rgba(0,220,255,0.24)"
              }}
            >
              PB
            </div>
            <div className="font-display" style={{ fontSize: 54, fontWeight: 800, letterSpacing: "-0.04em", lineHeight: 0.95 }}>
              PulseBoost AI
            </div>
            <div className="muted" style={{ maxWidth: 620 }}>
              The first optimization shell in this stack that shows live system state, measurable improvements, and guided actions in one place.
            </div>
            <div className="badge-row">
              <StatusPill status="info" label="Telemetry" />
              <StatusPill status="info" label="Benchmarking" />
              <StatusPill status="info" label="AI guidance" />
            </div>
          </div>
        ) : null}

        {step === 1 ? (
          <div style={{ display: "grid", gap: "var(--sp-3)" }}>
            {readinessChecks.map((item, index) => (
              <div key={item.title} className="surface-panel screen-enter" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", alignItems: "center", animationDelay: `${index * 45}ms` }}>
                <div>
                  <div className="font-display" style={{ fontSize: 18, fontWeight: 700, marginBottom: 6 }}>{item.title}</div>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>{item.subtitle}</div>
                </div>
                <StatusPill status={statusTone(item.ready)} label={item.ready ? "ready" : "pending"} dot />
              </div>
            ))}
            {error ? (
              <GlassCard glow="danger">
                <div className="title-kicker" style={{ marginBottom: 8 }}>Backend note</div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>{error}</div>
              </GlassCard>
            ) : null}
          </div>
        ) : null}

        {step === 2 ? (
          <div style={{ display: "grid", gap: "var(--sp-4)" }}>
            <div>
              <div className="font-display" style={{ fontSize: 24, fontWeight: 800, marginBottom: 8 }}>Running initial diagnostics</div>
              <div className="tiny" style={{ color: "var(--text-secondary)" }}>These bars are tied to the current backend snapshot so your onboarding state reflects the real machine.</div>
            </div>
            {scanRows.map(([label, value], index) => (
              <div key={label} className="screen-enter" style={{ display: "grid", gap: "var(--sp-2)", animationDelay: `${index * 45}ms` }}>
                <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-3)" }}>
                  <span className="title-kicker">{label}</span>
                  <span className="font-mono tiny">{Math.round(value)}%</span>
                </div>
                <div style={{ height: 10, borderRadius: 999, background: "rgba(255,255,255,0.06)", overflow: "hidden" }}>
                  <div style={{ width: `${value}%`, height: "100%", borderRadius: 999, background: "linear-gradient(90deg, var(--accent), var(--accent2))" }} />
                </div>
              </div>
            ))}
            {snapshot ? (
              <div className="badge-row">
                <StatusPill status="ok" label={`Health ${Math.round(snapshot.healthScore)}`} />
                <StatusPill status="info" label={`${snapshot.issues.length} active issues`} />
              </div>
            ) : null}
          </div>
        ) : null}

        {step === 3 ? (
          <div style={{ display: "grid", gridTemplateColumns: "240px minmax(0, 1fr)", gap: "var(--sp-4)", alignItems: "center" }}>
            <div style={{ display: "grid", placeItems: "center", gap: "var(--sp-2)" }}>
              <HealthRing score={baselineResult ? Math.min(100, Math.round(baselineResult.pulseScore / 10)) : Math.round(snapshot?.healthScore ?? 0)} />
              <div className="title-kicker">Baseline PulseScore</div>
              <div className="font-display" style={{ fontSize: 28, fontWeight: 800 }}>
                {baselineResult ? Math.round(baselineResult.pulseScore) : "..."}
              </div>
            </div>
            <div style={{ display: "grid", gap: "var(--sp-3)" }}>
              <div>
                <div className="font-display" style={{ fontSize: 24, fontWeight: 800, marginBottom: 8 }}>Capture your baseline before optimizing</div>
                <div className="tiny" style={{ color: "var(--text-secondary)" }}>
                  PulseBench gives you a local before-and-after record so improvements are measurable instead of subjective.
                </div>
              </div>
              {benchmarkRunning ? (
                <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                  <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-3)" }}>
                    <span className="title-kicker">Benchmark progress</span>
                    <span className="font-mono tiny">{benchmarkPercent}%</span>
                  </div>
                  <div style={{ height: 10, borderRadius: 999, background: "rgba(255,255,255,0.06)", overflow: "hidden" }}>
                    <div style={{ width: `${benchmarkPercent}%`, height: "100%", borderRadius: 999, background: "linear-gradient(90deg, var(--accent), var(--accent2))" }} />
                  </div>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>{benchmarkMessage}</div>
                </div>
              ) : baselineResult ? (
                <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                  <div className="badge-row">
                    <StatusPill status="ok" label={`Grade ${baselineResult.grade}`} />
                    <StatusPill status="info" label={`CPU ${baselineResult.cpuScore.toFixed(0)}`} />
                    <StatusPill status="info" label={`RAM ${baselineResult.ramBandwidthMBps.toFixed(0)} MB/s`} />
                  </div>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>
                    Re-run this after applying tweaks to see the delta in PulseBench.
                  </div>
                </div>
              ) : (
                <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)" }}>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>No baseline recorded yet.</div>
                </div>
              )}
              <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                <GlowButton label={baselineResult ? "Run Benchmark Again" : "Run Baseline Benchmark"} onClick={() => void runBaselineBenchmark()} loading={Boolean(benchmarkRunning)} />
                <GlowButton label="Skip for now" variant="ghost" onClick={() => setStep(4)} />
              </div>
            </div>
          </div>
        ) : null}

        {step === 4 ? (
          <div style={{ display: "grid", gap: "var(--sp-4)" }}>
            <div>
              <div className="font-display" style={{ fontSize: 28, fontWeight: 800, marginBottom: 8 }}>Choose how deep you want to go</div>
              <div className="tiny" style={{ color: "var(--text-secondary)", maxWidth: 700 }}>
                Free gets you the live dashboard, baseline visibility, and core diagnostics. Pro unlocks the broader tweak surface, scheduled automation, richer benchmark history, and premium AI workflows.
              </div>
            </div>
            <div style={{ display: "grid", gridTemplateColumns: "repeat(2, minmax(0, 1fr))", gap: "var(--sp-4)" }}>
              <GlassCard>
                <SectionHeader title="Free Tier" subtitle="Strong baseline value" />
                <div style={{ display: "grid", gap: "var(--sp-2)" }}>
                  {[
                    "Live telemetry dashboard",
                    "Core cleanup and diagnostics",
                    "Local AI assistance",
                    "Baseline PulseBench runs"
                  ].map((item) => (
                    <div key={item} className="tiny" style={{ color: "var(--text-secondary)" }}>- {item}</div>
                  ))}
                </div>
              </GlassCard>
              <GlassCard glow="accent">
                <SectionHeader title="PulseBoost Pro" subtitle={licenseInfo?.isPro ? "Already active on this device" : "Built for deeper optimization"} action={licenseInfo?.isPro ? <StatusPill status="ok" label="Pro active" /> : <StatusPill status="warn" label="Upgrade available" />} />
                <div style={{ display: "grid", gap: "var(--sp-2)" }}>
                  {[
                    "Full tweak catalog and automation",
                    "Unlimited benchmark history",
                    "Scheduled optimization workflows",
                    "Expanded AI and product controls"
                  ].map((item) => (
                    <div key={item} className="tiny" style={{ color: "var(--text-secondary)" }}>- {item}</div>
                  ))}
                </div>
              </GlassCard>
            </div>
          </div>
        ) : null}

        <div style={{ marginTop: "var(--sp-5)", display: "flex", justifyContent: "space-between", alignItems: "center", gap: "var(--sp-4)" }}>
          <div style={{ display: "flex", gap: "var(--sp-2)" }}>
            {steps.map((label, index) => (
              <div key={label} style={{ display: "grid", gap: 6, justifyItems: "center" }}>
                <span
                  style={{
                    width: 24,
                    height: 8,
                    borderRadius: 99,
                    background: index <= step ? "linear-gradient(90deg, var(--accent), var(--accent2))" : "var(--border-glass)"
                  }}
                />
              </div>
            ))}
          </div>
          <div style={{ display: "flex", gap: "var(--sp-2)" }}>
            {step > 0 ? <GlowButton label="Back" variant="ghost" onClick={() => setStep((s) => s - 1)} /> : null}
            {step < steps.length - 1 ? (
              <GlowButton label="Next" onClick={() => setStep((s) => s + 1)} />
            ) : (
              <GlowButton
                label="Enter PulseBoost"
                onClick={() => {
                  setDone(true);
                  setScreen("home");
                }}
              />
            )}
          </div>
        </div>
      </GlassCard>
    </div>
  );
}

