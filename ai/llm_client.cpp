#include "PulseBoostAI/ai/llm_client.hpp"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

namespace pulseboost {

LlmClient::LlmClient(QObject *parent) : QObject(parent) {
    QSettings settings;
    endpoint_ = settings.value("llm/endpoint", "http://127.0.0.1:11434/api/generate").toString();
    model_ = settings.value("llm/model", "llama3").toString();

    connect(&nam_, &QNetworkAccessManager::finished, this, &LlmClient::onReplyFinished);
}

void LlmClient::sendPrompt(const QString &prompt) {
    QJsonObject body;
    body["model"] = model_;
    body["prompt"] = prompt;
    body["stream"] = false;

    QNetworkRequest request{QUrl(endpoint_)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/json"));
    request.setTransferTimeout(20000);
    nam_.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void LlmClient::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    const auto document = QJsonDocument::fromJson(reply->readAll());
    if (!document.isObject()) {
        emit errorOccurred("Invalid JSON response from LLM");
        return;
    }

    const auto object = document.object();
    QString response = object.value("response").toString().trimmed();
    if (response.isEmpty()) {
        response = object.value("content").toString().trimmed();
    }

    if (response.isEmpty()) {
        emit errorOccurred("Empty response from LLM");
        return;
    }

    emit responseReady(response);
}

QString LlmClient::offlineFallback(const QString &prompt,
                                   const SystemSnapshot &snapshot,
                                   const std::vector<OptimizationPlanStep> &plan) {
    QStringList lines;
    lines << "PulseBoost AI is running without a reachable LLM endpoint.";

    if (snapshot.diskUsagePercent > 85.0) {
        lines << "The system drive is heavily utilized, so storage pressure is a likely cause of slowdowns.";
    }
    if (snapshot.ramUsagePercent > 80.0) {
        lines << "Memory usage is elevated and can contribute to app stalls and long task-switch delays.";
    }
    if (snapshot.startupPrograms > 10) {
        lines << "Startup load is high and will affect boot/login performance.";
    }
    if (!snapshot.heavyProcesses.empty()) {
        const auto &process = snapshot.heavyProcesses.front();
        lines << QString("The heaviest process right now is %1 at %2 MB.")
                     .arg(QString::fromStdString(process.name))
                     .arg(process.memoryMb, 0, 'f', 0);
    }

    if (!plan.empty()) {
        lines << "";
        lines << "PLAN";
        int index = 1;
        for (const auto &step : plan) {
            lines << QString("%1. %2").arg(index++).arg(QString::fromStdString(step.title));
        }
    }

    if (lines.size() == 1 && prompt.contains("slow", Qt::CaseInsensitive)) {
        lines << "No single bottleneck dominates right now, so focus on storage cleanup, startup reduction, and background app pressure first.";
    }

    lines << "";
    lines << "Configure a local Ollama endpoint in settings for full reasoning responses.";
    return lines.join('\n');
}

}  // namespace pulseboost
