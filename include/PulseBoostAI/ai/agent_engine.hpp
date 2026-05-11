#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

#include "PulseBoostAI/ai/chat_router.hpp"
#include "PulseBoostAI/ai/pulse_model.hpp"
#include "PulseBoostAI/ai/session_memory.hpp"
#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/data/telemetry_cache.hpp"
#include "PulseBoostAI/modules/defrag_trigger.hpp"
#include "PulseBoostAI/modules/developer_mode.hpp"
#include "PulseBoostAI/modules/game_mode.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"
#include "PulseBoostAI/modules/network_optimizer.hpp"
#include "PulseBoostAI/modules/ram_optimizer.hpp"
#include "PulseBoostAI/modules/safety_guard.hpp"

namespace pulseboost {

class AgentEngine : public QObject {
    Q_OBJECT
public:
    explicit AgentEngine(SystemScanner &scanner,
                         TelemetryCache &telemetryCache,
                         JunkCleaner &junkCleaner,
                         ProcessManager &processManager,
                         StartupOptimizer &startupOptimizer,
                         GameMode &gameMode,
                         DeveloperMode &developerMode,
                         OptimizationHistory &history,
                         SafetyGuard &safetyGuard,
                         NetworkOptimizer &networkOptimizer,
                         QObject *parent = nullptr);

    Q_INVOKABLE void ask(const QString &message);
    Q_INVOKABLE void submitFeedback(const QString &sessionId, const QString &templateId, bool positive);
    Q_INVOKABLE void submitActionResult(const QString &sessionId, const QString &templateId, bool accepted);
    Q_INVOKABLE void executeAction(const QString &actionId);
    Q_INVOKABLE int totalInteractions() const;
    Q_INVOKABLE double satisfactionRate() const;
    Q_INVOKABLE QString modelLabel() const;
    Q_INVOKABLE bool resetAdaptiveLearning();

    void updateSnapshot(const pulseboost::SystemSnapshot &snapshot);

signals:
    void agentReplyChunk(QString chunk);
    void agentReply(QString reply, bool actionTaken);
    void agentActionMetadata(QString sessionId, QString templateId, QString actionId, QString actionLabel);

private:
    QString sanitizeErrorForUi(const QString &rawError) const;
    QString formatPlan(const std::vector<OptimizationPlanStep> &plan) const;
    bool executeActionByName(const QString &actionName, QStringList &executionNotes);
    void streamReply(const QString &text, bool actionTaken);
    QString currentSessionId() const;

    SystemScanner &scanner_;
    TelemetryCache &telemetryCache_;
    JunkCleaner &junkCleaner_;
    ProcessManager &processManager_;
    StartupOptimizer &startupOptimizer_;
    GameMode &gameMode_;
    DeveloperMode &developerMode_;
    OptimizationHistory &history_;
    SafetyGuard &safetyGuard_;
    NetworkOptimizer &networkOptimizer_;
    DefragTrigger defragTrigger_;

    ChatRouter router_;
    SessionMemory memory_;
    PulseModel pulseModel_;

    pulseboost::SystemSnapshot latestSnapshot_;
    QString pendingMessage_;
    QStringList executionNotes_;
    std::vector<OptimizationPlanStep> pendingPlan_;
    QString lastTemplateId_;
    QString lastActionId_;
    QString lastActionLabel_;
    bool actionTaken_ = false;

    QTimer streamTimer_;
    QStringList streamWords_;
    QString streamAccumulated_;
    int streamIndex_ = 0;
    bool streamActionTaken_ = false;
};

}  // namespace pulseboost

