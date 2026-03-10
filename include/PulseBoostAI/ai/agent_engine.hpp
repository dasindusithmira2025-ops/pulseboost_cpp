#pragma once

#include <QObject>
#include <QString>

#include "PulseBoostAI/ai/chat_router.hpp"
#include "PulseBoostAI/ai/llm_client.hpp"
#include "PulseBoostAI/ai/prompt_builder.hpp"
#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"
#include "PulseBoostAI/data/telemetry_cache.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/modules/safety_guard.hpp"
#include "PulseBoostAI/modules/developer_mode.hpp"
#include "PulseBoostAI/modules/game_mode.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"

namespace pulseboost {

/// Top-level AI orchestrator exposed to QML.
/// ask() is non-blocking: result arrives via agentReply() signal.
class AgentEngine : public QObject {
    Q_OBJECT
public:
    explicit AgentEngine(SystemScanner      &scanner,
                         TelemetryCache     &telemetryCache,
                         JunkCleaner        &junkCleaner,
                         StartupOptimizer   &startupOptimizer,
                         GameMode           &gameMode,
                          DeveloperMode      &developerMode,
                         OptimizationHistory &history,
                         SafetyGuard        &safetyGuard,
                         QObject            *parent = nullptr);

    /// Called from QML – non-blocking.
    Q_INVOKABLE void ask(const QString &message);

    /// Update the cached snapshot so prompts contain fresh data.
    void updateSnapshot(const pulseboost::SystemSnapshot &snapshot);

signals:
    /// Emitted when the AI has a response.
    /// @param reply    The text to display in the chat panel.
    /// @param actionTaken  true if a module was executed (e.g. junk clean).
    void agentReply(QString reply, bool actionTaken);

private slots:
    void onLlmResponse(QString response);
    void onLlmError(QString error);

private:
    SystemScanner       &scanner_;
    TelemetryCache      &telemetryCache_;
    JunkCleaner         &junkCleaner_;
    StartupOptimizer    &startupOptimizer_;
    GameMode            &gameMode_;
    DeveloperMode       &developerMode_;
    OptimizationHistory &history_;
    SafetyGuard         &safetyGuard_;

    ChatRouter          router_;
    PromptBuilder       promptBuilder_;
    LlmClient           *llmClient_;

    pulseboost::SystemSnapshot latestSnapshot_;
    QString             pendingMessage_;
    std::vector<OptimizationPlanStep> pendingPlan_;
};

}  // namespace pulseboost
