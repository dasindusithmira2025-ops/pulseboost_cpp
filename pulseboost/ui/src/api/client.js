export async function apiJsonFetch(url, options) {
  const response = await fetch(url, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  if (!response.ok) {
    let detail = `${response.status} ${response.statusText}`;
    try {
      const payload = await response.json();
      detail = payload.detail?.reasoning || payload.detail?.abort_reason || payload.detail || detail;
    } catch {
      // ignore
    }
    throw new Error(detail);
  }
  return response.status === 204 ? null : response.json();
}

export function fetchTimeline(hours, limit = 400) {
  return apiJsonFetch(`/api/timeline?hours=${hours}&limit=${limit}`);
}

export function fetchState() {
  return apiJsonFetch("/api/state");
}

export function fetchMetrics() {
  return apiJsonFetch("/api/metrics");
}

export function fetchStatus() {
  return apiJsonFetch("/api/status");
}

export function fetchCapabilities() {
  return apiJsonFetch("/api/capabilities");
}

export function fetchSettings() {
  return apiJsonFetch("/api/settings");
}

export function fetchAuthStatus() {
  return apiJsonFetch("/api/auth/status");
}

export function createLocalSession(payload) {
  return apiJsonFetch("/api/auth/local-session", {
    method: "POST",
    body: JSON.stringify(payload),
  });
}

export function refreshEntitlements() {
  return apiJsonFetch("/api/auth/refresh-entitlements", {
    method: "POST",
  });
}

export function signOutSession() {
  return apiJsonFetch("/api/auth/sign-out", {
    method: "POST",
  });
}

export function exchangeWebsiteToken(payload) {
  return apiJsonFetch("/api/auth/token-exchange", {
    method: "POST",
    body: JSON.stringify(payload),
  });
}

export function fetchAccountIdentity() {
  return apiJsonFetch("/api/account/identity");
}

export function fetchAccountPlan() {
  return apiJsonFetch("/api/account/plan");
}

export function fetchAccountEntitlements() {
  return apiJsonFetch("/api/account/entitlements");
}

export function fetchAccountActivation() {
  return apiJsonFetch("/api/account/activation");
}

export function fetchTweakCatalog() {
  return apiJsonFetch("/api/tweaks");
}

export function applyTweak(tweakId, params = {}) {
  return apiJsonFetch(`/api/tweaks/${tweakId}/apply`, {
    method: "POST",
    body: JSON.stringify({ params }),
  });
}

export function revertTweak(tweakId, snapshotId = null) {
  return apiJsonFetch(`/api/tweaks/${tweakId}/revert`, {
    method: "POST",
    body: JSON.stringify({ snapshot_id: snapshotId }),
  });
}

export function fetchAudit(limit = 100) {
  return apiJsonFetch(`/api/audit?limit=${limit}`);
}

export function revertAuditEntry(auditId) {
  return apiJsonFetch(`/api/audit/${auditId}/revert`, {
    method: "POST",
  });
}

export function previewAction(actionType, params) {
  return apiJsonFetch("/api/actions/preview", {
    method: "POST",
    body: JSON.stringify({ action_type: actionType, params }),
  });
}

export function runAction(actionType, params, confirmed = false) {
  return apiJsonFetch("/api/actions/execute", {
    method: "POST",
    body: JSON.stringify({ action_type: actionType, params, confirmed }),
  });
}

export function fetchBenchmarkResults(limit = 25) {
  return apiJsonFetch(`/api/benchmark/results?limit=${limit}`);
}

export function runBenchmark(payload) {
  return apiJsonFetch("/api/benchmark/run", {
    method: "POST",
    body: JSON.stringify(payload),
  });
}

export function exportBenchmarkResult(benchmarkId) {
  return fetch(`/api/benchmark/results/${benchmarkId}/export`).then(async (response) => {
    if (!response.ok) {
      throw new Error(`Failed to export benchmark (${response.status})`);
    }
    return response.text();
  });
}

export function toggleAdaptive(enabled) {
  return apiJsonFetch("/api/adaptive/toggle", {
    method: "POST",
    body: JSON.stringify({ enabled }),
  });
}

export function fetchNetworkDiagnostics() {
  return apiJsonFetch("/api/network/diagnostics");
}

export function applyNetworkQos(profileName, protocol = "udp") {
  return apiJsonFetch("/api/network/qos", {
    method: "POST",
    body: JSON.stringify({ profile_name: profileName, protocol }),
  });
}

export function fetchGpuStats() {
  return apiJsonFetch("/api/gpu/stats");
}

export function applyGpuSetting(settingId, value, confirmRisky = false) {
  return apiJsonFetch("/api/gpu/settings", {
    method: "POST",
    body: JSON.stringify({ setting_id: settingId, value, confirm_risky: confirmRisky }),
  });
}

export function fetchBiosChecklist() {
  return apiJsonFetch("/api/bios/checklist");
}

export function fetchGames() {
  return apiJsonFetch("/api/games");
}

export function fetchGameProfiles() {
  return apiJsonFetch("/api/games/profiles");
}

export function fetchGameProfile(gameId) {
  return apiJsonFetch(`/api/games/${gameId}/profile`);
}

export function saveGameProfile(gameId, payload) {
  return apiJsonFetch(`/api/games/${gameId}/profile`, {
    method: "POST",
    body: JSON.stringify(payload),
  });
}

export function exportGameProfile(gameId) {
  return fetch(`/api/games/${gameId}/export`).then(async (response) => {
    if (!response.ok) {
      throw new Error(`Failed to export profile (${response.status})`);
    }
    return response.text();
  });
}

export function importGameProfile(payload) {
  return apiJsonFetch("/api/games/import", {
    method: "POST",
    body: JSON.stringify(payload),
  });
}

export function fetchTrustCenterStatus() {
  return apiJsonFetch("/api/trust-center/status");
}

export function fetchOptimizations(limit = 25) {
  return apiJsonFetch(`/api/optimizations?limit=${limit}`);
}

export function fetchSuggestions() {
  return apiJsonFetch("/api/suggestions");
}

export function fetchHealthHistory(days = 7) {
  return apiJsonFetch(`/api/health/history?days=${days}`);
}

export function submitOptimizationDecision(optimizationId, decision = "approve") {
  return apiJsonFetch("/api/optimizations/decision", {
    method: "POST",
    body: JSON.stringify({ optimization_id: optimizationId, decision }),
  });
}

export function rollbackAllTrustChanges() {
  return apiJsonFetch("/api/trust-center/rollback-all", {
    method: "POST",
  });
}

export function undoTrustCategory(category) {
  return apiJsonFetch(`/api/trust-center/undo/${category}`, {
    method: "POST",
  });
}

export function setExpertMode(enabled) {
  return apiJsonFetch("/api/settings/expert-mode", {
    method: "POST",
    body: JSON.stringify({ enabled }),
  });
}

export function updateSettingsPreferences(preferences) {
  return apiJsonFetch("/api/settings/preferences", {
    method: "POST",
    body: JSON.stringify({ preferences }),
  });
}

export function runSettingsDataAction(action, payload = {}) {
  return apiJsonFetch("/api/settings/data-action", {
    method: "POST",
    body: JSON.stringify({ action, payload }),
  });
}


