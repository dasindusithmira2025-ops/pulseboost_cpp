#pragma once

#include <QObject>
#include <QString>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

enum class ChatIntent {
    Clean,
    Optimize,
    GameMode,
    DeveloperMode,
    Startup,
    StopHeavyProcesses,
    Analyze,
    FreeForm
};

/// Classifies the user's message into an intent category.
class ChatRouter : public QObject {
    Q_OBJECT
public:
    explicit ChatRouter(QObject *parent = nullptr);

    ChatIntent route(const QString &message) const;
    AgentDecision buildDecision(const QString &message, const SystemSnapshot &snapshot) const;
};

}  // namespace pulseboost
