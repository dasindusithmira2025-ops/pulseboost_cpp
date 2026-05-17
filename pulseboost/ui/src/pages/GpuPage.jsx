import { useMemo, useState } from "react";
import { ExternalLinkIcon, InfoIcon } from "lucide-react";

import Card from "../components/Card";
import StatusBadge from "../components/StatusBadge";

function metricValue(value, suffix = "") {
  return value === null || value === undefined ? "Unavailable" : `${value}${suffix}`;
}

function driverUrlForVendor(vendor) {
  if (vendor === "nvidia") return "https://www.nvidia.com/Download/index.aspx";
  if (vendor === "amd") return "https://www.amd.com/en/support";
  return null;
}

export default function GpuPage({
  biosChecklist,
  featureAccess,
  gpuStats,
  requestApplyGpuSetting,
}) {
  const [activeTab, setActiveTab] = useState("telemetry");
  const telemetrySupported = Boolean(gpuStats?.telemetry_supported);
  const settingsRows = gpuStats?.settings || [];
  const advancedControlsEnabled = Boolean(featureAccess?.advanced_gpu_controls);
  const driverUrl = useMemo(() => driverUrlForVendor(gpuStats?.vendor), [gpuStats?.vendor]);

  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          GPU
        </h1>
        <div className="flex items-center gap-2 rounded-md border border-success/20 bg-success-muted px-3 py-1.5">
          <span className="text-xs font-semibold text-success">
            {gpuStats?.model || "GPU surface"}
          </span>
        </div>
      </div>

      <div className="flex items-center gap-0 border-b border-border-subtle">
        {[
          { id: "telemetry", label: "Telemetry" },
          { id: "optimizations", label: "Optimizations" },
          { id: "driver", label: "Driver" },
          { id: "advisory", label: "Advisory" },
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

      {activeTab === "telemetry" ? (
        <div className="space-y-5">
          <div className="grid gap-4 md:grid-cols-2 xl:grid-cols-4">
            {[
              { label: "Core clock", value: metricValue(gpuStats?.clock_mhz, " MHz") },
              { label: "Memory used", value: metricValue(gpuStats?.memory_used_mb, " MB") },
              { label: "Temperature", value: metricValue(gpuStats?.temperature_c, " C") },
              { label: "Power draw", value: metricValue(gpuStats?.power_watts, " W") },
            ].map((metric) => (
              <Card key={metric.label} className="p-4">
                <div className="mb-1 text-[11px] text-txt-tertiary">{metric.label}</div>
                <div className="numeric-tabular text-lg font-semibold text-txt-primary">
                  {metric.value}
                </div>
              </Card>
            ))}
          </div>

          <Card className="p-5">
            <div className="mb-3 flex items-center justify-between">
              <h3 className="text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
                Telemetry support
              </h3>
              <StatusBadge status={telemetrySupported ? "LIVE" : "UNSUPPORTED"} />
            </div>
            <p className="text-sm text-txt-secondary">
              {gpuStats?.reason || "Live telemetry is available through the current vendor runtime."}
            </p>
          </Card>
        </div>
      ) : null}

      {activeTab === "optimizations" ? (
        <div className="space-y-3">
          {settingsRows.length ? settingsRows.map((setting) => (
            <Card key={setting.id} className="p-4">
              <div className="flex items-center justify-between gap-3">
                <div>
                  <div className="mb-1 flex flex-wrap items-center gap-2">
                    <h4 className="text-sm font-semibold text-txt-primary">
                      {setting.label}
                    </h4>
                    <StatusBadge status={setting.supported ? "VALIDATED" : "UNSUPPORTED"} />
                    {!advancedControlsEnabled ? <StatusBadge status="LOCKED" /> : null}
                  </div>
                  <p className="text-xs text-txt-secondary">
                    {setting.reason || "Vendor-aware GPU surface"}
                  </p>
                </div>
                <button
                  className="rounded-md border border-border-default px-3 py-1 text-xs text-txt-primary transition-colors hover:bg-surface-hover disabled:cursor-not-allowed disabled:opacity-60"
                  onClick={() => requestApplyGpuSetting?.(setting)}
                  disabled={!advancedControlsEnabled}
                  type="button"
                >
                  {setting.supported ? "Apply" : "Review"}
                </button>
              </div>
            </Card>
          )) : (
            <Card className="p-4 text-sm text-txt-tertiary">
              No GPU optimization surfaces were returned for this runtime.
            </Card>
          )}
        </div>
      ) : null}

      {activeTab === "driver" ? (
        <div className="space-y-5">
          <Card className="p-5">
            <h3 className="mb-4 text-[15px] font-semibold text-txt-primary">
              Current driver surface
            </h3>
            <div className="space-y-2 text-sm">
              <div className="flex justify-between">
                <span className="text-txt-tertiary">Version</span>
                <span className="numeric-tabular text-txt-primary">{gpuStats?.driver_version || "Unavailable"}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-txt-tertiary">Vendor</span>
                <span className="text-txt-primary">{gpuStats?.vendor || "Unavailable"}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-txt-tertiary">Status</span>
                <span className="text-success font-medium">
                  {telemetrySupported ? "Telemetry supported" : "Advisory only"}
                </span>
              </div>
            </div>
          </Card>
          <Card className="border-border-subtle p-4">
            <div className="flex items-start gap-3">
              <InfoIcon className="mt-0.5 h-4 w-4 shrink-0 text-txt-tertiary" />
              <div>
                <p className="text-sm text-txt-secondary">
                  PulseBoost does not install or modify GPU drivers. Use the vendor site for driver downloads and release notes.
                </p>
                {driverUrl ? (
                  <a href={driverUrl} target="_blank" rel="noreferrer" className="mt-1 inline-flex items-center gap-1 text-xs text-accent hover:text-accent-hover">
                    Open vendor downloads <ExternalLinkIcon className="h-3 w-3" />
                  </a>
                ) : (
                  <div className="mt-1 text-xs text-txt-tertiary">
                    No vendor driver URL is available for this hardware profile.
                  </div>
                )}
              </div>
            </div>
          </Card>
        </div>
      ) : null}

      {activeTab === "advisory" ? (
        <div className="space-y-5">
          <Card className="border-info/20 bg-info-muted/50 p-4">
            <div className="flex items-start gap-3">
              <InfoIcon className="mt-0.5 h-4 w-4 shrink-0 text-info" />
              <p className="text-sm text-info/80">
                These settings require BIOS changes and cannot be modified by PulseBoost. They are shown for informational purposes only.
              </p>
            </div>
          </Card>
          {(biosChecklist?.items || []).length ? (biosChecklist.items || []).map((item) => (
            <Card key={item.id} className="p-4">
              <div className="flex items-center justify-between gap-3">
                <div>
                  <div className="mb-1 flex items-center gap-2">
                    <h4 className="text-sm font-semibold text-txt-primary">
                      {item.title}
                    </h4>
                    <span className="rounded bg-surface-hover px-2 py-0.5 text-[10px] font-medium uppercase tracking-[0.14em] text-txt-tertiary">
                      BIOS
                    </span>
                  </div>
                  <p className="text-xs text-txt-tertiary">{item.reason}</p>
                </div>
                <StatusBadge status={item.recommended ? "VALIDATED" : item.status || "MONITORED"} />
              </div>
            </Card>
          )) : (
            <Card className="p-4 text-sm text-txt-tertiary">
              No BIOS advisory items are available for this machine yet.
            </Card>
          )}
        </div>
      ) : null}
    </div>
  );
}
