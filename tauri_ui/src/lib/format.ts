export function percentDelta(current: number, previous?: number): number {
  if (previous === undefined || previous === 0) {
    return 0;
  }
  return ((current - previous) / previous) * 100;
}

export function healthLabel(score: number): string {
  if (score >= 80) {
    return "GOOD";
  }
  if (score >= 55) {
    return "FAIR";
  }
  return "CRITICAL";
}

export function mbToGb(mb: number): number {
  return mb / 1024;
}
