#include "PulseBoostAI/ai/agent_engine.hpp"

#include <Windows.h>

#include <QDateTime>
#include <QDebug>

#include <algorithm>

#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/modules/safety_policy.hpp"

namespace pulseboost {

AgentEngine::AgentEngine(SystemScanner &scanner,
                         TelemetryCache &telemetryCache,
                         JunkCleaner &junkCleaner,
                         ProcessManager &processManager,
                         StartupOptimizer &startupOptimizer,
                         GameMode &gameMode,
                         DeveloperMode &developerMode,
                         OptimizationHistory &history,
                         SafetyGuard &safetyGuard,
                         NetworkOptimizer &networkOptimizer,
                         QObject *parent)
    : QObject(parent),
      scanner_(scanner),
      telemetryCache_(telemetryCache),
      junkCleaner_(junkCleaner),
      processManager_(processManager),
      startupOptimizer_(startupOptimizer),
      gameMode_(gameMode),
      developerMode_(developerMode),
      history_(history),
      safetyGuard_(safetyGuard),
      networkOptimizer_(networkOptimizer),
      pulseModel_() {
    const bool initialized = pulseModel_.initialize("data/pulsemodel_adaptive.sqlite");
    if (!initialized) {
        qWarning() << "[PulseModel] failed to initialize adaptive database";
    }

    streamTimer_.setSingleShot(false);
    streamTimer_.setInterval(24);
    connect(&streamTimer_, &QTimer::timeout, this, [this]() {
        if (streamIndex_ >= streamWords_.size()) {
            streamTimer_.stop();
            emit agentReply(streamAccumulated_, streamActionTaken_);
            return;
        }

        if (!streamAccumulated_.isEmpty()) {
            streamAccumulated_.append(' ');
        }
        streamAccumulated_.append(streamWords_.at(streamIndex_));
        streamIndex_ += 1;

        emit agentReplyChunk(streamWords_.at(streamIndex_ - 1) + (streamIndex_ < streamWords_.size() ? " " : ""));

        const QString justAdded = streamWords_.at(streamIndex_ - 1);
        if (justAdded.endsWith('.') || justAdded.endsWith('!') || justAdded.endsWith('?')) {
            streamTimer_.setInterval(160);
        } else if (justAdded.endsWith(',') || justAdded.endsWith(':')) {
            streamTimer_.setInterval(80);
        } else {
            streamTimer_.setInterval(24);
        }
    });
}

QString AgentEngine::sanitizeErrorForUi(const QString &rawError) const {
    qWarning() << "[PulseModel Error]" << rawError;
    return {};
}

void AgentEngine::updateSnapshot(const SystemSnapshot &snapshot) {
    latestSnapshot_ = snapshot;
}

QString AgentEngine::currentSessionId() const {
    return QStringLiteral("local-session");
}

QString AgentEngine::formatPlan(const std::vector<OptimizationPlanStep> &plan) const {
    if (plan.empty()) {
        return {};
    }
    QString output = "Plan:\n";
    int index = 1;
    for (const auto &step : plan) {
        output += QString("%1. %2").arg(index++).arg(QString::fromStdString(step.title));
        if (!step.details.empty()) {
            output += " - " + QString::fromStdString(step.details);
        }
        output += '\n';
    }
    return output.trimmed();
}

bool AgentEngine::executeActionByName(const QString &actionName, QStringList &executionNotes) {
    const QString normalized = actionName.trimmed().toLower();
    if (normalized.isEmpty() || normalized == "none") {
        return false;
    }

    auto advisoryOnly = [&executionNotes](const QString &label, const QString &reason) {
        executionNotes.push_back(label + " was prepared as advisory-only. " + reason);
        return false;
    };

    if (normalized == "create_restore_point") {
        const bool ok = safetyGuard_.createRestorePoint(L"PulseBoost AI");
        history_.record(ActionRecord {currentTimestampUtc(), "ai-restore-point", "Requested restore point", ok});
        executionNotes.push_back(ok ? "Created a restore point." : "Restore point creation failed.");
        return ok;
    }

    if (normalized == "clean_junk") {
        const auto result = junkCleaner_.cleanSafeTargets();
        history_.record(ActionRecord {currentTimestampUtc(), "ai-clean", "Recovered " + std::to_string(result.bytesRecovered) + " bytes", true});
        executionNotes.push_back("Cleaned junk and recovered " + QString::fromStdString(formatBytes(result.bytesRecovered)) + ".");
        return true;
    }

    if (normalized == "enable_game_mode") {
        return advisoryOnly(QStringLiteral("Game mode"), QStringLiteral("Manual confirmation is required before changing process priority or power state."));
    }

    if (normalized == "optimize_developer_mode") {
        const auto actions = developerMode_.optimizeFor("developer profile");
        history_.record(ActionRecord {currentTimestampUtc(), "ai-dev-mode", "Applied developer mode profile", true});
        executionNotes.push_back("Applied developer mode profile.");
        for (const auto &action : actions) {
            executionNotes.push_back(QString::fromStdString(action));
        }
        return true;
    }

    if (normalized == "analyze_startup") {
        const auto items = startupOptimizer_.scanStartupItems();
        history_.record(ActionRecord {currentTimestampUtc(), "ai-startup-audit", "Startup items: " + std::to_string(items.size()), true});
        executionNotes.push_back(QString("Audited %1 startup items.").arg(items.size()));
        return true;
    }

    if (normalized == "optimize_ram") {
        return advisoryOnly(QStringLiteral("RAM trim"), QStringLiteral("Working-set trimming is advanced/manual and is not a guaranteed performance boost."));
    }

    if (normalized == "optimize_disk") {
        return advisoryOnly(QStringLiteral("Disk optimization"), QStringLiteral("The Action Center must capture proof and confirmation before execution."));
    }

    if (normalized == "optimize_network") {
        return advisoryOnly(QStringLiteral("Network optimization"), QStringLiteral("Global netsh/TCP changes require backup plus explicit advanced confirmation."));
    }

    if (normalized == "full_scan") {
        latestSnapshot_ = scanner_.scan();
        history_.record(ActionRecord {currentTimestampUtc(), "ai-full-scan", "Performed full system scan", true});
        executionNotes.push_back("Completed full system scan.");
        return true;
    }

    if (normalized == "optimize_all") {
        executionNotes.push_back("Prepared a full optimization plan. Run dry-runs in the Action Center before applying individual actions.");
        return false;
    }

    if (normalized.startsWith("kill_process:")) {
        return advisoryOnly(QStringLiteral("Process termination"), QStringLiteral("AI can explain risk, but process termination must be user-confirmed outside chat."));
    }

    executionNotes.push_back("Unknown action: " + actionName);
    return false;
}

void AgentEngine::streamReply(const QString &text, bool actionTaken) {
    streamTimer_.stop();
    streamWords_ = text.split(' ', Qt::SkipEmptyParts);
    streamAccumulated_.clear();
    streamIndex_ = 0;
    streamActionTaken_ = actionTaken;

    if (streamWords_.isEmpty()) {
        emit agentReply(text, actionTaken);
        return;
    }

    streamTimer_.setInterval(24);
    streamTimer_.start();
}

void AgentEngine::ask(const QString &message) {
    pendingMessage_ = message.trimmed();
    if (pendingMessage_.isEmpty()) {
        return;
    }

    memory_.addMessage("user", pendingMessage_);
    executionNotes_.clear();
    pendingPlan_.clear();
    actionTaken_ = false;

    latestSnapshot_ = telemetryCache_.latest();
    if (latestSnapshot_.summary.empty()) {
        latestSnapshot_ = scanner_.scan();
    }

    const AgentDecision decision = router_.buildDecision(pendingMessage_, latestSnapshot_);
    pendingPlan_ = decision.plan;

    if (decision.shouldExecutePlan) {
        executionNotes_.push_back("AI prepared the requested plan as advisory-first. Use the Action Center dry-run buttons before applying changes.");
        for (const auto &step : pendingPlan_) {
            if (!step.execute) {
                continue;
            }

            QString actionName;
            const QString title = QString::fromStdString(step.title).toLower();
            if (title.contains("restore point")) {
                actionName = "create_restore_point";
            } else if (title.contains("clean") || title.contains("recover storage")) {
                actionName = "clean_junk";
            } else if (title.contains("game mode")) {
                actionName = "enable_game_mode";
            } else if (title.contains("developer")) {
                actionName = "optimize_developer_mode";
            } else if (title.contains("startup")) {
                actionName = "analyze_startup";
            } else if (title.contains("working set") || title.contains("ram")) {
                actionName = "optimize_ram";
            } else if (title.contains("drive layout") || title.contains("disk")) {
                actionName = "optimize_disk";
            } else if (title.contains("network")) {
                actionName = "optimize_network";
            }

            if (!actionName.isEmpty()) {
                actionTaken_ = executeActionByName(actionName, executionNotes_) || actionTaken_;
            }
        }
    }

    QVector<SystemSnapshot> historyVec;
    const auto history = telemetryCache_.history(60);
    historyVec.reserve(static_cast<int>(history.size()));
    for (const auto &sample : history) {
        historyVec.push_back(sample);
    }

    QVector<QString> recentActions;
    recentActions.reserve(executionNotes_.size());
    for (const QString &note : executionNotes_) {
        recentActions.push_back(note);
    }

    QVector<QString> conversationSummary;
    const QList<ChatMessage> historyMessages = memory_.getHistory();
    const int historySize = static_cast<int>(historyMessages.size());
    const int startIndex = std::max(0, historySize - 3);
    for (int index = startIndex; index < historySize; ++index) {
        conversationSummary.push_back(historyMessages.at(index).content.left(72));
    }

    const QString sessionId = currentSessionId();
    const PulseModelOutput modelOutput = pulseModel_.processInput(pendingMessage_,
                                                                  sessionId,
                                                                  latestSnapshot_,
                                                                  historyVec,
                                                                  recentActions,
                                                                  conversationSummary,
                                                                  historyMessages.size());

    lastTemplateId_ = modelOutput.templateId;
    lastActionId_ = modelOutput.actionId;
    lastActionLabel_ = modelOutput.actionLabel;
    emit agentActionMetadata(sessionId, lastTemplateId_, lastActionId_, lastActionLabel_);

    QString reply = modelOutput.text;
    if (!executionNotes_.isEmpty()) {
        reply += "\n\nActions completed:\n- " + executionNotes_.join("\n- ");
    } else if (!pendingPlan_.empty()) {
        reply += "\n\n" + formatPlan(pendingPlan_);
    }

    memory_.addMessage("model", reply);
    streamReply(reply, actionTaken_);
}

void AgentEngine::submitFeedback(const QString &sessionId, const QString &templateId, bool positive) {
    pulseModel_.submitFeedback(sessionId, templateId, positive);
}

void AgentEngine::submitActionResult(const QString &sessionId, const QString &templateId, bool accepted) {
    pulseModel_.submitActionResult(sessionId, templateId, accepted);
}

void AgentEngine::executeAction(const QString &actionId) {
    QStringList notes;
    const bool success = executeActionByName(actionId, notes);
    pulseModel_.submitActionResult(currentSessionId(), lastTemplateId_, success);
    const QString reply = success
                              ? QString("Action complete.\n- %1").arg(notes.join("\n- "))
                              : QString("Action failed or blocked.\n- %1").arg(notes.join("\n- "));
    streamReply(reply, success);
}

int AgentEngine::totalInteractions() const {
    return pulseModel_.totalInteractions();
}

double AgentEngine::satisfactionRate() const {
    return pulseModel_.satisfactionRate();
}

QString AgentEngine::modelLabel() const {
    return pulseModel_.modelVersion() + " - Local AI Engine";
}

bool AgentEngine::resetAdaptiveLearning() {
    return pulseModel_.resetLearning();
}

}  // namespace pulseboost

