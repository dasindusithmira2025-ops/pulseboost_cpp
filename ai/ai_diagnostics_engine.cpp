#include "PulseBoostAI/ai/ai_diagnostics_engine.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <limits>
#include <sstream>

#include "PulseBoostAI/ai/system_prediction.hpp"

namespace pulseboost {

namespace {

struct RiskSignal {
    std::string title;
    double score = 0.0;
    std::string evidence;
};

std::string toLower(std::string value) {
    for (char &character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

bool containsAny(const std::string &text, const std::initializer_list<const char *> &tokens) {
    for (const char *token : tokens) {
        if (text.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

double normalizedPressure(double value, double low, double high) {
    if (value <= low) {
        return 0.0;
    }
    if (value >= high) {
        return 100.0;
    }
    const double range = high - low;
    if (range <= std::numeric_limits<double>::epsilon()) {
        return 0.0;
    }
    return (value - low) * 100.0 / range;
}

double trendSlope(const std::vector<SystemSnapshot> &history,
                  const std::function<double(const SystemSnapshot &)> &metric) {
    const std::size_t n = history.size();
    if (n < 3) {
        return 0.0;
    }

    double sumX = 0.0;
    double sumY = 0.0;
    double sumXY = 0.0;
    double sumXX = 0.0;
    for (std::size_t index = 0; index < n; ++index) {
        const double x = static_cast<double>(index);
        const double y = metric(history[index]);
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }

    const double denom = static_cast<double>(n) * sumXX - sumX * sumX;
    if (std::abs(denom) < std::numeric_limits<double>::epsilon()) {
        return 0.0;
    }

    return (static_cast<double>(n) * sumXY - sumX * sumY) / denom;
}

std::vector<RiskSignal> buildRiskSignals(const SystemSnapshot &snapshot) {
    std::vector<RiskSignal> signals;
    signals.push_back(RiskSignal {
        .title = "Disk saturation",
        .score = normalizedPressure(snapshot.diskUsagePercent, 72.0, 95.0),
        .evidence = "System drive at " + std::to_string(static_cast<int>(snapshot.diskUsagePercent)) + "% used",
    });
    signals.push_back(RiskSignal {
        .title = "Memory pressure",
        .score = normalizedPressure(snapshot.ramUsagePercent, 68.0, 92.0),
        .evidence = "RAM usage at " + std::to_string(static_cast<int>(snapshot.ramUsagePercent)) + "%",
    });
    signals.push_back(RiskSignal {
        .title = "CPU contention",
        .score = normalizedPressure(snapshot.cpuUsagePercent, 65.0, 95.0),
        .evidence = "CPU usage at " + std::to_string(static_cast<int>(snapshot.cpuUsagePercent)) + "%",
    });
    signals.push_back(RiskSignal {
        .title = "Startup drag",
        .score = normalizedPressure(static_cast<double>(snapshot.startupPrograms), 7.0, 22.0),
        .evidence = std::to_string(snapshot.startupPrograms) + " startup entries enabled",
    });

    double processPressure = 0.0;
    if (!snapshot.heavyProcesses.empty()) {
        const auto &top = snapshot.heavyProcesses.front();
        const double cpu = normalizedPressure(top.cpuPercent, 20.0, 80.0);
        const double mem = normalizedPressure(top.memoryMb, 600.0, 3200.0);
        processPressure = std::max(cpu, mem);
    }
    signals.push_back(RiskSignal {
        .title = "Background process pressure",
        .score = processPressure,
        .evidence = snapshot.heavyProcesses.empty()
                        ? "No heavy process detected"
                        : ("Top process: " + snapshot.heavyProcesses.front().name),
    });

    std::sort(signals.begin(), signals.end(), [](const RiskSignal &left, const RiskSignal &right) {
        return left.score > right.score;
    });
    return signals;
}

void pushPlanStep(std::vector<OptimizationPlanStep> &plan, const OptimizationPlanStep &step) {
    for (const auto &existing : plan) {
        if (existing.title == step.title) {
            return;
        }
    }
    plan.push_back(step);
}

}  // namespace

LocalAgentDecision AiDiagnosticsEngine::reason(const std::string &question,
                                               const SystemSnapshot &snapshot,
                                               const std::vector<SystemSnapshot> &history) const {
    LocalAgentDecision result;
    const std::string lower = toLower(question);
    const bool wantsCleanup = containsAny(lower, {"clean", "junk", "cache", "temp"});
    const bool wantsOptimize = containsAny(lower, {"optimize", "speed up", "faster", "fix"});
    const bool wantsGaming = containsAny(lower, {"game", "gaming", "fps", "lag", "stutter"});
    const bool wantsDeveloper = containsAny(lower, {"developer", "docker", "unreal", "android", "node", "build"});
    const bool wantsStartup = containsAny(lower, {"startup", "boot", "logon"});
    const bool imperative = containsAny(lower, {"run", "apply", "execute", "start", "now", "do it", "boost", "clean"});

    const auto signals = buildRiskSignals(snapshot);
    const std::size_t reportableSignals = std::min<std::size_t>(3, signals.size());

    if (wantsOptimize || wantsCleanup || wantsGaming || wantsDeveloper || (imperative && signals.front().score > 55.0)) {
        result.actions.push_back("create_restore_point");
    }

    if (wantsCleanup || (wantsOptimize && snapshot.diskUsagePercent >= 78.0)) {
        pushPlanStep(result.plan, OptimizationPlanStep {
                                  .title = "Recover storage headroom",
                                  .details = "Clean temporary and disposable cache data to reduce system drive pressure.",
                                  .execute = imperative,
                              });
        if (imperative) {
            result.actions.push_back("clean_junk");
        }
    }

    if (wantsStartup || snapshot.startupPrograms >= 9 || wantsOptimize) {
        pushPlanStep(result.plan, OptimizationPlanStep {
                                  .title = "Audit startup footprint",
                                  .details = "Rank startup entries by impact and disable only non-essential items.",
                                  .execute = imperative && (wantsStartup || wantsOptimize),
                              });
        if (imperative && (wantsStartup || wantsOptimize)) {
            result.actions.push_back("analyze_startup");
        }
    }

    if (wantsGaming) {
        pushPlanStep(result.plan, OptimizationPlanStep {
                                  .title = "Enable game-priority scheduling",
                                  .details = "Raise active game process priority and reduce non-critical background contention.",
                                  .execute = imperative,
                              });
        if (imperative) {
            result.actions.push_back("enable_game_mode");
        }
    }

    if (wantsDeveloper) {
        pushPlanStep(result.plan, OptimizationPlanStep {
                                  .title = "Apply developer workload profile",
                                  .details = "Tune priorities for build tools and reduce interference from idle background tasks.",
                                  .execute = imperative,
                              });
        if (imperative) {
            result.actions.push_back("optimize_developer_mode");
        }
    }

    if (result.plan.empty()) {
        const auto &primary = signals.front();
        if (primary.title == "Disk saturation") {
            pushPlanStep(result.plan, OptimizationPlanStep {
                                      .title = "Reduce disk saturation",
                                      .details = "Reclaim disposable data and move cold files off the system volume.",
                                      .execute = false,
                                  });
        } else if (primary.title == "Memory pressure") {
            pushPlanStep(result.plan, OptimizationPlanStep {
                                      .title = "Reduce memory pressure",
                                      .details = "Close or tune the largest resident background processes.",
                                      .execute = false,
                                  });
        } else if (primary.title == "Startup drag") {
            pushPlanStep(result.plan, OptimizationPlanStep {
                                      .title = "Trim startup load",
                                      .details = "Disable low-value startup items that add login delay.",
                                      .execute = false,
                                  });
        } else {
            pushPlanStep(result.plan, OptimizationPlanStep {
                                      .title = "Stabilize active workload",
                                      .details = "Inspect top CPU and memory consumers and remove unnecessary contention.",
                                      .execute = false,
                                  });
        }
    }

    std::sort(result.actions.begin(), result.actions.end());
    result.actions.erase(std::unique(result.actions.begin(), result.actions.end()), result.actions.end());

    SystemPrediction predictor;
    result.predictions = predictor.predict(history);
    const double diskSlope = trendSlope(history, [](const SystemSnapshot &sample) { return sample.diskUsagePercent; });
    const double ramSlope = trendSlope(history, [](const SystemSnapshot &sample) { return sample.ramUsagePercent; });
    const double healthSlope = trendSlope(history, [](const SystemSnapshot &sample) {
        return static_cast<double>(sample.healthScore);
    });

    if (diskSlope > 0.08) {
        result.predictions.push_back("Disk usage trend is increasing; storage pressure will likely worsen without cleanup.");
    }
    if (ramSlope > 0.15) {
        result.predictions.push_back("Memory trend is climbing; background process pressure may intensify during long sessions.");
    }
    if (healthSlope < -0.1) {
        result.predictions.push_back("Health score trend is declining; intervene early to prevent sustained slowdowns.");
    }
    if (result.predictions.size() > 4) {
        result.predictions.resize(4);
    }

    const double topRisk = signals.empty() ? 0.0 : signals.front().score;
    result.confidence = std::clamp(55 + static_cast<int>(topRisk * 0.35) + static_cast<int>(history.size() / 12), 55, 97);

    std::ostringstream stream;
    stream << "PulseBoost Local AI report (on-device engine)\n";
    stream << "Confidence: " << result.confidence << "%\n\n";
    stream << "Primary bottlenecks:\n";
    for (std::size_t index = 0; index < reportableSignals; ++index) {
        if (signals[index].score < 12.0) {
            continue;
        }
        stream << "- " << signals[index].title << " (" << static_cast<int>(signals[index].score) << "% risk): "
               << signals[index].evidence << '\n';
    }
    stream << "\nTelemetry snapshot:\n";
    stream << "- CPU " << static_cast<int>(snapshot.cpuUsagePercent) << "% | RAM " << static_cast<int>(snapshot.ramUsagePercent)
           << "% | Disk " << static_cast<int>(snapshot.diskUsagePercent) << "%\n";
    stream << "- Startup apps " << snapshot.startupPrograms << " | Services " << snapshot.runningServices << '\n';

    stream << "\nPLAN\n";
    for (std::size_t index = 0; index < result.plan.size(); ++index) {
        stream << (index + 1) << ". " << result.plan[index].title << " - " << result.plan[index].details << '\n';
    }

    if (!result.predictions.empty()) {
        stream << "\nForecast:\n";
        for (const auto &prediction : result.predictions) {
            stream << "- " << prediction << '\n';
        }
    }

    stream << "\nSafety: restore point should be created before any high-impact optimization.";
    result.reply = stream.str();
    return result;
}

std::string AiDiagnosticsEngine::summarizeSystem(const SystemSnapshot &snapshot) const {
    std::ostringstream stream;
    stream << "Detected issues:\n";
    if (snapshot.issues.empty()) {
        stream << "- No critical issues detected.\n";
    } else {
        for (const auto &issue : snapshot.issues) {
            stream << "- " << issue << '\n';
        }
    }

    stream << "\nRecommended actions:\n";
    if (snapshot.ramUsagePercent > 80.0) {
        stream << "- Close or tune high-memory processes.\n";
    }
    if (snapshot.startupPrograms > 10) {
        stream << "- Disable or delay non-essential startup apps.\n";
    }
    if (snapshot.diskUsagePercent > 85.0) {
        stream << "- Run junk cleanup and move large files off the system drive.\n";
    }
    if (snapshot.cpuUsagePercent > 85.0) {
        stream << "- Use game/developer mode selectively and inspect CPU-heavy processes.\n";
    }
    if (snapshot.issues.empty() && snapshot.healthScore >= 80) {
        stream << "- Maintain current configuration and review telemetry trends.\n";
    }

    return stream.str();
}

std::string AiDiagnosticsEngine::answerQuestion(const std::string &question, const SystemSnapshot &snapshot) const {
    return reason(question, snapshot, {}).reply;
}

}  // namespace pulseboost
