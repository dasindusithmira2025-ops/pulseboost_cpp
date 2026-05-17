import { useMemo, useState } from "react";
import {
  DownloadIcon,
  RotateCcwIcon,
  SearchIcon,
} from "lucide-react";

import Card from "../components/Card";
import StatusBadge from "../components/StatusBadge";
import { formatDateTime } from "../utils/formatters";

function eventType(entry) {
  const status = String(entry.status || "").toLowerCase();
  const action = String(entry.action || "").toLowerCase();
  if (status === "failed") return "failed";
  if (action.includes("revert")) return "reverted";
  if (action.includes("benchmark")) return "benchmark";
  if (String(entry.triggered_by || "").toLowerCase().includes("adaptive")) return "ai";
  return "applied";
}

function dotColor(type) {
  switch (type) {
    case "applied":
      return "bg-accent";
    case "reverted":
      return "bg-surface-active";
    case "failed":
      return "bg-error";
    case "benchmark":
      return "border-2 border-info bg-transparent";
    case "ai":
      return "border border-accent/50 bg-accent";
    default:
      return "bg-surface-active";
  }
}

function groupLabel(timestamp) {
  if (!timestamp) return "Unknown";
  return new Date(Number(timestamp) * 1000).toLocaleDateString();
}

function safeJson(value) {
  if (value === null || value === undefined) return "Unavailable";
  return JSON.stringify(value, null, 2);
}

export default function AuditPage({
  auditEntries,
  featureAccess,
  handleExportAudit,
  requestRevertAudit,
}) {
  const [viewMode, setViewMode] = useState("timeline");
  const [search, setSearch] = useState("");
  const [expandedEntryId, setExpandedEntryId] = useState(null);

  const events = useMemo(() => {
    return (auditEntries || [])
      .map((entry) => {
        const afterValue = entry.after_value || {};
        return {
          ...entry,
          id: entry.id || `${entry.timestamp}-${entry.action}-${entry.target}`,
          time: formatDateTime(entry.timestamp),
          title: entry.action ? `${entry.action} · ${entry.target || entry.module || "system"}` : (entry.target || "Audit event"),
          badges: [String(entry.validity_tag || "VALIDATED").toUpperCase()],
          source: entry.triggered_by || "Manual",
          type: eventType(entry),
          group: groupLabel(entry.timestamp),
          revertible: Boolean(afterValue.revert_snapshot_id),
        };
      })
      .filter((entry) =>
        !search ||
        entry.title.toLowerCase().includes(search.toLowerCase()) ||
        entry.source.toLowerCase().includes(search.toLowerCase()),
      );
  }, [auditEntries, search]);

  const groups = [...new Set(events.map((event) => event.group))];

  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Audit Log
        </h1>
        <button
          className="flex items-center gap-1.5 rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover"
          onClick={() => handleExportAudit?.()}
          disabled={!featureAccess?.audit_export}
          type="button"
        >
          <DownloadIcon className="h-3.5 w-3.5" /> Export
        </button>
      </div>

      <div className="flex flex-wrap items-center justify-between gap-4">
        <div className="flex items-center gap-0 border-b border-border-subtle">
          {["timeline", "list"].map((mode) => (
            <button
              key={mode}
              onClick={() => setViewMode(mode)}
              className={`relative px-4 py-2 text-sm capitalize transition-colors ${
                viewMode === mode
                  ? "text-txt-primary"
                  : "text-txt-tertiary hover:text-txt-secondary"
              }`}
              type="button"
            >
              {mode}
              {viewMode === mode ? (
                <span className="absolute bottom-0 left-0 right-0 h-0.5 rounded-t bg-accent" />
              ) : null}
            </button>
          ))}
        </div>
        <div className="relative">
          <SearchIcon className="absolute left-2.5 top-1/2 h-3.5 w-3.5 -translate-y-1/2 text-txt-tertiary" />
          <input
            type="text"
            value={search}
            onChange={(event) => setSearch(event.target.value)}
            placeholder="Search events..."
            className="w-56 rounded-md border border-border-default bg-surface-sunken py-1.5 pl-8 pr-3 text-sm text-txt-primary placeholder:text-txt-tertiary focus:border-accent focus:outline-none"
          />
        </div>
      </div>

      {viewMode === "timeline" ? (
        <div className="space-y-6">
          {groups.map((group) => (
            <div key={group}>
              <div className="mb-3 text-xs font-semibold uppercase tracking-[0.18em] text-txt-tertiary">
                {group}
              </div>
              <div className="relative pl-6">
                <div className="absolute bottom-2 left-[7px] top-2 w-px bg-border-default" />
                <div className="space-y-4">
                  {events
                    .filter((event) => event.group === group)
                    .map((event) => {
                      const expanded = expandedEntryId === event.id;
                      return (
                        <div key={event.id} className="relative">
                          <div
                            className={`absolute -left-6 top-1.5 h-3.5 w-3.5 rounded-full ${dotColor(event.type)}`}
                          />
                          <Card className="ml-2 p-4">
                            <div className="flex items-start justify-between gap-3">
                              <div className="min-w-0 flex-1">
                                <div className="mb-1 flex items-center gap-2">
                                  <span className="numeric-tabular text-[11px] text-txt-tertiary">
                                    {event.time}
                                  </span>
                                  <h4 className="truncate text-sm font-medium text-txt-primary">
                                    {event.title}
                                  </h4>
                                </div>
                                <div className="mb-1 flex flex-wrap items-center gap-2">
                                  {event.badges.map((badge) => (
                                    <StatusBadge key={badge} status={badge} />
                                  ))}
                                  <span className="text-[11px] text-txt-tertiary">
                                    · {event.source}
                                  </span>
                                </div>
                                <div className="text-xs text-txt-tertiary">
                                  {event.rationale || "No rationale recorded."}
                                </div>
                              </div>
                              <div className="flex shrink-0 items-center gap-2">
                                <button
                                  className="text-[11px] text-txt-tertiary transition-colors hover:text-txt-secondary"
                                  onClick={() => setExpandedEntryId(expanded ? null : event.id)}
                                  type="button"
                                >
                                  {expanded ? "Hide details" : "Details"}
                                </button>
                                <button
                                  className="flex items-center gap-1 px-2 py-1 text-[11px] text-txt-tertiary transition-colors hover:text-txt-secondary disabled:cursor-not-allowed disabled:opacity-50"
                                  disabled={!event.revertible}
                                  onClick={() => requestRevertAudit?.(event)}
                                  type="button"
                                >
                                  <RotateCcwIcon className="h-3 w-3" /> Revert
                                </button>
                              </div>
                            </div>
                            {expanded ? (
                              <div className="mt-4 grid gap-3 border-t border-border-subtle pt-4 lg:grid-cols-2">
                                <div>
                                  <div className="mb-1 text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">
                                    Before
                                  </div>
                                  <pre className="overflow-x-auto rounded-md bg-surface-sunken p-3 text-xs text-txt-secondary">
                                    {safeJson(event.before_value)}
                                  </pre>
                                </div>
                                <div>
                                  <div className="mb-1 text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">
                                    After
                                  </div>
                                  <pre className="overflow-x-auto rounded-md bg-surface-sunken p-3 text-xs text-txt-secondary">
                                    {safeJson(event.after_value)}
                                  </pre>
                                </div>
                              </div>
                            ) : null}
                          </Card>
                        </div>
                      );
                    })}
                </div>
              </div>
            </div>
          ))}
        </div>
      ) : (
        <Card className="overflow-hidden">
          <table className="w-full">
            <thead>
              <tr className="bg-surface-sunken">
                <th className="px-4 py-2.5 text-left text-[11px] font-medium uppercase tracking-[0.14em] text-txt-tertiary">
                  Time
                </th>
                <th className="px-4 py-2.5 text-left text-[11px] font-medium uppercase tracking-[0.14em] text-txt-tertiary">
                  Action
                </th>
                <th className="px-4 py-2.5 text-left text-[11px] font-medium uppercase tracking-[0.14em] text-txt-tertiary">
                  Status
                </th>
                <th className="px-4 py-2.5 text-left text-[11px] font-medium uppercase tracking-[0.14em] text-txt-tertiary">
                  Source
                </th>
                <th className="px-4 py-2.5 text-right text-[11px] font-medium uppercase tracking-[0.14em] text-txt-tertiary">
                  Actions
                </th>
              </tr>
            </thead>
            <tbody>
              {events.map((event) => (
                <tr
                  key={event.id}
                  className="border-t border-border-subtle transition-colors hover:bg-surface-hover"
                >
                  <td className="numeric-tabular whitespace-nowrap px-4 py-3 text-xs text-txt-tertiary">
                    {event.time}
                  </td>
                  <td className="px-4 py-3 text-sm text-txt-primary">
                    {event.title}
                  </td>
                  <td className="px-4 py-3">
                    <div className="flex gap-1">
                      {event.badges.map((badge) => (
                        <StatusBadge key={badge} status={badge} />
                      ))}
                    </div>
                  </td>
                  <td className="px-4 py-3 text-xs text-txt-tertiary">
                    {event.source}
                  </td>
                  <td className="px-4 py-3 text-right">
                    <button
                      className="text-[11px] text-txt-tertiary hover:text-txt-secondary disabled:cursor-not-allowed disabled:opacity-50"
                      disabled={!event.revertible}
                      onClick={() => requestRevertAudit?.(event)}
                      type="button"
                    >
                      Revert
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </Card>
      )}
    </div>
  );
}
