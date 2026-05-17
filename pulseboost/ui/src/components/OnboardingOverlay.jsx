import { useEffect, useMemo, useState } from "react";

function StepDots({ total, active }) {
  return (
    <div className="flex items-center gap-2">
      {Array.from({ length: total }).map((_, index) => (
        <span
          key={`dot-${index}`}
          className={[
            "h-2 w-2 rounded-full transition-colors",
            index === active ? "bg-[#2D7FF9]" : "bg-border-default",
          ].join(" ")}
        />
      ))}
    </div>
  );
}

export default function OnboardingOverlay({
  open,
  hardwareSummary,
  initialHealthScore,
  activeGame,
  onFinish,
  onSkip,
}) {
  const [step, setStep] = useState(0);
  const [scanProgress, setScanProgress] = useState(0);

  useEffect(() => {
    if (!open) {
      setStep(0);
      setScanProgress(0);
    }
  }, [open]);

  useEffect(() => {
    if (!open || step !== 1) return undefined;
    setScanProgress(0);
    const started = Date.now();
    const timer = window.setInterval(() => {
      const elapsed = Date.now() - started;
      const progress = Math.min(100, Math.round((elapsed / 5000) * 100));
      setScanProgress(progress);
    }, 100);
    return () => window.clearInterval(timer);
  }, [open, step]);

  const scoreReady = step !== 1 || scanProgress >= 100;
  const examplePrompt = useMemo(
    () => `Your ${hardwareSummary.gpu} is running at 71C - that's healthy. Want to optimize it for ${activeGame || "your game"}?`,
    [activeGame, hardwareSummary.gpu],
  );

  if (!open) return null;

  return (
    <div className="fixed inset-0 z-[120] flex items-center justify-center bg-[#0A0A0B]/90 px-4">
      <div className="w-full max-w-[720px] rounded-[12px] border border-border-default bg-surface p-6">
        <div className="mb-5 flex items-center justify-between">
          <h2 className="text-xl font-semibold text-txt-primary">Welcome to PulseBoost</h2>
          <button type="button" className="text-xs uppercase tracking-[0.14em] text-txt-tertiary hover:text-txt-secondary" onClick={onSkip}>
            Skip
          </button>
        </div>

        {step === 0 ? (
          <div className="space-y-4">
            <h3 className="text-lg font-medium text-txt-primary">Step 1 - Welcome to PulseBoost</h3>
            <p className="text-sm text-txt-secondary">We detected your hardware and prepared your optimization workspace.</p>
            <div className="rounded-[12px] border border-border-default bg-surface-sunken p-4 text-sm">
              <p><span className="text-txt-tertiary">CPU:</span> {hardwareSummary.cpu}</p>
              <p><span className="text-txt-tertiary">GPU:</span> {hardwareSummary.gpu}</p>
              <p><span className="text-txt-tertiary">RAM:</span> {hardwareSummary.ram}</p>
            </div>
          </div>
        ) : null}

        {step === 1 ? (
          <div className="space-y-4">
            <h3 className="text-lg font-medium text-txt-primary">Step 2 - Your first Health Score</h3>
            <p className="text-sm text-txt-secondary">Running quick system scan...</p>
            <div className="h-2 overflow-hidden rounded-full bg-[#111113]">
              <div className="h-full bg-[#2D7FF9] transition-all duration-100" style={{ width: `${scanProgress}%` }} />
            </div>
            <p className="numeric-tabular text-sm text-txt-tertiary">{scanProgress}%</p>
            {scoreReady ? (
              <div className="rounded-[12px] border border-[#2D7FF9]/30 bg-[#2D7FF9]/10 p-4">
                <p className="text-sm text-txt-secondary">Initial Health Score</p>
                <p className="numeric-tabular text-2xl font-semibold text-txt-primary">{Math.round(initialHealthScore)}/100</p>
              </div>
            ) : null}
          </div>
        ) : null}

        {step === 2 ? (
          <div className="space-y-4">
            <h3 className="text-lg font-medium text-txt-primary">Step 3 - Meet PulseAI</h3>
            <div className="space-y-3">
              <div className="ml-auto max-w-[80%] rounded-[12px] border border-[#2D7FF9] bg-[#2D7FF9]/15 px-3 py-2 text-sm text-[#EAF3FF]">
                How is my system looking right now?
              </div>
              <div className="max-w-[80%] rounded-[12px] border border-border-default bg-[#111214] px-3 py-2 text-sm text-txt-primary">
                {examplePrompt}
              </div>
            </div>
          </div>
        ) : null}

        {step === 3 ? (
          <div className="space-y-4">
            <h3 className="text-lg font-medium text-txt-primary">Step 4 - You&apos;re ready</h3>
            <p className="text-sm text-txt-secondary">Open PulseCore to run your first Boost cycle and monitor live system intel.</p>
          </div>
        ) : null}

        <div className="mt-6 flex items-center justify-between">
          <StepDots total={4} active={step} />
          <div className="flex items-center gap-2">
            {step > 0 ? (
              <button
                type="button"
                className="rounded-[10px] border border-border-default px-3 py-1.5 text-xs text-txt-secondary hover:bg-surface-hover"
                onClick={() => setStep((current) => Math.max(0, current - 1))}
              >
                Back
              </button>
            ) : null}
            {step < 3 ? (
              <button
                type="button"
                disabled={step === 1 && !scoreReady}
                className="rounded-[10px] bg-[#2D7FF9] px-4 py-1.5 text-xs font-semibold text-white hover:bg-[#3E89FA] disabled:opacity-60"
                onClick={() => setStep((current) => Math.min(3, current + 1))}
              >
                Next
              </button>
            ) : (
              <button
                type="button"
                className="rounded-[10px] bg-[#2D7FF9] px-4 py-1.5 text-xs font-semibold text-white hover:bg-[#3E89FA]"
                onClick={onFinish}
              >
                Go to PulseCore
              </button>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

