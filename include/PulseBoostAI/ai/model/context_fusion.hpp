#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

#include "PulseBoostAI/ai/model/intent_classifier.hpp"
#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

struct FusedContext {
    double cpuPct = 0.0;
    double ramPct = 0.0;
    double diskPct = 0.0;
    double networkMbps = 0.0;
    double healthScore = 0.0;
    double cpuTempC = 0.0;
    QString topCpuProcess;
    QString topRamProcess;
    int processCount = 0;
    bool isRamPressured = false;
    bool isCpuThrottling = false;
    bool isDiskSaturated = false;

    bool cpuTrending = false;
    bool ramTrending = false;
    bool healthTrending = false;
    double cpuDelta = 0.0;
    double ramDelta = 0.0;
    double healthDelta = 0.0;

    QVector<QString> recentActions;
    QVector<QString> conversationSummary;
    int totalSessionMessages = 0;

    QString currentUsageMode;
    bool isTypicalUsageTime = false;
    QString mostCommonIssue;

    ClassificationResult intent;
    QString rawInput;

    QString healthGrade;
    QStringList activeAlerts;
    QStringList recentOptimizations;
    bool gameRunning = false;
    bool devToolsRunning = false;
    QString primaryIssue;
};

class ContextFusion {
public:
    FusedContext fuse(const QString &userInput,
                      const ClassificationResult &intent,
                      const SystemSnapshot &snap,
                      const QVector<SystemSnapshot> &history,
                      const QVector<QString> &recentActions,
                      const QVector<QString> &conversationSummary,
                      int totalSessionMessages) const;

private:
    QString assessPrimaryIssue(const SystemSnapshot &snap) const;
    QString detectUsageMode(const SystemSnapshot &snap) const;
    bool detectGameRunning(const SystemSnapshot &snap) const;
    bool detectDevToolsRunning(const SystemSnapshot &snap) const;
    QString healthGradeLabel(double healthScore) const;
};

}  // namespace pulseboost

