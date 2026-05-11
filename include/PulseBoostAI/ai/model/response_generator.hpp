#pragma once

#include <QHash>
#include <QString>
#include <QVector>

#include <functional>

#include "PulseBoostAI/ai/model/context_fusion.hpp"

namespace pulseboost {

struct ResponseTemplate {
    QString id;
    Intent intent = Intent::Unknown;
    QString templateText;
    std::function<bool(const FusedContext &)> condition;
    float weight = 1.0F;
    int useCount = 0;
    float successRate = 0.5F;
};

struct GeneratedResponse {
    QString text;
    QString templateId;
    bool hasAction = false;
    QString actionId;
    QString actionLabel;
    QVector<QString> followUpSuggestions;
};

class ResponseGenerator {
public:
    ResponseGenerator();

    GeneratedResponse generate(const FusedContext &ctx, const QHash<QString, float> &externalWeights = {});

private:
    QVector<ResponseTemplate> m_templates;

    void buildTemplates();
    QString fillSlots(const QString &templateText, const FusedContext &ctx) const;
    ResponseTemplate *selectBestTemplate(const QVector<ResponseTemplate *> &candidates,
                                         const QHash<QString, float> &externalWeights);
    GeneratedResponse fallbackUnknown(const FusedContext &ctx) const;
    void decorateAction(GeneratedResponse &response, const FusedContext &ctx) const;
};

}  // namespace pulseboost

