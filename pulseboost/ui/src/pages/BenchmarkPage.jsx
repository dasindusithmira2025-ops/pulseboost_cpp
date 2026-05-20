import { useMemo, useState } from "react";
import { PlayIcon } from "lucide-react";

import Card from "../components/Card";
import StatusBadge from "../components/StatusBadge";
import { formatDateTime } from "../utils/formatters";

function deltaLabel(value, suffix = "") {
  if (value === null || value === undefined) return "Unavailable";
  const sign = Number(value) > 0 ? "+" : "";
  return `${sign}${value}${suffix}`;
}

function captureValue(capture, key, suffix = "") {
  const value = capture?.[key];
  return value === null || value === undefined ? "Unavailable" : `${value}${suffix}`;
}

function metricValue(value, suffix = "") {
  return value === null || value === undefined ? "Unavailable" : `${value}${suffix}`;
}

function normalizeBenchmarkResult(result) {
  if (!result) return null;
  const baseline = result.baseline || {};
  const optimized = result.optimized || {};
  return {
    ...result,
    baseline,
    optimized,
    frametime_supported: Boolean(result.frametime_supported),
    frametime_source: result.frametime_source ?? baseline.frametime_source ?? optimized.frametime_source ?? null,
    baseline_fps_average: result.baseline_fps_average ?? baseline.avg_fps ?? null,
    optimized_fps_average: result.optimized_fps_average ?? optimized.avg_fps ?? null,
    baseline_fps_1_low: result.baseline_fps_1_low ?? baseline.one_percent_low_fps ?? null,
    optimized_fps_1_low: result.optimized_fps_1_low ?? optimized.one_percent_low_fps ?? null,
    baseline_p95_frame_time_ms: result.baseline_p95_frame_time_ms ?? baseline.p95_frame_time_ms ?? null,
    optimized_p95_frame_time_ms: result.optimized_p95_frame_time_ms ?? optimized.p95_frame_time_ms ?? null,
    frame_time_reason: result.frame_time_reason ?? baseline.frametime_reason ?? optimized.frametime_reason ?? null,
  };
}

export default function BenchmarkPage({
  benchmarkDuration,
  benchmarkNotes,
  benchmarkResults,
  benchmarkRunning,
  benchmarkWorkload,
  onExportBenchmark,
  requestRunBenchmark,
  selectedBenchmark,
  selectedBenchmarkTweaks,
  selectedTweakCatalog,
  setBenchmarkDuration,
  setBenchmarkNotes,
  setBenchmarkWorkload,
  setSelectedBenchmarkId,
  toggleBenchmarkTweak,
}) {
  const [activeTab, setActiveTab] = useState("latest");
  const safeBenchmarkResults = useMemo(
    () => (benchmarkResults || []).map((item) => normalizeBenchmarkResult(item)).filter(Boolean),
    [benchmarkResults],
  );
  const latest = useMemo(
    () => normalizeBenchmarkResult(selectedBenchmark || safeBenchmarkResults[0] || null),
    [selectedBenchmark, safeBenchmarkResults],
  );

  const verdictHistory = useMemo(
    () => safeBenchmarkResults.map((item) => ({
      benchmark_id: item.benchmark_id,
      verdict: item.verdict,
      created_at: item.created_at,
    })),
    [safeBenchmarkResults],
  );

  const tweakChoices = (selectedTweakCatalog || []).map((item) => ({
    id: item.id,
    name: item.name,
  }));

  const comparisonBars = useMemo(() => {
    if (!latest) return [];
    const baselineCpu = Number(latest?.baseline?.cpu_percent);
    const optimizedCpu = Number(latest?.optimized?.cpu_percent);
    const baselineRam = Number(latest?.baseline?.ram_percent);
    const optimizedRam = Number(latest?.optimized?.ram_percent);
    const baselineLatency = Number(latest?.baseline?.ping_ms);
    const optimizedLatency = Number(latest?.optimized?.ping_ms);
    return [
      { label: "CPU Load", before: baselineCpu, after: optimizedCpu, suffix: "%" },
      { label: "RAM Usage", before: baselineRam, after: optimizedRam, suffix: "%" },
      { label: "Latency", before: baselineLatency, after: optimizedLatency, suffix: " ms" },
    ].filter((row) => Number.isFinite(row.before) && Number.isFinite(row.after));
  }, [latest]);

  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Benchmark
        </h1>
        <button
          className="flex items-center gap-2 rounded-md bg-accent px-4 py-1.5 text-sm font-medium text-white transition-colors hover:bg-accent-hover disabled:cursor-not-allowed disabled:opacity-70"
          onClick={() => requestRunBenchmark?.()}
          disabled={benchmarkRunning}
          type="button"
        >
          <PlayIcon className="h-4 w-4" /> {benchmarkRunning ? "Running..." : "Run Benchmark"}
        </button>
      </div>

      <div className="flex items-center gap-0 border-b border-border-subtle">
        {[
          { id: "latest", label: "Latest Result" },
          { id: "history", label: "History" },
          { id: "compare", label: "Compare" },
        ].map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`relative px-4 py-2 text-sm transition-colors ${
              activeTab === tab.id
                ? "text-txt-primary"
                : "text-txt-tertiary hover:text-txt-secondary"
            }`}
            type="button"
          >
            {tab.label}
            {activeTab === tab.id ? (
              <span className="absolute bottom-0 left-0 right-0 h-0.5 rounded-t bg-accent" />
            ) : null}
          </button>
        ))}
      </div>

      {activeTab === "latest" && latest ? (
        <div className="space-y-5">
          <div className="flex flex-wrap items-center gap-4 text-sm text-txt-tertiary">
            <span>Benchmark #{latest.benchmark_id?.slice(0, 8) || "latest"}</span>
            <span>·</span>
            <span>{formatDateTime(latest.created_at)}</span>
            <span>·</span>
            <span>Duration: {latest.duration_seconds}s</span>
            <span>·</span>
            <span>Tweaks active: {latest.tweak_set?.length || 0}</span>
          </div>

          <Card elevated className="p-6">
            <div className="mb-3 text-[11px] font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              Verdict
            </div>
            <div className="flex flex-col gap-3 md:flex-row md:items-center md:justify-between">
              <div>
                <div className="text-3xl font-semibold text-txt-primary">
                  {String(latest.verdict || "NO_MEASURABLE_IMPACT").replaceAll("_", " ")}
                </div>
                <div className="mt-1 text-sm text-txt-tertiary">
                  Unsupported metrics remain explicit instead of fabricated.
                </div>
              </div>
              <div className="flex items-center gap-2">
                <StatusBadge status={latest.verdict || "NO_MEASURABLE_IMPACT"} />
                <button
                  type="button"
                  onClick={() => onExportBenchmark?.(latest.benchmark_id)}
                  className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover"
                >
                  Export
                </button>
              </div>
            </div>
          </Card>

          {comparisonBars.length ? (
            <Card className="p-5">
              <h3 className="mb-3 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">Before Boost vs After Boost</h3>
              <div className="space-y-3">
                {comparisonBars.map((row) => (
                  <div key={row.label}>
                    <div className="mb-1 flex items-center justify-between text-xs text-txt-secondary">
                      <span>{row.label}</span>
                      <span className="numeric-tabular">Before {row.before}{row.suffix} - After {row.after}{row.suffix}</span>
                    </div>
                    <div className="grid grid-cols-2 gap-2">
                      <div className="h-2 overflow-hidden rounded-full bg-surface-sunken">
                        <div className="h-full bg-warning" style={{ width: `${Math.max(0, Math.min(100, row.before))}%` }} />
                      </div>
                      <div className="h-2 overflow-hidden rounded-full bg-surface-sunken">
                        <div className="h-full bg-success" style={{ width: `${Math.max(0, Math.min(100, row.after))}%` }} />
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </Card>
          ) : null}

          <div className="grid gap-4 md:grid-cols-2 xl:grid-cols-4">
            {[
              {
                label: "CPU percent",
                baseline: captureValue(latest.baseline, "cpu_percent", "%"),
                optimized: captureValue(latest.optimized, "cpu_percent", "%"),
                delta: deltaLabel(latest.cpu_delta, "%"),
              },
              {
                label: "GPU percent",
                baseline: captureValue(latest.baseline, "gpu_percent", "%"),
                optimized: captureValue(latest.optimized, "gpu_percent", "%"),
                delta: deltaLabel(latest.gpu_delta, "%"),
              },
              {
                label: "Ping",
                baseline: captureValue(latest.baseline, "ping_ms", " ms"),
                optimized: captureValue(latest.optimized, "ping_ms", " ms"),
                delta: deltaLabel(latest.ping_delta, " ms"),
              },
              {
                label: "Jitter",
                baseline: captureValue(latest.baseline, "jitter_ms", " ms"),
                optimized: captureValue(latest.optimized, "jitter_ms", " ms"),
                delta: deltaLabel(latest.jitter_delta, " ms"),
              },
            ].map((metric) => (
              <Card key={metric.label} className="p-4">
                <div className="mb-2 text-[11px] text-txt-tertiary">{metric.label}</div>
                <div className="space-y-1 text-sm">
                  <div className="flex justify-between">
                    <span className="text-txt-tertiary">Baseline</span>
                    <span className="numeric-tabular text-txt-primary">{metric.baseline}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-txt-tertiary">Optimized</span>
                    <span className="numeric-tabular text-txt-primary">{metric.optimized}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-txt-tertiary">Delta</span>
                    <span className="numeric-tabular text-txt-primary">{metric.delta}</span>
                  </div>
                </div>
              </Card>
            ))}
          </div>

          <Card className="p-5">
            <h3 className="mb-4 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              Frame-time evidence
            </h3>
            <div className="grid gap-4 md:grid-cols-3">
              {[
                {
                  label: "FPS average",
                  baseline: metricValue(latest.baseline_fps_average),
                  optimized: metricValue(latest.optimized_fps_average),
                  delta: deltaLabel(latest.fps_delta_percent, "%"),
                },
                {
                  label: "1% low FPS",
                  baseline: metricValue(latest.baseline_fps_1_low),
                  optimized: metricValue(latest.optimized_fps_1_low),
                  delta: deltaLabel(latest.one_percent_low_delta),
                },
                {
                  label: "P95 frame time",
                  baseline: metricValue(latest.baseline_p95_frame_time_ms, " ms"),
                  optimized: metricValue(latest.optimized_p95_frame_time_ms, " ms"),
                  delta: deltaLabel(latest.p95_frame_time_delta_percent, "%"),
                },
              ].map((metric) => (
                <div key={metric.label} className="rounded-md bg-surface-sunken p-4">
                  <div className="mb-2 text-[11px] text-txt-tertiary">{metric.label}</div>
                  <div className="space-y-1 text-sm">
                    <div className="flex justify-between">
                      <span className="text-txt-tertiary">Baseline</span>
                      <span className="numeric-tabular text-txt-primary">{metric.baseline}</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-txt-tertiary">Optimized</span>
                      <span className="numeric-tabular text-txt-primary">{metric.optimized}</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-txt-tertiary">Delta</span>
                      <span className="numeric-tabular text-txt-primary">{metric.delta}</span>
                    </div>
                  </div>
                </div>
              ))}
            </div>
            {!latest.frametime_supported && latest.frame_time_reason ? (
              <div className="mt-4 rounded-md border border-border-default bg-surface-sunken px-4 py-3 text-sm text-txt-tertiary">
                Frame-time evidence unavailable: {latest.frame_time_reason}
              </div>
            ) : null}
          </Card>

          <Card className="p-5">
            <h3 className="mb-4 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              Tweak attribution
            </h3>
            <div className="space-y-3">
              {(latest.tweak_set || []).length ? latest.tweak_set.map((tweakId) => {
                const tweak = tweakChoices.find((item) => item.id === tweakId);
                return (
                  <div
                    key={tweakId}
                    className="flex items-center justify-between border-b border-border-subtle py-2 last:border-0"
                  >
                    <span className="text-sm text-txt-secondary">
                      {tweak?.name || tweakId}
                    </span>
                    <span className="numeric-tabular text-sm font-medium text-success">
                      Included in proof set
                    </span>
                  </div>
                );
              }) : (
                <div className="text-sm text-txt-tertiary">
                  No explicit tweak attribution was captured for this run.
                </div>
              )}
            </div>
          </Card>
        </div>
      ) : null}

      {activeTab === "history" ? (
        <div className="space-y-5">
          <Card className="overflow-hidden">
            <table className="w-full">
              <thead>
                <tr className="bg-surface-sunken">
                  {["Run", "Date", "Verdict", "Tweaks", "Duration"].map((label) => (
                    <th key={label} className="px-4 py-2.5 text-left text-[11px] font-medium uppercase tracking-[0.14em] text-txt-tertiary">
                      {label}
                    </th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {safeBenchmarkResults.map((result) => (
                  <tr
                    key={result.benchmark_id}
                    className="cursor-pointer border-t border-border-subtle transition-colors hover:bg-surface-hover"
                    onClick={() => setSelectedBenchmarkId?.(result.benchmark_id)}
                  >
                    <td className="px-4 py-3 text-sm font-medium text-txt-primary">
                      #{result.benchmark_id?.slice(0, 6)}
                    </td>
                    <td className="numeric-tabular px-4 py-3 text-sm text-txt-secondary">
                      {formatDateTime(result.created_at)}
                    </td>
                    <td className="px-4 py-3">
                      <StatusBadge status={result.verdict || "NO_MEASURABLE_IMPACT"} />
                    </td>
                    <td className="numeric-tabular px-4 py-3 text-sm text-txt-tertiary">
                      {result.tweak_set?.length || 0}
                    </td>
                    <td className="numeric-tabular px-4 py-3 text-sm text-txt-tertiary">
                      {result.duration_seconds}s
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </Card>

          <Card className="p-5">
            <h3 className="mb-4 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              Verdict sequence
            </h3>
            <div className="space-y-2">
              {verdictHistory.length ? verdictHistory.map((item) => (
                <div key={item.benchmark_id} className="flex items-center justify-between rounded-md bg-surface-sunken p-3">
                  <span className="text-sm text-txt-secondary">{formatDateTime(item.created_at)}</span>
                  <StatusBadge status={item.verdict || "MONITORED"} />
                </div>
              )) : (
                <div className="text-sm text-txt-tertiary">No benchmark runs have been recorded yet.</div>
              )}
            </div>
          </Card>
        </div>
      ) : null}

      {activeTab === "compare" ? (
        <div className="space-y-5">
          <div className="grid gap-4 md:grid-cols-2">
            <div>
              <label className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">
                Workload
              </label>
              <input
                className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-1.5 text-sm text-txt-primary focus:border-accent focus:outline-none"
                value={benchmarkWorkload}
                onChange={(event) => setBenchmarkWorkload?.(event.target.value)}
              />
            </div>
            <div>
              <label className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">
                Duration
              </label>
              <select
                className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-1.5 text-sm text-txt-primary focus:border-accent focus:outline-none"
                value={benchmarkDuration}
                onChange={(event) => setBenchmarkDuration?.(Number(event.target.value))}
              >
                {[4, 6, 8, 10, 12].map((duration) => (
                  <option key={duration} value={duration}>
                    {duration} seconds
                  </option>
                ))}
              </select>
            </div>
          </div>

          <Card className="p-5">
            <h3 className="mb-4 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              Proof set
            </h3>
            <div className="grid gap-2 md:grid-cols-2">
              {tweakChoices.map((tweak) => {
                const selected = selectedBenchmarkTweaks?.includes(tweak.id);
                return (
                  <button
                    key={tweak.id}
                    className={`flex items-center justify-between rounded-md border px-3 py-2 text-left text-sm transition-colors ${
                      selected
                        ? "border-accent bg-accent-muted text-txt-primary"
                        : "border-border-default bg-surface-sunken text-txt-secondary hover:bg-surface-hover"
                    }`}
                    onClick={() => toggleBenchmarkTweak?.(tweak.id)}
                    type="button"
                  >
                    <span>{tweak.name}</span>
                    {selected ? <StatusBadge status="ACTIVE" /> : null}
                  </button>
                );
              })}
            </div>
            <label className="mt-4 block">
              <span className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">
                Notes
              </span>
              <textarea
                rows="4"
                className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-2 text-sm text-txt-primary focus:border-accent focus:outline-none"
                value={benchmarkNotes}
                onChange={(event) => setBenchmarkNotes?.(event.target.value)}
              />
            </label>
          </Card>
        </div>
      ) : null}
    </div>
  );
}
