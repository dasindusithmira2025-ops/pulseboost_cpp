import { useMemo, useState } from "react";
import {
  AlertTriangleIcon,
  CheckCircleIcon,
  WifiIcon,
} from "lucide-react";

import Card from "../components/Card";
import StatusBadge from "../components/StatusBadge";
function metricValue(value, suffix = "") {
  return value === null || value === undefined ? "Unavailable" : `${value}${suffix}`;
}

function probeRows(diagnostics) {
  const targets = diagnostics?.targets || {};
  return [
    { key: "router", label: "Router", probe: targets.router },
    { key: "public", label: "Public target", probe: targets.public },
    { key: "game_server", label: "Game server", probe: targets.game_server },
  ];
}

function recommendationRows(diagnostics) {
  const protocol = diagnostics?.protocol_profile?.detected_transport || "unknown";
  const qosSupported = Boolean(diagnostics?.qos?.supported);
  const publicProbe = diagnostics?.targets?.public;
  const rows = [];

  if (protocol === "udp") {
    rows.push({
      title: "Latency-sensitive session detected",
      detail: "PulseBoost will prefer UDP-oriented diagnostics and QoS posture for active game traffic.",
      status: "VALIDATED",
    });
  }

  if (publicProbe?.supported && Number(publicProbe.jitter_ms || 0) > 5) {
      rows.push({
        title: "Jitter spike observed",
        detail: "Investigate background traffic before applying QoS changes. The current path is noisier than ideal.",
        status: "WARNING",
      });
  }

  if (!qosSupported) {
    rows.push({
      title: "QoS writes unavailable",
      detail: diagnostics?.qos?.note || "This runtime cannot apply QoS policies safely.",
      status: "UNSUPPORTED",
    });
  }

  if (!rows.length) {
    rows.push({
      title: "No immediate network action queued",
      detail: "Current diagnostics do not show a backend-supported change worth applying right now.",
      status: "MONITORED",
    });
  }

  return rows;
}

export default function NetworkPage({
  featureAccess,
  networkDiagnostics,
  refreshNetworkDiagnostics,
  requestApplyNetworkQos,
}) {
  const [activeTab, setActiveTab] = useState("overview");
  const diagnostics = networkDiagnostics || {};
  const adapter = diagnostics.nic_capabilities?.[0] || null;
  const summary = diagnostics.summary || {};
  const qosSupported = Boolean(diagnostics.qos?.supported);
  const advancedControlsEnabled = Boolean(featureAccess?.advanced_network_controls);
  const targetRows = useMemo(() => probeRows(diagnostics), [diagnostics]);
  const recommendations = useMemo(() => recommendationRows(diagnostics), [diagnostics]);
  const adapterAddresses = adapter?.addresses || [];

  return (
    <div className="space-y-5">
      <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
        Network
      </h1>

      <div className="flex items-center gap-0 border-b border-border-subtle">
        {[
          { id: "overview", label: "Overview" },
          { id: "diagnostics", label: "Diagnostics" },
          { id: "qos", label: "QoS" },
          { id: "adapters", label: "Adapters" },
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

      {activeTab === "overview" ? (
        <div className="space-y-5">
          <Card className="p-5">
            <div className="mb-4 flex items-center gap-3">
              <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-success-muted">
                <WifiIcon className="h-5 w-5 text-success" />
              </div>
              <div>
                <h3 className="text-sm font-semibold text-txt-primary">
                  {adapter?.name || "Primary adapter"}
                </h3>
                <p className="text-xs text-txt-secondary">
                  {adapter?.is_up ? "Connected" : "Adapter status unavailable"}{adapter?.speed_mbps ? ` · ${adapter.speed_mbps} Mbps` : ""}
                </p>
              </div>
            </div>
            <div className="grid gap-4 md:grid-cols-4">
              {[
                { label: "Latency", value: metricValue(summary.latency_ms, " ms"), color: "text-success" },
                { label: "Jitter", value: metricValue(summary.jitter_ms, " ms"), color: "text-txt-primary" },
                { label: "Packet loss", value: metricValue(summary.packet_loss_percent, "%"), color: "text-txt-primary" },
                { label: "Bufferbloat", value: summary.bufferbloat_grade || "Unavailable", color: "text-txt-primary" },
              ].map((metric) => (
                <div key={metric.label} className="text-center">
                  <div className="mb-1 text-[11px] text-txt-tertiary">{metric.label}</div>
                  <div className={`numeric-tabular text-lg font-semibold ${metric.color}`}>
                    {metric.value}
                  </div>
                </div>
              ))}
            </div>
          </Card>

          <div className="grid gap-5 lg:grid-cols-2">
            <Card className="p-5">
              <h3 className="mb-4 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
                Runtime posture
              </h3>
              <div className="space-y-3 text-sm">
                <div className="flex items-center justify-between">
                  <span className="text-txt-secondary">Detected transport</span>
                  <span className="font-medium text-txt-primary">
                    {diagnostics.protocol_profile?.detected_transport || "Unknown"}
                  </span>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-txt-secondary">Confidence</span>
                  <span className="numeric-tabular font-medium text-txt-primary">
                    {diagnostics.protocol_profile?.confidence !== undefined
                      ? `${Math.round(Number(diagnostics.protocol_profile.confidence) * 100)}%`
                      : "Unavailable"}
                  </span>
                </div>
                <div className="rounded-md bg-surface-sunken p-3 text-xs text-txt-tertiary">
                  {diagnostics.protocol_profile?.reasoning || "Protocol reasoning is unavailable for this sample."}
                </div>
              </div>
            </Card>

            <Card className="p-5">
              <h3 className="mb-4 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
                Adapter activity
              </h3>
              <div className="space-y-3 text-sm">
                <div className="flex items-center justify-between">
                  <span className="text-txt-secondary">MTU</span>
                  <span className="numeric-tabular text-txt-primary">{adapter?.mtu || "Unavailable"}</span>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-txt-secondary">Address count</span>
                  <span className="numeric-tabular text-txt-primary">{adapterAddresses.length}</span>
                </div>
                <div className="rounded-md bg-surface-sunken p-3 text-xs text-txt-tertiary">
                  {adapterAddresses.length
                    ? adapterAddresses.join(" · ")
                    : "Interface addresses are not exposed by the current runtime sample."}
                </div>
              </div>
            </Card>
          </div>

          <Card className="p-5">
            <h3 className="mb-3 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              Recommendations
            </h3>
            <div className="space-y-2">
              {recommendations.map((item) => (
                <div key={item.title} className="flex items-start gap-3 rounded-md bg-surface-sunken p-3">
                  <span className="mt-1.5 h-1.5 w-1.5 shrink-0 rounded-full bg-accent" />
                  <div className="flex-1">
                    <div className="text-sm text-txt-primary">{item.title}</div>
                    <div className="text-xs text-txt-tertiary">{item.detail}</div>
                  </div>
                  <StatusBadge status={item.status} />
                </div>
              ))}
            </div>
          </Card>
        </div>
      ) : null}

      {activeTab === "diagnostics" ? (
        <div className="space-y-5">
          <div className="flex items-center justify-between">
            <p className="text-sm text-txt-secondary">
              Run a fresh backend diagnostic to refresh the current target probes and adapter capabilities.
            </p>
            <button
              className="rounded-md bg-accent px-4 py-1.5 text-sm font-medium text-white transition-colors hover:bg-accent-hover"
              onClick={() => refreshNetworkDiagnostics?.()}
              type="button"
            >
              Run Diagnostic
            </button>
          </div>
          <Card className="p-5">
            <h3 className="mb-4 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
              Probe results
            </h3>
            <div className="space-y-3">
              {targetRows.map((row) => (
                <div key={row.key} className="flex items-center justify-between border-b border-border-subtle py-2 last:border-0">
                  <div>
                    <div className="text-sm text-txt-primary">{row.label}</div>
                    <div className="text-xs text-txt-tertiary">
                      {row.probe?.supported
                        ? `${metricValue(row.probe?.latency_ms, " ms")} latency · ${metricValue(row.probe?.jitter_ms, " ms")} jitter`
                        : row.probe?.reason || "Probe unavailable"}
                    </div>
                  </div>
                  {row.probe?.supported ? (
                    <CheckCircleIcon className="h-4 w-4 text-success" />
                  ) : (
                    <AlertTriangleIcon className="h-4 w-4 text-warning" />
                  )}
                </div>
              ))}
            </div>
          </Card>
        </div>
      ) : null}

      {activeTab === "qos" ? (
        <div className="space-y-5">
          <Card className="p-5">
            <div className="mb-3 flex items-center gap-3">
              <h3 className="text-[15px] font-semibold text-txt-primary">
                QoS Packet Scheduling
              </h3>
              <StatusBadge status={qosSupported ? "DRY_RUN" : "UNSUPPORTED"} />
            </div>
            <p className="mb-4 text-sm text-txt-secondary">
              {diagnostics.qos?.note || "No QoS capability note was returned by the backend."}
            </p>
            <div className="mt-4 flex gap-2">
              <button
                className="rounded-md border border-accent px-3 py-1.5 text-xs text-accent transition-colors hover:bg-accent-muted disabled:cursor-not-allowed disabled:opacity-60"
                onClick={() => requestApplyNetworkQos?.("Low Latency", "udp")}
                disabled={!advancedControlsEnabled}
                type="button"
              >
                Apply Low Latency
              </button>
              <button
                className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover disabled:cursor-not-allowed disabled:opacity-60"
                onClick={() => requestApplyNetworkQos?.("Balanced", "tcp")}
                disabled={!advancedControlsEnabled}
                type="button"
              >
                Apply Balanced
              </button>
            </div>
          </Card>
        </div>
      ) : null}

      {activeTab === "adapters" ? (
        <div className="space-y-3">
          {(diagnostics.nic_capabilities || []).map((nic) => (
            <Card key={nic.name} className="p-5">
              <div className="mb-3 flex items-center justify-between">
                <div className="flex flex-wrap items-center gap-3">
                  <h3 className="text-sm font-semibold text-txt-primary">
                    {nic.name}
                  </h3>
                  <span className={`text-xs font-medium ${nic.is_up ? "text-success" : "text-txt-disabled"}`}>
                    {nic.is_up ? "Connected" : "Disconnected"}
                  </span>
                </div>
                {nic.speed_mbps ? (
                  <span className="numeric-tabular text-xs text-txt-tertiary">
                    {nic.speed_mbps} Mbps
                  </span>
                ) : null}
              </div>
              <div className="mb-3 grid gap-2 text-xs md:grid-cols-2">
                <div>
                  <span className="text-txt-tertiary">MTU: </span>
                  <span className="numeric-tabular text-txt-secondary">
                    {nic.mtu || "Unknown"}
                  </span>
                </div>
                <div>
                  <span className="text-txt-tertiary">Addresses: </span>
                  <span className="text-txt-secondary">
                    {(nic.addresses || []).length || 0}
                  </span>
                </div>
              </div>
              <div className="flex flex-wrap items-center gap-3">
                <span className="text-[11px] text-txt-tertiary">
                  Capabilities:
                </span>
                {[
                  { label: "QoS", supported: nic.qos_supported },
                  { label: "Advanced tuning", supported: nic.advanced_tuning_supported },
                ].map((capability) => (
                  <span
                    key={capability.label}
                    className={`rounded px-2 py-0.5 text-[10px] font-medium ${
                      capability.supported
                        ? "bg-success-muted text-success"
                        : "bg-surface-hover text-txt-disabled"
                    }`}
                  >
                    {capability.label}: {capability.supported ? "Yes" : "No"}
                  </span>
                ))}
              </div>
            </Card>
          ))}
          {!diagnostics.nic_capabilities?.length ? (
            <Card className="p-5 text-sm text-txt-tertiary">
              No adapter capability rows are available from the backend yet.
            </Card>
          ) : null}
        </div>
      ) : null}
    </div>
  );
}
