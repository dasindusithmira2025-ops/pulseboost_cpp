export interface SystemSnapshot {
  cpuPercent: number;
  ramPercent: number;
  ramUsedMb: number;
  ramTotalMb: number;
  diskPercent: number;
  diskUsedGb: number;
  diskTotalGb: number;
  netDownloadKbps: number;
  netUploadKbps: number;
  gpuPercent: number;
  temperatureCelsius: number;
  fanRpm: number;
  healthScore: number;
  healthSummary: string;
  issues: string[];
  processes: ProcessInfo[];
  startupItems: StartupItem[];
  services: ServiceInfo[];
  drivers: DriverInfo[];
  storageCategories: StorageCategory[];
  timestamp: number;
}

export interface ProcessInfo {
  pid: number;
  name: string;
  imagePath?: string;
  cpuPercent: number;
  ramMb: number;
  status: "running" | "suspended" | "high_cpu" | "high_ram";
  priority: string;
  isCritical?: boolean;
  riskLabel?: "safe" | "unknown" | "suspicious" | "critical";
}

export interface StartupItem {
  name: string;
  publisher: string;
  location: string;
  enabled: boolean;
  impact: "low" | "medium" | "high";
  delayMs?: number;
}

export interface ServiceInfo {
  name: string;
  displayName: string;
  status: string;
  startType: string;
}

export interface DriverInfo {
  name: string;
  manufacturer: string;
  version: string;
  status: "ok" | "warning" | "error";
  date: string;
}

export interface StorageCategory {
  name: string;
  sizeGb: number;
  percent: number;
}

export interface LargeFileItem {
  path: string;
  bytes: number;
}

export interface RamBreakdown {
  ok: boolean;
  totalMb: number;
  usedMb: number;
  freeMb: number;
  appsMb: number;
  systemMb: number;
  cachedMb: number;
  recoverableMb: number;
}

export interface NetworkDiagnosticsPayload {
  ok: boolean;
  connectionType: string;
  adapterName: string;
  adapterDescription: string;
  dnsSuffix: string;
  dnsServers: string[];
  pings: NetworkPingResult[];
}

export interface RecentActionItem {
  timestamp: string;
  action: string;
  details: string;
  success: boolean;
}

export interface SnapshotRecord {
  id: string;
  path: string;
  createdAt: number;
  healthScore: number;
}

export interface ScheduledTask {
  id: string;
  enabled: boolean;
  type: string;
  intervalHours: number;
  lastRun: string;
}

export interface LicenseInfo {
  ok: boolean;
  isPro: boolean;
  isTrial: boolean;
  trialDaysLeft: number;
  tierLabel: string;
  licenseKey?: string;
  reason?: string;
}

export interface AiPreferences {
  ok: boolean;
  mode: "local" | "cloud";
  cloudConfigured: boolean;
  apiKeyMasked?: string;
}

export interface UpdateStatus {
  ok: boolean;
  currentVersion: string;
  updateAvailable: boolean;
  latestVersion?: string;
  downloadUrl?: string;
  sha256?: string;
  manifestTrusted?: boolean;
  lastError?: string;
}

export interface ErrorLogEntry {
  source: string;
  message: string;
}

export interface ErrorLogPayload {
  ok: boolean;
  entries: ErrorLogEntry[];
}

export interface ExportPathPayload {
  ok: boolean;
  path?: string;
  reason?: string;
}

export interface GameModeStatus {
  ok: boolean;
  active: boolean;
  pid: number;
  name: string;
  executableName?: string;
}

export interface NotificationItem {
  id: string;
  message: string;
  type: "info" | "ok" | "warn" | "danger";
  createdAt: number;
}

export interface ActionProgressPayload {
  action: string;
  percent: number;
  message: string;
}

export interface ActionCompletePayload {
  action: string;
  success: boolean;
  result: unknown;
}

export interface ActionProgressItem {
  action: string;
  percent: number;
  message: string;
  startedAt: number;
  completedAt?: number;
  success?: boolean;
  result?: unknown;
}

export interface ActionProof {
  action: string;
  lines: string[];
  capturedAt: number;
}

export type TweakCategory = "windows" | "gaming" | "network" | "ram" | "gpu" | "privacy" | "power";
export type ImpactLevel = "high" | "medium" | "low";
export type RiskLevel = "safe" | "moderate" | "advanced";

export interface Tweak {
  id: string;
  category: TweakCategory;
  name: string;
  description: string;
  detailedInfo: string;
  impact: ImpactLevel;
  risk: RiskLevel;
  requiresRestart: boolean;
  isApplied: boolean;
  isApplicable: boolean;
  notApplicableReason?: string;
}

export interface TweakResult {
  id: string;
  success: boolean;
  error?: string;
  requiresRestart: boolean;
}

export interface BenchmarkResult {
  cpuScore: number;
  ramBandwidthMBps: number;
  diskReadMBps: number;
  networkLatencyMs: number;
  gpuScore: number;
  compositeScore: number;
  pulseScore: number;
  grade: "S" | "A" | "B" | "C" | "D" | "F";
  timestamp: number;
}

export interface BenchmarkDelta {
  before: BenchmarkResult;
  after: BenchmarkResult;
  percentChange: number;
  scoreDelta: number;
}

export interface GameProfile {
  executableName: string;
  displayName: string;
  installPath: string;
  launcher: "steam" | "epic" | "battlenet" | "direct" | "other";
  isDetected: boolean;
  isOptimized: boolean;
  isRunning: boolean;
  runningPid?: number;
  tweaksApplied: string[];
  tweaksAvailable: number;
  gameIcon?: string;
}

export interface SystemAdvisorItem {
  id: string;
  category: "ram" | "gpu" | "bios" | "windows" | "security";
  title: string;
  description: string;
  impact: "high" | "medium" | "low";
  status: "optimal" | "suboptimal" | "warning" | "unknown";
  actionable: boolean;
  actionLabel?: string;
  actionId?: string;
}

export interface PulseScoreBreakdown {
  total: number;
  hardwareTier: number;
  optimizationLevel: number;
  healthState: number;
  benchScore: number;
  grade: "S" | "A" | "B" | "C" | "D" | "F";
  percentile: number;
}

export interface NetworkPingResult {
  host: string;
  name: string;
  latencyMs: number;
  status: "ok" | "slow" | "unreachable";
}

export interface PulseScorePayload extends PulseScoreBreakdown {
  tweaksApplied: number;
  tweaksAvailable: number;
  advisorItems: SystemAdvisorItem[];
  benchmarkRuns: number;
  latestBenchmark?: BenchmarkResult;
}

export interface HomeSummary {
  ok: boolean;
  pulseScore: PulseScorePayload;
  healthScore: number;
  healthSummary: string;
  recentActions: RecentActionItem[];
  advisorItems: SystemAdvisorItem[];
  benchmarkSummary: {
    runs: number;
    latest?: BenchmarkResult;
    latestDelta?: BenchmarkDelta;
  };
}

export interface BackupSummary {
  ok: boolean;
  actions: RecentActionItem[];
  snapshots: SnapshotRecord[];
  guidance: {
    backupRecommended: boolean;
    message: string;
  };
}

export interface OptimizationPreset {
  id: string;
  label: string;
  description: string;
  impact: "high" | "medium" | "low";
  recommended: boolean;
  proOnly: boolean;
  action?: string;
  tweakIds?: string[];
}

export interface BoostUpAction {
  id: string;
  label: string;
  description: string;
  duration: string;
  tone: "accent" | "info" | "warn" | "danger";
}

export type ToolsTab =
  | "processes"
  | "storage"
  | "network"
  | "startup"
  | "ram"
  | "thermals";

export type ScreenId =
  | "home"
  | "optimizations"
  | "boost_up"
  | "games"
  | "backup"
  | "ai"
  | "settings"
  | "onboarding";
