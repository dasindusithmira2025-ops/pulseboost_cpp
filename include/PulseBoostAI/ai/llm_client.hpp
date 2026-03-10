#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QUrl>

#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

/// Async LLM client using QNetworkAccessManager.
/// Targets a local Ollama instance (http://127.0.0.1:11434/api/generate)
/// by default; configurable via QSettings key "llm/endpoint" and "llm/model".
class LlmClient : public QObject {
    Q_OBJECT
public:
    explicit LlmClient(QObject *parent = nullptr);

    /// Sends a prompt to the configured LLM endpoint.
    /// emits responseReady() when the reply arrives (or errorOccurred() on failure).
    void sendPrompt(const QString &prompt);

    /// Quick synchronous fallback text when no LLM is reachable.
    static QString offlineFallback(const QString &prompt,
                                   const SystemSnapshot &snapshot,
                                   const std::vector<OptimizationPlanStep> &plan);

signals:
    void responseReady(QString response);
    void errorOccurred(QString error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager nam_;
    QString               endpoint_;
    QString               model_;
};

}  // namespace pulseboost
