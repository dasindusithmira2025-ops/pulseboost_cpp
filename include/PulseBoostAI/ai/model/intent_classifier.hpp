#pragma once

#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QVector>

namespace pulseboost {

enum class Intent {
    Greeting,
    HowAreYou,
    ThankYou,
    Goodbye,
    WhyIsSlow,
    WhatsWrong,
    SystemStatus,
    CpuQuestion,
    RamQuestion,
    DiskQuestion,
    NetworkQuestion,
    TempQuestion,
    BatteryQuestion,
    ProcessQuestion,
    HealthQuestion,
    RunClean,
    RunOptimize,
    RunGameMode,
    RunDevMode,
    RunNetworkFix,
    RunStartupFix,
    RunRamFree,
    RunFullScan,
    KillProcess,
    CreateRestore,
    AskAboutFeature,
    AskAboutScore,
    AskAboutProcess,
    AskHistory,
    AskForecast,
    ChangeSettings,
    Unknown
};

struct ClassificationResult {
    Intent intent = Intent::Unknown;
    float confidence = 0.0F;
    QString extractedEntity;
    QMap<QString, QString> slotValues;
};

class IntentClassifier {
public:
    IntentClassifier();

    ClassificationResult classify(const QString &input) const;

private:
    struct IntentRule {
        Intent intent = Intent::Unknown;
        QVector<QString> keywords;
        QVector<QString> phrases;
        QVector<QRegularExpression> patterns;
        float baseConfidence = 0.5F;
    };

    QVector<IntentRule> m_rules;

    void buildRules();
    float scoreTfIdf(const QString &input, const IntentRule &rule) const;
    QString extractEntity(const QString &input, Intent intent) const;
    QMap<QString, QString> extractSlots(const QString &input, Intent intent) const;
};

}  // namespace pulseboost
