#include "PulseBoostAI/ai/pulse_model.hpp"

namespace pulseboost {

PulseModel::PulseModel() = default;

bool PulseModel::initialize(const QString &adaptiveDbPath) {
    m_classifier = std::make_unique<IntentClassifier>();
    m_fusion = std::make_unique<ContextFusion>();
    m_generator = std::make_unique<ResponseGenerator>();
    m_adaptive = std::make_unique<AdaptiveEngine>(adaptiveDbPath);
    m_ready = m_adaptive->isReady();
    return m_ready;
}

bool PulseModel::isReady() const {
    return m_ready;
}

PulseModelOutput PulseModel::processInput(const QString &userInput,
                                          const QString &sessionId,
                                          const SystemSnapshot &snapshot,
                                          const QVector<SystemSnapshot> &history,
                                          const QVector<QString> &recentActions,
                                          const QVector<QString> &conversationSummary,
                                          int totalSessionMessages) {
    PulseModelOutput output;
    if (!m_ready) {
        output.text = "PulseModel is not ready.";
        return output;
    }

    const ClassificationResult classification = m_classifier->classify(userInput);
    const FusedContext context = m_fusion->fuse(userInput,
                                                classification,
                                                snapshot,
                                                history,
                                                recentActions,
                                                conversationSummary,
                                                totalSessionMessages);
    const GeneratedResponse generated = m_generator->generate(context, m_adaptive->getAllWeights());

    output.text = generated.text;
    output.templateId = generated.templateId;
    output.intent = classification.intent;
    output.confidence = classification.confidence;
    output.hasAction = generated.hasAction;
    output.actionId = generated.actionId;
    output.actionLabel = generated.actionLabel;
    output.followUpSuggestions = generated.followUpSuggestions;

    InteractionRecord record;
    record.sessionId = sessionId;
    record.timestamp = QDateTime::currentDateTimeUtc();
    record.userInput = userInput;
    record.detectedIntent = classification.intent;
    record.intentConfidence = classification.confidence;
    record.templateUsed = generated.templateId;
    record.actionOffered = generated.hasAction;
    record.cpuAtTime = snapshot.cpuUsagePercent;
    record.ramAtTime = snapshot.ramUsagePercent;
    record.healthAtTime = snapshot.healthScore;
    m_adaptive->recordInteraction(record);

    return output;
}

void PulseModel::submitFeedback(const QString &sessionId, const QString &templateId, bool positive) {
    if (!m_ready || templateId.isEmpty()) {
        return;
    }
    if (positive) {
        m_adaptive->recordThumbsUp(sessionId, templateId);
    } else {
        m_adaptive->recordThumbsDown(sessionId, templateId);
    }
}

void PulseModel::submitActionResult(const QString &sessionId, const QString &templateId, bool accepted) {
    if (!m_ready || templateId.isEmpty()) {
        return;
    }
    if (accepted) {
        m_adaptive->recordActionAccepted(sessionId, templateId);
    } else {
        m_adaptive->recordActionDeclined(sessionId, templateId);
    }
}

void PulseModel::recordFollowUp(const QString &sessionId, const QString &followUpText) {
    if (!m_ready) {
        return;
    }
    m_adaptive->recordFollowUp(sessionId, followUpText);
}

bool PulseModel::resetLearning() {
    if (!m_ready) {
        return false;
    }
    return m_adaptive->resetLearnedData();
}

int PulseModel::totalInteractions() const {
    if (!m_ready) {
        return 0;
    }
    return m_adaptive->getTotalInteractions();
}

float PulseModel::satisfactionRate() const {
    if (!m_ready) {
        return 0.0F;
    }
    return m_adaptive->getOverallSatisfactionRate();
}

QHash<QString, float> PulseModel::templateWeights() const {
    if (!m_ready) {
        return {};
    }
    return m_adaptive->getAllWeights();
}

}  // namespace pulseboost

