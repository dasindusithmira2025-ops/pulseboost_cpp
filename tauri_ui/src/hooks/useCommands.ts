import { isTauriRuntime, tauriInvoke } from "../lib/tauri";
import { useSystemStore } from "../stores/systemStore";

interface ResolvedCommand {
  name: string;
  args?: Record<string, unknown>;
}

function normalizeName(input: string): string {
  return input.trim().toLowerCase();
}

const progressDrivenCommands = new Set([
  "clean_junk",
  "full_scan",
  "optimize_disk",
  "find_duplicates",
  "run_security_scan",
  "autofix_low_risk",
  "fix_security_issue"
]);

export function useCommands() {
  const pushNotification = useSystemStore((s) => s.pushNotification);
  const fetchSnapshot = useSystemStore((s) => s.fetchSnapshot);
  const getSnapshot = useSystemStore((s) => s.snapshot);

  const resolveCommands = (
    command: string,
    args?: Record<string, unknown>
  ): ResolvedCommand[] => {
    const name = normalizeName(command);

    if (name.startsWith("kill_process:")) {
      const processName = name.slice("kill_process:".length).trim();
      if (!processName) {
        throw new Error("Missing process name in action.");
      }
      const snapshot = getSnapshot;
      const matches =
        snapshot?.processes.filter((p) =>
          p.name.toLowerCase().includes(processName.toLowerCase())
        ) ?? [];
      if (matches.length === 0) {
        throw new Error(`No running process matched: ${processName}`);
      }
      return matches.slice(0, 1).map((process) => ({
        name: "kill_process",
        args: { pid: process.pid }
      }));
    }

    switch (name) {
      case "optimize_network":
        return [{ name: "flush_dns" }, { name: "optimize_tcp" }];
      case "optimize_all":
        return [
          { name: "clean_junk" },
          { name: "optimize_ram" },
          { name: "optimize_disk" },
          { name: "flush_dns" }
        ];
      case "analyze_startup":
        return [{ name: "refresh_all" }];
      case "optimize_developer_mode":
        return [{ name: "optimize_ram" }];
      default:
        return [{ name, args }];
    }
  };

  const run = async (command: string, args?: Record<string, unknown>) => {
    try {
      if (!isTauriRuntime()) {
        throw new Error("Command execution requires Tauri runtime.");
      }

      const operations = resolveCommands(command, args);
      for (const operation of operations) {
        const result = await tauriInvoke<unknown>(operation.name, operation.args);
        if (typeof result === "boolean" && !result) {
          throw new Error(`${operation.name} returned failure.`);
        }
      }

      const directSingleOperation = operations.length === 1 ? operations[0]?.name ?? command : command;
      if (!progressDrivenCommands.has(directSingleOperation)) {
        pushNotification(`${command.split("_").join(" ")} completed`, "ok");
      }
    } catch (error) {
      pushNotification(
        error instanceof Error ? error.message : `${command} failed`,
        "danger"
      );
    } finally {
      await fetchSnapshot();
    }
  };

  return { run };
}

