import { useEffect, useState } from "react";
import { AlertTriangleIcon, ShieldCheckIcon } from "lucide-react";

import Card from "../components/Card";

function formatTime(unixSeconds) {
  if (!unixSeconds) return "Never";
  return new Date(Number(unixSeconds) * 1000).toLocaleString();
}

export default function TrustPage({
  trustCenter,
  handleExpertModeToggle,
  requestPermissionUndo,
  requestRollbackAll,
}) {
  const rollback = trustCenter?.rollback_readiness || {};
  const matrix = trustCenter?.touch_matrix || [];
  const permissionAudit = trustCenter?.permission_audit || [];
  const [localExpertMode, setLocalExpertMode] = useState(Boolean(trustCenter?.expert_mode_state));

  useEffect(() => {
    setLocalExpertMode(Boolean(trustCenter?.expert_mode_state));
  }, [trustCenter?.expert_mode_state]);

  const expertMode = localExpertMode || Boolean(trustCenter?.expert_mode_state);

  return (
    <div className="space-y-5">
      <div>
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">Trust Center</h1>
        <p className="mt-1 text-sm text-txt-secondary">Radical transparency and full rollback visibility.</p>
      </div>

      <Card className="p-5">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-success-muted">
              <ShieldCheckIcon className="h-6 w-6 text-success" />
            </div>
            <div>
              <h2 className="text-lg font-semibold text-txt-primary">
                Recovery: {rollback.pending_revert_snapshots > 0 ? "Ready" : "Monitoring"}
              </h2>
              <p className="text-sm text-txt-secondary">
                Active temporary tweaks: {rollback.active_temporary_tweaks ?? 0}
              </p>
            </div>
          </div>
          <button
            type="button"
            onClick={() => {
              if (window.confirm("Revert every active temporary tweak tracked by Trust Center?")) {
                requestRollbackAll?.();
              }
            }}
            className="rounded-md border border-red-500/35 px-3 py-1.5 text-xs uppercase tracking-[0.12em] text-red-400 hover:bg-red-500/10"
          >
            Revert All Temporary Changes
          </button>
        </div>
      </Card>

      <Card className="p-5">
        <h3 className="mb-3 text-[15px] font-semibold text-txt-primary">What PulseBoost can and cannot touch</h3>
        <div className="overflow-hidden rounded-[12px] border border-border-default">
          <table className="w-full">
            <thead>
              <tr className="bg-surface-sunken">
                <th className="px-3 py-2 text-left text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">Category</th>
                <th className="px-3 py-2 text-left text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">Access</th>
                <th className="px-3 py-2 text-left text-[11px] uppercase tracking-[0.14em] text-txt-tertiary">Notes</th>
              </tr>
            </thead>
            <tbody>
              {matrix.map((row) => (
                <tr key={row.category} className="border-t border-border-subtle">
                  <td className="px-3 py-2 text-sm text-txt-primary">{row.category}</td>
                  <td className="px-3 py-2 text-sm">
                    <span className={row.can_touch ? "text-success" : "text-txt-tertiary"}>{row.can_touch ? "YES" : "NO"}</span>
                  </td>
                  <td className="px-3 py-2 text-sm text-txt-secondary">{row.note}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Card>

      <Card className="p-5">
        <h3 className="mb-3 text-[15px] font-semibold text-txt-primary">Permission Audit</h3>
        <div className="space-y-2">
          {permissionAudit.map((row) => (
            <div key={row.category_key} className="rounded-[12px] border border-border-default bg-surface-sunken px-3 py-2">
              <div className="flex items-center justify-between gap-3">
                <div>
                  <p className="text-sm text-txt-primary">{row.category_label}</p>
                  <p className="text-[11px] text-txt-tertiary">
                    Last action: {row.last_action ? `${row.last_action.action} - ${formatTime(row.last_action.timestamp)}` : "Never"}
                  </p>
                  <p className="text-[11px] text-txt-tertiary">
                    Last reverted: {row.last_reverted ? `${row.last_reverted.action} - ${formatTime(row.last_reverted.timestamp)}` : "Never"}
                  </p>
                </div>
                <button
                  type="button"
                  disabled={!row.undo_supported}
                  onClick={() => requestPermissionUndo?.(row.category_key)}
                  className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary hover:bg-surface-hover disabled:cursor-not-allowed disabled:opacity-60"
                >
                  Undo
                </button>
              </div>
            </div>
          ))}
          {!permissionAudit.length ? (
            <p className="text-xs text-txt-tertiary">No permission audit rows available yet.</p>
          ) : null}
        </div>
      </Card>

      <Card className="p-5">
        <div className="mb-3 flex items-center justify-between">
          <h3 className="text-[15px] font-semibold text-txt-primary">Expert Mode</h3>
          <button
            onClick={() => {
              const next = !expertMode;
              setLocalExpertMode(next);
              handleExpertModeToggle?.(next);
            }}
            className={`flex h-5 w-9 items-center rounded-full px-0.5 transition-colors ${expertMode ? "justify-end bg-warning" : "justify-start bg-surface-active"}`}
            type="button"
          >
            <span className="h-4 w-4 rounded-full bg-white" />
          </button>
        </div>
        {expertMode ? (
          <div className="flex items-start gap-2 rounded-md bg-warning-muted p-3">
            <AlertTriangleIcon className="mt-0.5 h-4 w-4 shrink-0 text-warning" />
            <p className="text-sm text-warning">Expert mode is active. Advanced controls may increase system risk.</p>
          </div>
        ) : (
          <p className="text-sm text-txt-secondary">Expert mode is disabled. Only validated safe controls are exposed.</p>
        )}
      </Card>
    </div>
  );
}
