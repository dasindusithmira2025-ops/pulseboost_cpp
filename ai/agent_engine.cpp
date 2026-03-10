#include "PulseBoostAI/ai/agent_engine.hpp"

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

QString planSummary(const std::vector<OptimizationPlanStep> &plan) {
    if (plan.empty()) {
        return {};
    }

    QString summary = "PLAN\n";
    int index = 1;
    for (const auto &step : plan) {
        summary += QString("%1. %2\n").arg(index++).arg(QString::fromStdString(step.title));
    }
    return summary.trimmed();
}

}  // namespace

AgentEngine::AgentEngine(SystemScanner &scanner,
                         TelemetryCache &telemetryCache,
                         JunkCleaner &junkCleaner,
                         StartupOptimizer &startupOptimizer,
                         GameMode &gameMode,
                         DeveloperMode &developerMode,
                         OptimizationHistory &history,
                         SafetyGuard &safetyGuard,
                         QObject *parent)
    : QObject(parent),
      scanner_(scanner),
      telemetryCache_(telemetryCache),
      junkCleaner_(junkCleaner),
      startupOptimizer_(startupOptimizer),
      gameMode_(gameMode),
      developerMode_(developerMode),
      history_(history),
      safetyGuard_(safetyGuard),
      llmClient_(new LlmClient(this)) {
    connect(llmClient_, &LlmClient::responseReady, this, &AgentEngine::onLlmResponse);
    connect(llmClient_, &LlmClient::errorOccurred, this, &AgentEngine::onLlmError);
}

void AgentEngine::updateSnapshot(const SystemSnapshot &snapshot) {
    latestSnapshot_ = snapshot;
}

void AgentEngine::ask(const QString &message) {
    pendingMessage_ = message;
    latestSnapshot_ = telemetryCache_.latest();
    if (latestSnapshot_.summary.empty()) {
        latestSnapshot_ = scanner_.scan();
    }

    const ChatIntent intent = router_.route(message);
    const AgentDecision decision = router_.buildDecision(message, latestSnapshot_);
    pendingPlan_ = decision.plan;
    const auto historyWindow = telemetryCache_.history(24);

    switch (intent) {
    case ChatIntent::Clean: {
        const auto result = junkCleaner_.cleanSafeTargets();
        history_.record(ActionRecord {
            .timestampUtc = currentTimestampUtc(),
            .action = "ai-clean",
            .details = "Recovered " + std::to_string(result.bytesRecovered) + " bytes",
            .success = true,
        });
        emit agentReply(planSummary(pendingPlan_) + "\n\nCompleted cleanup. Removed " +
                            QString::number(result.filesRemoved) + " files and recovered " +
                            QString::fromStdString(formatBytes(result.bytesRecovered)) + ".",
                        true);
        return;
    }

    case ChatIntent::Optimize: {
        safetyGuard_.createRestorePoint(L"PulseBoost AI - optimization");
        const auto result = junkCleaner_.cleanSafeTargets();
        history_.record(ActionRecord {
            .timestampUtc = currentTimestampUtc(),
            .action = "ai-optimize",
            .details = "Created restore point and recovered " + std::to_string(result.bytesRecovered) + " bytes",
            .success = true,
        });

        const auto prompt = promptBuilder_.buildAgentPrompt(latestSnapshot_,
                                                            historyWindow,
                                                            pendingPlan_,
                                                            message.toStdString());
        llmClient_->sendPrompt("The plan already executed a restore point and safe junk cleanup.\nRecovered " +
                               QString::fromStdString(formatBytes(result.bytesRecovered)) + ".\n\n" +
                               QString::fromStdString(prompt));
        return;
    }

    case ChatIntent::GameMode: {
        for (const auto &process : latestSnapshot_.heavyProcesses) {
            if (process.isCritical || process.memoryMb < 512.0) {
                continue;
            }

            const bool ok = gameMode_.enableForProcess(process.pid);
            history_.record(ActionRecord {
                .timestampUtc = currentTimestampUtc(),
                .action = "ai-game-mode",
                .details = "Target PID " + std::to_string(process.pid),
                .success = ok,
            });
            emit agentReply(planSummary(pendingPlan_) + "\n\n" +
                                (ok ? "Game boost enabled for " + QString::fromStdString(process.name) + "."
                                    : "Game boost could not be enabled for " + QString::fromStdString(process.name) + "."),
                            ok);
            return;
        }

        emit agentReply("No obvious game process is active right now. Launch a game first, then ask again.", false);
        return;
    }

    case ChatIntent::DeveloperMode: {
        const auto actions = developerMode_.optimizeFor(message.toStdString());
        QString response = planSummary(pendingPlan_) + "\n\nDeveloper profile recommendations:\n";
        for (const auto &action : actions) {
            response += QString("- %1\n").arg(QString::fromStdString(action));
        }
        emit agentReply(response.trimmed(), false);
        return;
    }

    case ChatIntent::Startup: {
        const auto items = startupOptimizer_.scanStartupItems();
        emit agentReply(planSummary(pendingPlan_) + "\n\nDetected " + QString::number(items.size()) +
                            " startup entries. Review the highest-impact programs before disabling them.",
                        false);
        return;
    }

    case ChatIntent::StopHeavyProcesses: {
        emit agentReply(planSummary(pendingPlan_) +
                            "\n\nPulseBoost recommends closing the current top non-critical background apps from the Processes page before forcing termination automatically.",
                        false);
        return;
    }

    case ChatIntent::Analyze:
    case ChatIntent::FreeForm:
    default: {
        const auto prompt = promptBuilder_.buildAgentPrompt(latestSnapshot_,
                                                            historyWindow,
                                                            pendingPlan_,
                                                            message.toStdString());
        llmClient_->sendPrompt(QString::fromStdString(prompt));
        return;
    }
    }
}

void AgentEngine::onLlmResponse(QString response) {
    if (!pendingPlan_.empty() && !response.contains("PLAN", Qt::CaseInsensitive)) {
        response = planSummary(pendingPlan_) + "\n\n" + response;
    }
    emit agentReply(response, false);
}

void AgentEngine::onLlmError(QString error) {
    Q_UNUSED(error)
    emit agentReply(LlmClient::offlineFallback(pendingMessage_, latestSnapshot_, pendingPlan_), false);
}

}  // namespace pulseboost
