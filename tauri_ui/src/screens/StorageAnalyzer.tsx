import { useEffect, useMemo, useState } from "react";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { useCommands } from "../hooks/useCommands";
import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";
import type { LargeFileItem } from "../types/system";

const categoryColor: Record<string, string> = {
  Documents: "var(--accent)",
  Media: "var(--accent2)",
  System: "var(--status-ok)",
  Other: "var(--text-secondary)",
  Apps: "var(--status-warn)"
};

function formatBytes(bytes: number): string {
  const gb = bytes / (1024 * 1024 * 1024);
  if (gb >= 1) {
    return `${gb.toFixed(2)} GB`;
  }
  return `${(bytes / (1024 * 1024)).toFixed(0)} MB`;
}

export default function StorageAnalyzer() {
  const [tab, setTab] = useState<"treemap" | "large">("treemap");
  const [largeFiles, setLargeFiles] = useState<LargeFileItem[]>([]);
  const [isScanningLargeFiles, setIsScanningLargeFiles] = useState(false);
  const [junkEstimateBytes, setJunkEstimateBytes] = useState(0);
  const [pendingDelete, setPendingDelete] = useState<string | null>(null);
  const [busyPath, setBusyPath] = useState<string | null>(null);
  const snapshot = useSystemStore((s) => s.snapshot);
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const { run } = useCommands();

  const categories = snapshot?.storageCategories ?? [];
  const total = useMemo(() => categories.reduce((acc, c) => acc + c.sizeGb, 0), [categories]);

  const loadJunkEstimate = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    try {
      const result = await tauriInvoke<{ bytes?: number }>("estimate_junk");
      setJunkEstimateBytes(result.bytes ?? 0);
    } catch {
      setJunkEstimateBytes(0);
    }
  };

  const refreshLargeFiles = async () => {
    if (!isTauriRuntime()) {
      return;
    }
    setIsScanningLargeFiles(true);
    try {
      const result = await tauriInvoke<{ files?: LargeFileItem[] }>("scan_large_files");
      setLargeFiles(result.files ?? []);
    } catch (error) {
      setLargeFiles([]);
      pushNotification(error instanceof Error ? error.message : "Large-file scan failed", "danger");
    } finally {
      setIsScanningLargeFiles(false);
    }
  };

  useEffect(() => {
    void loadJunkEstimate();
  }, [snapshot?.timestamp]);

  useEffect(() => {
    if (tab === "large") {
      void refreshLargeFiles();
    }
  }, [tab]);

  const openLocation = async (path: string) => {
    try {
      setBusyPath(path);
      await tauriInvoke<boolean>("open_file_location", { path });
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Could not open file location", "danger");
    } finally {
      setBusyPath(null);
    }
  };

  const deleteFile = async (path: string) => {
    try {
      setBusyPath(path);
      const result = await tauriInvoke<{ ok: boolean; bytes?: number }>("delete_large_file", { path });
      if (result.ok) {
        setLargeFiles((current) => current.filter((file) => file.path !== path));
        setPendingDelete(null);
        setJunkEstimateBytes((current) => current + (result.bytes ?? 0));
        pushNotification(`Deleted ${path.split(/[\/]/).pop() ?? "file"}`, "ok");
      }
    } catch (error) {
      pushNotification(error instanceof Error ? error.message : "Delete failed", "danger");
    } finally {
      setBusyPath(null);
    }
  };

  return (
    <div className="screen-enter" style={{ display: "grid", gridTemplateColumns: "320px minmax(0, 1fr)", gap: "var(--sp-4)", height: "100%" }}>
      <GlassCard>
        <SectionHeader title="Drives" subtitle="Usage summary" />
        <div style={{ display: "grid", gap: "var(--sp-3)" }}>
          <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)" }}>
            <div className="font-display" style={{ fontSize: 26, fontWeight: 800 }}>
              {snapshot?.diskUsedGb.toFixed(1)} / {snapshot?.diskTotalGb.toFixed(0)} GB
            </div>
            <div className="tiny">System drive usage</div>
            <div style={{ marginTop: "var(--sp-3)", height: 8, borderRadius: 999, background: "rgba(255,255,255,0.06)" }}>
              <div style={{ width: `${snapshot?.diskPercent ?? 0}%`, height: "100%", borderRadius: 999, background: "linear-gradient(90deg, var(--accent), var(--accent2))" }} />
            </div>
          </div>
          {categories.map((category) => (
            <div key={category.name} className="surface-panel" style={{ padding: "var(--sp-3)", borderRadius: "var(--r-lg)", display: "flex", justifyContent: "space-between", gap: "var(--sp-3)", alignItems: "center" }}>
              <div>
                <div className="font-display" style={{ fontSize: 15, fontWeight: 700 }}>{category.name}</div>
                <div className="tiny">{category.percent.toFixed(1)}%</div>
              </div>
              <span className="font-mono tiny">{category.sizeGb.toFixed(1)} GB</span>
            </div>
          ))}
        </div>
      </GlassCard>

      <GlassCard className="h-full" style={{ minHeight: 0 }}>
        <SectionHeader
          title="Storage Analyzer"
          subtitle="Treemap, large-file intelligence, and cleanup actions"
          action={
            <div style={{ display: "flex", gap: "var(--sp-2)" }}>
              <GlowButton label="Treemap" variant={tab === "treemap" ? "primary" : "ghost"} onClick={() => setTab("treemap")} />
              <GlowButton label="Large Files" variant={tab === "large" ? "primary" : "ghost"} onClick={() => setTab("large")} />
            </div>
          }
        />

        {tab === "treemap" ? (
          <div style={{ display: "grid", gap: "var(--sp-4)" }}>
            <div>
              <div className="title-kicker" style={{ marginBottom: 8 }}>Category surface</div>
              <div style={{ display: "grid", gridTemplateColumns: "repeat(12, minmax(0, 1fr))", gap: "var(--sp-2)" }}>
                {categories.map((category, index) => {
                  const span = Math.max(2, Math.round((category.percent / 100) * 12));
                  return (
                    <button
                      key={category.name}
                      type="button"
                      className="interactive focus-ring screen-enter"
                      style={{
                        gridColumn: `span ${span}`,
                        minHeight: 120 + (index % 3) * 24,
                        borderRadius: "var(--r-lg)",
                        border: `1px solid ${categoryColor[category.name] ?? "var(--border-glass-md)"}`,
                        background: "linear-gradient(180deg, rgba(255,255,255,0.05), rgba(255,255,255,0.02))",
                        color: "var(--text-primary)",
                        textAlign: "left",
                        padding: "var(--sp-3)",
                        cursor: "pointer",
                        animationDelay: `${Math.min(index, 8) * 35}ms`
                      }}
                    >
                      <div className="font-display" style={{ fontWeight: 700, marginBottom: 6 }}>{category.name}</div>
                      <div className="font-mono tiny">{category.sizeGb.toFixed(1)} GB</div>
                    </button>
                  );
                })}
              </div>
            </div>
            <div className="badge-row">
              {categories.map((category) => (
                <span key={`legend-${category.name}`} className="glass-card" style={{ padding: "var(--sp-1) var(--sp-2)", borderColor: categoryColor[category.name] ?? "var(--border-glass)" }}>
                  <span className="font-mono tiny">{category.name}: {(total ? (category.sizeGb / total) * 100 : 0).toFixed(1)}%</span>
                </span>
              ))}
            </div>
          </div>
        ) : (
          <div style={{ display: "grid", gap: "var(--sp-2)" }}>
            {isScanningLargeFiles ? (
              <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)" }}>
                <span className="tiny">Scanning for large files...</span>
              </div>
            ) : largeFiles.length === 0 ? (
              <div className="surface-panel" style={{ padding: "var(--sp-4)", borderRadius: "var(--r-lg)" }}>
                <span className="tiny">No large files found in the default user folders.</span>
              </div>
            ) : largeFiles.map((file, index) => {
              const isPending = pendingDelete === file.path;
              const isBusy = busyPath === file.path;
              return (
                <div key={file.path} className="surface-panel screen-enter" style={{ display: "grid", gap: "var(--sp-2)", padding: "var(--sp-3)", borderRadius: "var(--r-lg)", animationDelay: `${Math.min(index, 8) * 35}ms` }}>
                  <div style={{ display: "grid", gridTemplateColumns: "minmax(0, 1fr) auto", gap: "var(--sp-3)", alignItems: "center" }}>
                    <div style={{ minWidth: 0 }}>
                      <div className="font-mono tiny" title={file.path} style={{ whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>
                        {file.path}
                      </div>
                      <div className="tiny" style={{ color: "var(--text-tertiary)", marginTop: 4 }}>
                        {formatBytes(file.bytes)}
                      </div>
                    </div>
                    <div style={{ display: "flex", gap: "var(--sp-2)", flexWrap: "wrap", justifyContent: "flex-end" }}>
                      <GlowButton label="Open Folder" variant="ghost" onClick={() => void openLocation(file.path)} loading={isBusy && !isPending} />
                      {!isPending ? (
                        <GlowButton label="Delete" variant="ghost" onClick={() => setPendingDelete(file.path)} />
                      ) : (
                        <>
                          <StatusPill status="warn" label="Delete this file?" />
                          <GlowButton label="Confirm" onClick={() => void deleteFile(file.path)} loading={isBusy} />
                          <GlowButton label="Cancel" variant="ghost" onClick={() => setPendingDelete(null)} disabled={isBusy} />
                        </>
                      )}
                    </div>
                  </div>
                </div>
              );
            })}
          </div>
        )}

        <div style={{ marginTop: "var(--sp-4)", display: "flex", gap: "var(--sp-2)", flexWrap: "wrap" }}>
          <GlowButton label={`Clean Junk (${formatBytes(junkEstimateBytes)})`} onClick={() => void run("clean_junk")} />
          <GlowButton label="Find Duplicates" variant="ghost" onClick={() => void run("find_duplicates")} />
          <GlowButton label="Optimize Drive" variant="ghost" onClick={() => void run("optimize_disk")} />
          {tab === "large" ? <GlowButton label="Re-scan Large Files" variant="ghost" onClick={() => void refreshLargeFiles()} loading={isScanningLargeFiles} /> : null}
        </div>
      </GlassCard>
    </div>
  );
}

