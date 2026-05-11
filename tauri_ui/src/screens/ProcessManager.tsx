import { useMemo, useState } from "react";
import { FixedSizeList as List, type ListChildComponentProps } from "react-window";
import GlassCard from "../components/ui/GlassCard";
import GlowButton from "../components/ui/GlowButton";
import SectionHeader from "../components/ui/SectionHeader";
import StatusPill from "../components/ui/StatusPill";
import { useCommands } from "../hooks/useCommands";
import { useSystemStore } from "../stores/systemStore";
import type { ProcessInfo } from "../types/system";

type SortBy = "name" | "cpu" | "ram";
type FilterBy = "all" | "high_cpu" | "high_ram" | "suspended" | "critical" | "suspicious";

type RiskTone = "ok" | "warn" | "danger" | "info";

function riskMeta(process: ProcessInfo): { label: string; tone: RiskTone } {
  switch (process.riskLabel) {
    case "critical":
      return { label: "critical", tone: "danger" };
    case "suspicious":
      return { label: "suspicious", tone: "warn" };
    case "safe":
      return { label: "safe", tone: "ok" };
    default:
      return { label: "unknown", tone: "info" };
  }
}

function statusTone(process: ProcessInfo): "ok" | "warn" | "danger" | "info" {
  if (process.isCritical) {
    return "danger";
  }
  if (process.cpuPercent > 80) {
    return "danger";
  }
  if (process.ramMb > 1800) {
    return "warn";
  }
  if (process.status === "suspended") {
    return "info";
  }
  return "ok";
}

export default function ProcessManager() {
  const { run } = useCommands();
  const processes = useSystemStore((s) => s.snapshot?.processes ?? []);
  const [query, setQuery] = useState("");
  const [sortBy, setSortBy] = useState<SortBy>("cpu");
  const [filter, setFilter] = useState<FilterBy>("all");
  const [confirmKillPid, setConfirmKillPid] = useState<number | null>(null);

  const filtered = useMemo(() => {
    let output = processes.filter((process) =>
      process.name.toLowerCase().includes(query.toLowerCase())
    );
    if (filter === "high_cpu") {
      output = output.filter((process) => process.cpuPercent >= 70);
    } else if (filter === "high_ram") {
      output = output.filter((process) => process.ramMb >= 1200);
    } else if (filter === "suspended") {
      output = output.filter((process) => process.status === "suspended");
    } else if (filter === "critical") {
      output = output.filter((process) => process.isCritical || process.riskLabel === "critical");
    } else if (filter === "suspicious") {
      output = output.filter((process) => process.riskLabel === "suspicious");
    }

    return output.sort((left, right) => {
      if (sortBy === "name") {
        return left.name.localeCompare(right.name);
      }
      if (sortBy === "ram") {
        return right.ramMb - left.ramMb;
      }
      return right.cpuPercent - left.cpuPercent;
    });
  }, [filter, processes, query, sortBy]);

  const Row = ({ index, style }: ListChildComponentProps) => {
    const process = filtered[index];
    const status = statusTone(process);
    const risk = riskMeta(process);
    const isConfirmingKill = confirmKillPid === process.pid;
    const protectedProcess = Boolean(process.isCritical || process.riskLabel === "critical");

    return (
      <div style={{ ...style, padding: "0 var(--sp-2)" }}>
        <div
          className="glass-card interactive"
          style={{
            minHeight: 74,
            display: "grid",
            gridTemplateColumns: "2.2fr 1fr 1fr 1fr 1fr 1fr auto",
            alignItems: "center",
            gap: "var(--sp-2)",
            padding: "var(--sp-2) var(--sp-3)",
            background: index % 2 ? "var(--bg-glass)" : "transparent"
          }}
        >
          <div style={{ minWidth: 0 }}>
            <div className="font-mono" style={{ fontSize: 12, whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }} title={process.name}>
              {process.name}
            </div>
            <div className="tiny">PID {process.pid}</div>
            <div className="tiny" style={{ color: "var(--text-tertiary)", whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }} title={process.imagePath || "Path unavailable"}>
              {process.imagePath || "Path unavailable"}
            </div>
          </div>
          <div>
            <div className="font-mono tiny">{process.cpuPercent.toFixed(1)}%</div>
            <div style={{ height: 4, borderRadius: 99, background: "var(--bg-glass-hover)" }}>
              <div
                style={{
                  width: `${Math.min(100, process.cpuPercent)}%`,
                  height: "100%",
                  borderRadius: 99,
                  background: "var(--accent)"
                }}
              />
            </div>
          </div>
          <div className="font-mono tiny">{process.ramMb.toFixed(0)} MB</div>
          <StatusPill status={status} label={process.status.replace("_", " ")} />
          <StatusPill status={risk.tone} label={risk.label} />
          <div className="tiny" style={{ color: protectedProcess ? "var(--status-danger)" : "var(--text-secondary)" }}>
            {protectedProcess ? "protected" : process.priority}
          </div>
          <div style={{ display: "flex", gap: "var(--sp-1)", justifyContent: "flex-end", flexWrap: "wrap" }}>
            {process.status === "suspended" ? (
              <GlowButton label="Resume" variant="ghost" onClick={() => void run("resume_process", { pid: process.pid })} />
            ) : (
              <GlowButton label="Suspend" variant="ghost" onClick={() => void run("suspend_process", { pid: process.pid })} />
            )}
            {protectedProcess ? (
              <GlowButton label="Protected" variant="ghost" disabled title="Critical system process cannot be terminated" />
            ) : isConfirmingKill ? (
              <>
                <GlowButton
                  label="Confirm"
                  variant="danger"
                  onClick={() => {
                    setConfirmKillPid(null);
                    void run("kill_process", { pid: process.pid });
                  }}
                />
                <GlowButton label="Cancel" variant="ghost" onClick={() => setConfirmKillPid(null)} />
              </>
            ) : (
              <GlowButton label="Kill" variant="danger" onClick={() => setConfirmKillPid(process.pid)} />
            )}
          </div>
        </div>
      </div>
    );
  };

  return (
    <div className="screen-enter" style={{ height: "100%", display: "grid", gap: "var(--sp-3)" }}>
      <SectionHeader title="Process Manager" subtitle="Search, sort, and control processes with risk-aware safeguards" />
      <GlassCard>
        <div style={{ display: "grid", gridTemplateColumns: "2fr auto auto", gap: "var(--sp-2)" }}>
          <input
            value={query}
            onChange={(event) => setQuery(event.target.value)}
            className="focus-ring"
            placeholder="Search process..."
            style={{
              height: 34,
              borderRadius: "var(--r-md)",
              border: "1px solid var(--border-glass)",
              background: "var(--bg-glass)",
              color: "var(--text-primary)",
              padding: "0 var(--sp-3)",
              outline: "none"
            }}
          />
          <select
            value={sortBy}
            onChange={(event) => setSortBy(event.target.value as SortBy)}
            className="focus-ring"
            style={{
              height: 34,
              borderRadius: "var(--r-md)",
              border: "1px solid var(--border-glass)",
              background: "var(--bg-glass)",
              color: "var(--text-primary)",
              padding: "0 var(--sp-2)"
            }}
          >
            <option value="cpu">Sort: CPU</option>
            <option value="ram">Sort: RAM</option>
            <option value="name">Sort: Name</option>
          </select>
          <select
            value={filter}
            onChange={(event) => setFilter(event.target.value as FilterBy)}
            className="focus-ring"
            style={{
              height: 34,
              borderRadius: "var(--r-md)",
              border: "1px solid var(--border-glass)",
              background: "var(--bg-glass)",
              color: "var(--text-primary)",
              padding: "0 var(--sp-2)"
            }}
          >
            <option value="all">All</option>
            <option value="high_cpu">High CPU</option>
            <option value="high_ram">High RAM</option>
            <option value="suspended">Suspended</option>
            <option value="critical">Critical</option>
            <option value="suspicious">Suspicious</option>
          </select>
        </div>
      </GlassCard>

      <GlassCard className="h-full">
        <div
          style={{
            display: "grid",
            gridTemplateColumns: "2.2fr 1fr 1fr 1fr 1fr 1fr auto",
            gap: "var(--sp-2)",
            marginBottom: "var(--sp-2)",
            padding: "0 var(--sp-3)",
            color: "var(--text-tertiary)",
            fontSize: 11
          }}
        >
          <span>Name</span>
          <span>CPU</span>
          <span>RAM</span>
          <span>Status</span>
          <span>Risk</span>
          <span>Control</span>
          <span>Actions</span>
        </div>
        <List
          height={420}
          width="100%"
          itemCount={filtered.length}
          itemSize={80}
          overscanCount={3}
        >
          {Row}
        </List>
      </GlassCard>
    </div>
  );
}

