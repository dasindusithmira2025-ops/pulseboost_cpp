#include "PulseBoostAI/ai/chat_router.hpp"

namespace pulseboost {

ChatRouter::ChatRouter(QObject *parent) : QObject(parent) {}

ChatIntent ChatRouter::route(const QString &message) const {
    const QString lower = message.toLower();

    if (lower.contains("clean") || lower.contains("junk") || lower.contains("temp")) {
        return ChatIntent::Clean;
    }
    if (lower.contains("optimize system") || lower.contains("optimize my") ||
        lower.contains("speed up") || lower.contains("make it faster")) {
        return ChatIntent::Optimize;
    }
    if (lower.contains("game mode") || lower.contains("boost gaming") ||
        lower.contains("gaming") || lower.contains("fps")) {
        return ChatIntent::GameMode;
    }
    if (lower.contains("developer") || lower.contains("docker") ||
        lower.contains("unreal") || lower.contains("android studio") ||
        lower.contains("visual studio") || lower.contains("compile")) {
        return ChatIntent::DeveloperMode;
    }
    if (lower.contains("startup") || lower.contains("boot")) {
        return ChatIntent::Startup;
    }
    if (lower.contains("stop") || lower.contains("kill") || lower.contains("close background")) {
        return ChatIntent::StopHeavyProcesses;
    }
    if (lower.contains("analyze") || lower.contains("scan") ||
        lower.contains("check") || lower.contains("diagnose") || lower.contains("why")) {
        return ChatIntent::Analyze;
    }

    return ChatIntent::FreeForm;
}

AgentDecision ChatRouter::buildDecision(const QString &message, const SystemSnapshot &snapshot) const {
    Q_UNUSED(message)

    AgentDecision decision;
    switch (route(message)) {
    case ChatIntent::Clean:
        decision.plan.push_back({"Clean temporary files", "Remove temp/cache data from safe disposable paths.", true});
        decision.shouldExecutePlan = true;
        break;

    case ChatIntent::Optimize:
        decision.plan.push_back({"Create restore point", "Capture a safe rollback point before any optimization.", true});
        if (snapshot.diskUsagePercent > 75.0) {
            decision.plan.push_back({"Recover storage space", "Run the junk cleaner and reclaim disposable files.", true});
        }
        if (snapshot.startupPrograms > 8) {
            decision.plan.push_back({"Review startup load", "Flag high-impact startup entries for manual approval.", false});
        }
        if (!snapshot.heavyProcesses.empty()) {
            decision.plan.push_back({"Address heavy background processes", "Identify the apps driving CPU and memory pressure.", false});
        }
        decision.shouldExecutePlan = true;
        break;

    case ChatIntent::GameMode:
        decision.plan.push_back({"Enable game mode", "Prioritize the game process and reduce background contention.", true});
        decision.shouldExecutePlan = true;
        break;

    case ChatIntent::DeveloperMode:
        decision.plan.push_back({"Apply developer profile", "Prioritize build-related tools and reduce background interference.", true});
        break;

    case ChatIntent::Startup:
        decision.plan.push_back({"Audit startup programs", "List and rank startup entries by estimated impact.", false});
        break;

    case ChatIntent::StopHeavyProcesses:
        decision.plan.push_back({"Review heavy processes", "Recommend which non-critical processes to close first.", false});
        break;

    case ChatIntent::Analyze:
    case ChatIntent::FreeForm:
    default:
        decision.plan.push_back({"Analyze live telemetry", "Use the current CPU, RAM, disk, startup, and process state for diagnosis.", false});
        break;
    }

    return decision;
}

}  // namespace pulseboost
