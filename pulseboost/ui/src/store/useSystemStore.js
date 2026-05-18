import { create } from "zustand";

const MAX_HISTORY = 400;

const useSystemStore = create((set, get) => ({
  connected: false,
  metrics: null,
  metricsHistory: [],
  healthScore: 100,
  breakdown: {},
  efficiencyScore: 100,
  efficiencyGrade: "A",
  efficiencyBreakdown: {},
  sessionMode: "normal",
  currentBottleneck: null,
  bottleneckDetails: null,
  frametime: null,
  sessionConfidence: 0,
  sessionEvidence: [],
  activeSession: null,
  recentSessions: [],
  anomalies: [],
  predictions: [],
  alerts: [],
  actions: [],
  suggestedActions: [],
  optimizations: [],
  processIntelligence: [],
  pillarScores: {},
  scheduler: { is_idle: false, deferral_candidates: [], optimal_maintenance_window: "23:00", queued_tasks: [] },
  similarEvents: [],
  baselineStatus: { ready: false, progress_percent: 0 },
  adaptive: { enabled: false, recent_actions: [], notifications: [], executed_actions: [] },
  features: {},
  featureAccess: {},
  plan: "free",
  planInfo: null,
  machine: null,
  executor: null,
  authStatus: null,
  entitlementSnapshot: null,
  foundationStatus: null,
  tweakCatalog: [],
  auditEntries: [],
  benchmarkResults: [],
  networkDiagnostics: null,
  gpuStats: null,
  biosChecklist: null,
  gameCatalog: [],
  currentGameProfile: null,
  trustCenter: null,
  activePage: "dashboard",
  chatMessages: [],
  chatStreaming: false,
  currentChatToken: "",
  selectedTimelineIndex: null,
  timelineHours: 24,
  errorMessage: "",

  setConnected: (connected) => set({ connected }),
  setErrorMessage: (errorMessage) => set({ errorMessage }),
  setMetricsHistory: (metricsHistory) => set({ metricsHistory }),
  setTimelineHours: (timelineHours) => set({ timelineHours, selectedTimelineIndex: null }),
  setFoundationStatus: (foundationStatus) => set({
    foundationStatus,
    plan: foundationStatus?.plan ?? foundationStatus?.plan_info?.plan_tier ?? "free",
    planInfo: foundationStatus?.plan_info ?? null,
    features: foundationStatus?.features ?? {},
    featureAccess: foundationStatus?.feature_access ?? {},
    authStatus: foundationStatus?.auth ?? null,
    entitlementSnapshot: foundationStatus?.entitlement_snapshot ?? null,
    machine: foundationStatus?.machine ?? null,
    executor: foundationStatus?.executor ?? null,
  }),
  setTweakCatalog: (tweakCatalog) => set({ tweakCatalog }),
  setAuditEntries: (auditEntries) => set({ auditEntries }),
  setBenchmarkResults: (benchmarkResults) => set({ benchmarkResults }),
  setNetworkDiagnostics: (networkDiagnostics) => set({ networkDiagnostics }),
  setGpuStats: (gpuStats) => set({ gpuStats }),
  setBiosChecklist: (biosChecklist) => set({ biosChecklist }),
  setGameCatalog: (gameCatalog) => set({ gameCatalog }),
  setCurrentGameProfile: (currentGameProfile) => set({ currentGameProfile }),
  setTrustCenter: (trustCenter) => set({ trustCenter }),
  setActivePage: (activePage) => set({ activePage }),
  setSystemUpdate: (payload) =>
    set((state) => {
      const metrics = payload.metrics || state.metrics;
      const historyEntry = metrics
        ? {
            timestamp: payload.timestamp || metrics.timestamp,
            cpu_total: metrics.cpu_total,
            ram_percent: metrics.ram_percent,
            disk_percent: metrics.disk_percent,
            net_sent_bytes: metrics.net_sent_bytes,
            net_recv_bytes: metrics.net_recv_bytes,
            temperature: metrics.temperature,
            health_score: payload.health_score,
            session_mode: payload.session_mode,
            sample_period: 3,
          }
        : null;
      const nextHistory = historyEntry
        ? [...state.metricsHistory, historyEntry].slice(-MAX_HISTORY)
        : state.metricsHistory;

      return {
        metrics,
        healthScore: payload.health_score ?? state.healthScore,
        breakdown: payload.breakdown ?? state.breakdown,
        efficiencyScore: payload.efficiency_score ?? state.efficiencyScore,
        efficiencyGrade: payload.efficiency_grade ?? state.efficiencyGrade,
        efficiencyBreakdown: payload.efficiency_breakdown ?? state.efficiencyBreakdown,
        sessionMode: payload.session_mode ?? state.sessionMode,
        currentBottleneck: payload.current_bottleneck ?? payload.metrics?.current_bottleneck ?? state.currentBottleneck,
        bottleneckDetails: payload.bottleneck_details ?? payload.metrics?.bottleneck_details ?? state.bottleneckDetails,
        frametime: payload.frametime ?? payload.metrics?.frametime ?? state.frametime,
        sessionConfidence: payload.session_confidence ?? state.sessionConfidence,
        sessionEvidence: payload.session_evidence ?? state.sessionEvidence,
        activeSession: payload.active_session ?? state.activeSession,
        recentSessions: payload.recent_sessions ?? state.recentSessions,
        anomalies: payload.anomalies ?? state.anomalies,
        predictions: payload.predictions ?? state.predictions,
        alerts: payload.alerts ?? state.alerts,
        actions: payload.actions ?? state.actions,
        suggestedActions: payload.suggested_actions ?? state.suggestedActions,
        optimizations: payload.optimizations ?? state.optimizations,
        processIntelligence: payload.process_intelligence ?? state.processIntelligence,
        pillarScores: payload.pillar_scores ?? state.pillarScores,
        scheduler: payload.scheduler ?? state.scheduler,
        similarEvents: payload.similar_events ?? state.similarEvents,
        baselineStatus: payload.baseline_status ?? state.baselineStatus,
        adaptive: payload.adaptive ?? state.adaptive,
        plan: payload.plan ?? state.plan,
        planInfo: payload.plan_info ?? state.planInfo,
        features: payload.features ?? state.features,
        featureAccess: payload.feature_access ?? state.featureAccess,
        authStatus: payload.auth ?? state.authStatus,
        entitlementSnapshot: payload.entitlement_snapshot ?? state.entitlementSnapshot,
        machine: payload.machine ?? state.machine,
        executor: payload.executor ?? state.executor,
        metricsHistory: payload.timeline?.length ? payload.timeline : nextHistory,
      };
    }),
  setSelectedTimelineIndex: (selectedTimelineIndex) => set({ selectedTimelineIndex }),
  clearTimelineSelection: () => set({ selectedTimelineIndex: null }),
  startChatStream: () => set({ chatStreaming: true, currentChatToken: "" }),
  appendChatToken: (token) =>
    set((state) => ({ currentChatToken: state.currentChatToken + token })),
  finishChatStream: () =>
    set((state) => ({
      chatMessages: [
        ...state.chatMessages,
        { role: "assistant", content: state.currentChatToken.trim(), timestamp: Date.now() },
      ],
      chatStreaming: false,
      currentChatToken: "",
    })),
  addUserMessage: (content) =>
    set((state) => ({ chatMessages: [...state.chatMessages, { role: "user", content, timestamp: Date.now() }] })),
  clearChat: () => set({ chatMessages: [], chatStreaming: false, currentChatToken: "" }),
  selectedFrame: () => {
    const { selectedTimelineIndex, metricsHistory, metrics, healthScore, sessionMode } = get();
    if (selectedTimelineIndex === null || !metricsHistory[selectedTimelineIndex]) {
      return {
        metrics,
        healthScore,
        sessionMode,
        historical: false,
      };
    }
    const point = metricsHistory[selectedTimelineIndex];
    return {
      metrics: {
        ...(metrics || {}),
        cpu_total: point.cpu_total,
        ram_percent: point.ram_percent,
        disk_percent: point.disk_percent,
        net_sent_bytes: point.net_sent ?? point.net_sent_bytes ?? 0,
        net_recv_bytes: point.net_recv ?? point.net_recv_bytes ?? 0,
        temperature: point.temperature,
      },
      healthScore: point.health_score,
      sessionMode: point.session_mode,
      historical: true,
      timestamp: point.timestamp,
      samplePeriod: point.sample_period,
    };
  },
  featureEnabled: (featureKey) => Boolean(get().featureAccess?.[featureKey]),
}));

export default useSystemStore;











