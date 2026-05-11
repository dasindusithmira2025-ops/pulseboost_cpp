import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import LineChart from "../components/ui/LineChart";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { BackupSummary, RecentActionItem, SnapshotRecord } from "../types/system";

type RestoreSnapshotPayload = {
  ok: boolean;
  disabledCount?: number;
  failedCount?: number;
  restorePointCreated?: boolean;
  reason?: string;
};

function relativeTime(timestamp: string | number): string {
  const date = new Date(timestamp);
  if (Number.isNaN(date.getTime())) {
    return "just now";
  }
  const deltaMs = Date.now() - date.getTime();
  const deltaMinutes = Math.max(0, Math.round(deltaMs / 60000));
  if (deltaMinutes < 1) {
    return "just now";
  }
  if (deltaMinutes < 60) {
    return `${deltaMinutes}m ago`;
  }
  const deltaHours = Math.round(deltaMinutes / 60);
  if (deltaHours < 24) {
    return `${deltaHours}h ago`;
  }
  return `${Math.round(deltaHours / 24)}d ago`;
}

export default function HealthHistory() {
  const history = useSystemStore((s) => s.history);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const fetchSnapshot = useSystemStore((s) => s.fetchSnapshot);
  const [range, setRange] = useState<"today" | "week" | "month" | "quarter">("week");
  const [events, setEvents] = useState<RecentActionItem[]>([]);
  const [snapshots, setSnapshots] = useState<SnapshotRecord[]>([]);
  const [isExporting, setIsExporting] = useState(false);
  const [isCapturing, setIsCapturing] = useState(false);
  const [confirmRestoreId, setConfirmRestoreId] = useState<string | null>(null);
  const [restoringSnapshotId, setRestoringSnapshotId] = useState<string | null>(null);

  const healthSeries = useMemo(
    () => history.cpu.map((cpu, i) => Math.max(20, 100 - cpu * 0.32 - (history.ram[i] ?? 0) * 0.22)),
    [history.cpu, history.ram]
  );

  const loadHistoryData = async () => {
    try {
      const summary = await tauriInvoke<BackupSummary>("get_backup_summary");
      setEvents((summary.actions ?? []).slice(0, 8));
      setSnapshots(summary.snapshots ?? []);
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Failed to load health history data", "danger");
    }
  };

  useEffect(() => {
    void loadHistoryData();
  }, [pushNotification]);

  const exportReport = async () => {
    setIsExporting(true);
    try {
      const result = await tauriInvoke<{ ok: boolean; path?: string; reason?: string }>("export_health_report");
      if (!result.ok || !result.path) {
        throw new Error(result.reason ?? "Health report export failed");
      }
      pushNotification(`Health report exported: ${result.path}`, "ok");
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Health report export failed", "danger");
    } finally {
      setIsExporting(false);
    }
  };

  const captureSnapshot = async () => {
    setIsCapturing(true);
    try {
      const path = await tauriInvoke<string>("take_snapshot");
      pushNotification(`Snapshot captured: ${path}`, "ok");
      await loadHistoryData();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Snapshot capture failed", "danger");
    } finally {
      setIsCapturing(false);
    }
  };

  const restoreSnapshot = async (snapshot: SnapshotRecord) => {
    setRestoringSnapshotId(snapshot.id);
    try {
      const result = await tauriInvoke<RestoreSnapshotPayload>("restore_snapshot", { snapshotPath: snapshot.path });
      if (!result.ok) {
        throw new Error(result.reason ?? "Snapshot restore failed");
      }
      pushNotification(
        result.disabledCount && result.disabledCount > 0
          ? `Snapshot restored - disabled ${result.disabledCount} startup entries`
          : "Snapshot restore complete",
        "ok"
      );
      setConfirmRestoreId(null);
      await loadHistoryData();
      await fetchSnapshot();
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Snapshot restore failed", "danger");
    } finally {
      setRestoringSnapshotId(null);
    }
  };

  return (
    <div className="screen-enter" style={{ display: "grid", gap: "var(--sp-4)" }}>
      <SectionHeader
        title="Backup"
        subtitle="Capture known-good states, restore safely, and keep recovery close to your optimization workflow."
        eyebrow="Recovery and snapshots"
        action={
          <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
            {["today", "week", "month", "quarter"].map((id) => (
              <GlowButton
                key={id}
                label={id === "today" ? "Today" : id === "week" ? "This Week" : id === "month" ? "This Month" : "3 Months"}
                variant={range === id ? "primary" : "ghost"}
                onClick={() => setRange(id as "today" | "week" | "month" | "quarter")}
              />
            ))}
            <GlowButton label="Take Snapshot" variant="ghost" onClick={() => void captureSnapshot()} loading={isCapturing} />
            <GlowButton label="Export Report" variant="ghost" onClick={() => void exportReport()} loading={isExporting} />
          </div>
        }
      />

      <GlassCard glow="accent">
        <SectionHeader title="Health Trend" subtitle={`Range: ${range}`} />
        <LineChart data={healthSeries} labels={history.labels} color="var(--accent2)" />
      </GlassCard>

      <div style={{ display: "grid", gridTemplateColumns: "1.2fr 0.8fr", gap: "var(--sp-4)" }}>
        <GlassCard>
          <SectionHeader title="Recent Events" subtitle="Persisted optimization and recovery history" />
          <div style={{ display: "grid", gap: "var(--sp-2)" }}>
            {events.map((event, index) => (
              <div
                key={`${event.timestamp}-${event.action}`}
                className="surface-panel screen-enter"
                style={{
                  display: "flex",
                  justifyContent: "space-between",
                  gap: "var(--sp-3)",
                  alignItems: "center",
                  padding: "var(--sp-3)",
                  borderRadius: "var(--r-lg)",
                  animationDelay: `${Math.min(index, 8) * 35}ms`
                }}
              >
                <div style={{ minWidth: 0 }}>
                  <div className="font-display nav-item-label" style={{ fontSize: 14, fontWeight: 700 }}>
                    {event.details || event.action}
                  </div>
                  <div className="tiny">{event.action}</div>
                </div>
                <div style={{ display: "grid", justifyItems: "end", gap: 4 }}>
                  <StatusPill status={event.success ? "ok" : "danger"} label={event.success ? "ok" : "fail"} />
                  <span className="font-mono tiny">{relativeTime(event.timestamp)}</span>
                </div>
              </div>
            ))}
            {events.length === 0 ? <div className="tiny" style={{ color: "var(--text-secondary)" }}>No persisted events yet.</div> : null}
          </div>
        </GlassCard>

        <GlassCard>
          <SectionHeader title="Snapshots" subtitle="Captured backup states" />
          <div style={{ display: "grid", gap: "var(--sp-2)" }}>
            {snapshots.map((snapshot, index) => {
              const confirming = confirmRestoreId === snapshot.id;
              const restoring = restoringSnapshotId === snapshot.id;
              return (
                <div
                  key={snapshot.id}
                  className="surface-panel screen-enter"
                  style={{
                    padding: "var(--sp-3)",
                    borderRadius: "var(--r-lg)",
                    display: "grid",
                    gap: 8,
                    animationDelay: `${Math.min(index, 8) * 35}ms`
                  }}
                >
                  <div style={{ display: "flex", justifyContent: "space-between", gap: "var(--sp-2)", alignItems: "center" }}>
                    <div className="font-display" style={{ fontSize: 15, fontWeight: 700 }}>{snapshot.id}</div>
                    <StatusPill status={snapshot.healthScore >= 75 ? "ok" : snapshot.healthScore >= 45 ? "warn" : "danger"} label={`Health ${snapshot.healthScore.toFixed(0)}`} />
                  </div>
                  <div className="tiny" style={{ color: "var(--text-secondary)" }}>{relativeTime(snapshot.createdAt)}</div>
                  <div className="tiny" style={{ color: "var(--text-tertiary)", wordBreak: "break-word" }}>{snapshot.path}</div>
                  {confirming ? (
                    <div className="surface-subtle" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "grid", gap: "var(--sp-2)" }}>
                      <div className="tiny" style={{ color: "var(--text-secondary)" }}>
                        Restore this snapshot baseline? This will disable startup items not present in the captured baseline and create a restore point first when possible.
                      </div>
                      <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                        <GlowButton label="Confirm Restore" onClick={() => void restoreSnapshot(snapshot)} loading={restoring} />
                        <GlowButton label="Cancel" variant="ghost" onClick={() => setConfirmRestoreId(null)} disabled={restoring} />
                      </div>
                    </div>
                  ) : (
                    <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
                      <GlowButton label="Restore" variant="ghost" onClick={() => setConfirmRestoreId(snapshot.id)} />
                    </div>
                  )}
                </div>
              );
            })}
            {snapshots.length === 0 ? <div className="tiny" style={{ color: "var(--text-secondary)" }}>No snapshots captured yet.</div> : null}
          </div>
        </GlassCard>
      </div>
    </div>
  );
}

