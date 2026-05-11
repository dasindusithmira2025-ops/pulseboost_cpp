#pragma once

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVector>

#include <QtSql/QSqlDatabase>

#include "PulseBoostAI/ai/model/intent_classifier.hpp"

namespace pulseboost {

struct InteractionRecord {
    QString sessionId;
    QDateTime timestamp;
    QString userInput;
    Intent detectedIntent = Intent::Unknown;
    float intentConfidence = 0.0F;
    QString templateUsed;
    bool actionOffered = false;
    bool actionAccepted = false;
    int feedbackScore = 0;
    double cpuAtTime = 0.0;
    double ramAtTime = 0.0;
    double healthAtTime = 0.0;
    QString followUpQuestion;
};

class AdaptiveEngine {
public:
    explicit AdaptiveEngine(const QString &dbPath);
    ~AdaptiveEngine();

    bool isReady() const;

    void recordInteraction(const InteractionRecord &record);
    void recordThumbsUp(const QString &sessionId, const QString &templateId);
    void recordThumbsDown(const QString &sessionId, const QString &templateId);
    void recordActionAccepted(const QString &sessionId, const QString &templateId);
    void recordActionDeclined(const QString &sessionId, const QString &templateId);
    void recordFollowUp(const QString &sessionId, const QString &followUpText);

    float getTemplateWeight(const QString &templateId) const;
    QHash<QString, float> getAllWeights() const;

    QString getMostEffectiveTemplateForIntent(Intent intent) const;
    bool userPrefersVerboseResponses() const;
    bool userPrefersActionSuggestions() const;
    QString getUserPrimaryIssuePattern() const;

    int getTotalInteractions() const;
    float getOverallSatisfactionRate() const;
    QVector<QString> getFrequentlyAskedTopics() const;

    bool resetLearnedData();

private:
    void initializeDatabase();
    void updateTemplateWeight(const QString &templateId, float delta);
    void incrementCounter(const QString &templateId, const QString &column);
    QString ensureConnectionName() const;

    static constexpr float WEIGHT_FLOOR = 0.1F;
    static constexpr float WEIGHT_CEILING = 2.0F;
    static constexpr float DECAY_RATE = 0.999F;

    QString m_dbPath;
    QString m_connectionName;
    QSqlDatabase m_db;
    bool m_ready = false;
};

}  // namespace pulseboost

