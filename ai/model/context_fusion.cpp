#include "PulseBoostAI/ai/model/context_fusion.hpp"

#include <algorithm>
#include <functional>

namespace pulseboost {

namespace {

double computeDelta(const QVector<SystemSnapshot> &history,
                    const std::function<double(const SystemSnapshot &)> &metric) {
    if (history.size() < 2) {
        return 0.0;
    }
    return metric(history.back()) - metric(history.front());
}

QString processNameBy(const SystemSnapshot &snap, bool byCpu) {
    if (snap.heavyProcesses.empty()) {
        return {};
    }

    const auto comparator = [byCpu](const ProcessInfo &left, const ProcessInfo &right) {
        if (byCpu) {
            return left.cpuPercent < right.cpuPercent;
        }
        return left.memoryMb < right.memoryMb;
    };
    const auto item = std::max_element(snap.heavyProcesses.begin(), snap.heavyProcesses.end(), comparator);
    if (item == snap.heavyProcesses.end()) {
        return {};
    }
    return QString::fromStdString(item->name);
}

}  // namespace

FusedContext ContextFusion::fuse(const QString &userInput,
                                 const ClassificationResult &intent,
                                 const SystemSnapshot &snap,
                                 const QVector<SystemSnapshot> &history,
                                 const QVector<QString> &recentActions,
                                 const QVector<QString> &conversationSummary,
                                 int totalSessionMessages) const {
    FusedContext context;
    context.rawInput = userInput;
    context.intent = intent;

    context.cpuPct = snap.cpuUsagePercent;
    context.ramPct = snap.ramUsagePercent;
    context.diskPct = snap.diskUsagePercent;
    context.networkMbps = snap.networkMbps;
    context.healthScore = snap.healthScore;
    context.cpuTempC = snap.thermalStateScore > 0 ? (100.0 - snap.thermalStateScore) + 40.0 : 0.0;
    context.processCount = static_cast<int>(snap.heavyProcesses.size());

    context.topCpuProcess = processNameBy(snap, true);
    context.topRamProcess = processNameBy(snap, false);
    context.isRamPressured = snap.ramUsagePercent >= 80.0 || snap.memoryPressureScore < 60;
    context.isCpuThrottling = snap.thermalStateScore < 45;
    context.isDiskSaturated = snap.diskUsagePercent >= 88.0 || snap.diskHealthScore < 45;

    context.cpuDelta = computeDelta(history, [](const SystemSnapshot &sample) { return sample.cpuUsagePercent; });
    context.ramDelta = computeDelta(history, [](const SystemSnapshot &sample) { return sample.ramUsagePercent; });
    context.healthDelta = computeDelta(history, [](const SystemSnapshot &sample) {
        return static_cast<double>(sample.healthScore);
    });
    context.cpuTrending = context.cpuDelta > 1.5;
    context.ramTrending = context.ramDelta > 1.0;
    context.healthTrending = context.healthDelta >= 0.0;

    context.recentActions = recentActions;
    context.conversationSummary = conversationSummary;
    context.totalSessionMessages = totalSessionMessages;

    context.currentUsageMode = detectUsageMode(snap);
    const int hour = QDateTime::currentDateTime().time().hour();
    context.isTypicalUsageTime = hour >= 8 && hour <= 22;
    context.mostCommonIssue = assessPrimaryIssue(snap);

    context.healthGrade = healthGradeLabel(context.healthScore);
    context.gameRunning = detectGameRunning(snap);
    context.devToolsRunning = detectDevToolsRunning(snap);
    context.primaryIssue = context.mostCommonIssue;

    if (snap.cpuUsagePercent >= 90.0) {
        context.activeAlerts.push_back("CPU load is critical.");
    }
    if (snap.ramUsagePercent >= 90.0) {
        context.activeAlerts.push_back("RAM pressure is critical.");
    }
    if (snap.diskUsagePercent >= 92.0) {
        context.activeAlerts.push_back("System drive capacity is critical.");
    }

    for (const QString &action : recentActions) {
        context.recentOptimizations.push_back(action);
    }

    return context;
}

QString ContextFusion::assessPrimaryIssue(const SystemSnapshot &snap) const {
    if (snap.diskUsagePercent >= 90.0) {
        return "system drive is near full capacity";
    }
    if (snap.ramUsagePercent >= 85.0) {
        return "memory pressure is causing paging";
    }
    if (snap.cpuUsagePercent >= 80.0) {
        return "CPU is under sustained high load";
    }
    if (snap.startupPrograms >= 12) {
        return "startup footprint is larger than normal";
    }
    return "no critical bottleneck is active";
}

QString ContextFusion::detectUsageMode(const SystemSnapshot &snap) const {
    if (detectGameRunning(snap)) {
        return "Gaming";
    }
    if (detectDevToolsRunning(snap)) {
        return "Work";
    }
    if (snap.cpuUsagePercent < 25.0 && snap.ramUsagePercent < 55.0) {
        return "Idle";
    }
    return "General";
}

bool ContextFusion::detectGameRunning(const SystemSnapshot &snap) const {
    for (const ProcessInfo &process : snap.heavyProcesses) {
        const QString name = QString::fromStdString(process.name).toLower();
        if (name.contains("game") || name.contains("steam") || name.contains("valorant") || name.contains("fortnite") ||
            name.contains("dota") || name.contains("cs2")) {
            return true;
        }
    }
    return false;
}

bool ContextFusion::detectDevToolsRunning(const SystemSnapshot &snap) const {
    for (const ProcessInfo &process : snap.heavyProcesses) {
        const QString name = QString::fromStdString(process.name).toLower();
        if (name.contains("code") || name.contains("devenv") || name.contains("cl.exe") || name.contains("docker") ||
            name.contains("rustc") || name.contains("javac") || name.contains("node")) {
            return true;
        }
    }
    return false;
}

QString ContextFusion::healthGradeLabel(double healthScore) const {
    if (healthScore >= 85.0) {
        return "Excellent";
    }
    if (healthScore >= 65.0) {
        return "Good";
    }
    if (healthScore >= 45.0) {
        return "Fair";
    }
    if (healthScore >= 25.0) {
        return "Poor";
    }
    return "Critical";
}

}  // namespace pulseboost
