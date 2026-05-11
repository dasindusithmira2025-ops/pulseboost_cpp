import { useMemo } from "react";
import { useSystemStore } from "../stores/systemStore";

export function useTelemetry() {
  const history = useSystemStore((s) => s.history);
  const snapshot = useSystemStore((s) => s.snapshot);

  return useMemo(() => {
    const cpu = history.cpu;
    const ram = history.ram;
    const disk = history.disk;
    const net = history.net;

    const summarize = (arr: number[]) => {
      if (!arr.length) {
        return { min: 0, max: 0, avg: 0 };
      }
      const min = Math.min(...arr);
      const max = Math.max(...arr);
      const avg = arr.reduce((a, b) => a + b, 0) / arr.length;
      return {
        min: Number(min.toFixed(1)),
        max: Number(max.toFixed(1)),
        avg: Number(avg.toFixed(1))
      };
    };

    return {
      history,
      summary: {
        cpu: summarize(cpu),
        ram: summarize(ram),
        disk: summarize(disk),
        net: summarize(net)
      },
      snapshot
    };
  }, [history, snapshot]);
}
