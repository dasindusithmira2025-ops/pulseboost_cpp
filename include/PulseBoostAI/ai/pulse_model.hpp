#pragma once

#include <memory>

#include <QString>
#include <QStringList>
#include <QVector>

#include "PulseBoostAI/ai/model/adaptive_engine.hpp"
#include "PulseBoostAI/ai/model/context_fusion.hpp"
#include "PulseBoostAI/ai/model/intent_classifier.hpp"
#include "PulseBoostAI/ai/model/response_generator.hpp"
#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

struct PulseModelOutput {
    QString text;
    QString templateId;
    Intent intent = Intent::Unknown;
    float confidence = 0.0F;
    bool hasAction = false;
    QString actionId;
    QString actionLabel;
    QVector<QString> followUpSuggestions;
};

class PulseModel {
public:
    PulseModel();

    bool initialize(const QString &adaptiveDbPath);
    bool isReady() const;

    PulseModelOutput processInput(const QString &userInput,
                                  const QString &sessionId,
                                  const SystemSnapshot &snapshot,
                                  const QVector<SystemSnapshot> &history,
                                  const QVector<QString> &recentActions,
                                  const QVector<QString> &conversationSummary,
                                  int totalSessionMessages);

    void submitFeedback(const QString &sessionId, const QString &templateId, bool positive);
    void submitActionResult(const QString &sessionId, const QString &templateId, bool accepted);
    void recordFollowUp(const QString &sessionId, const QString &followUpText);
    bool resetLearning();

    int totalInteractions() const;
    float satisfactionRate() const;
    QHash<QString, float> templateWeights() const;
    QString modelVersion() const { return QStringLiteral("PulseModel v1.0"); }

private:
    std::unique_ptr<IntentClassifier> m_classifier;
    std::unique_ptr<ContextFusion> m_fusion;
    std::unique_ptr<ResponseGenerator> m_generator;
    std::unique_ptr<AdaptiveEngine> m_adaptive;
    bool m_ready = false;
};

}  // namespace pulseboost

