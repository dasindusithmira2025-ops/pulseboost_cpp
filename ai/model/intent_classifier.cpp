#include "PulseBoostAI/ai/model/intent_classifier.hpp"

#include <QStringList>
#include <algorithm>

namespace pulseboost {

IntentClassifier::IntentClassifier() {
    buildRules();
}

void IntentClassifier::buildRules() {
    auto addRule = [this](Intent intent,
                          QVector<QString> keywords,
                          QVector<QString> phrases,
                          QVector<QRegularExpression> patterns,
                          float baseConfidence) {
        IntentRule rule;
        rule.intent = intent;
        rule.keywords = std::move(keywords);
        rule.phrases = std::move(phrases);
        rule.patterns = std::move(patterns);
        rule.baseConfidence = baseConfidence;
        m_rules.push_back(std::move(rule));
    };

    addRule(Intent::Greeting,
            {"hi", "hello", "hey", "yo", "sup", "howdy"},
            {"hi there", "hello there", "hey there"},
            {QRegularExpression("^(hi|hello|hey|yo|sup)\\b", QRegularExpression::CaseInsensitiveOption)},
            0.95F);

    addRule(Intent::HowAreYou,
            {"how are you", "whats up", "what's up", "hows it going", "how's it going"},
            {"how are you", "what's up", "hows it going"},
            {QRegularExpression("how\\s+are\\s+you", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::ThankYou,
            {"thanks", "thank", "appreciate", "awesome", "perfect", "great"},
            {"thank you", "thanks a lot", "that worked"},
            {},
            0.88F);

    addRule(Intent::Goodbye,
            {"bye", "goodbye", "exit", "close", "quit"},
            {"see you", "talk later"},
            {QRegularExpression("^(bye|goodbye|exit|quit)\\b", QRegularExpression::CaseInsensitiveOption)},
            0.93F);

    addRule(Intent::WhyIsSlow,
            {"slow", "lag", "lagging", "sluggish", "stutter", "freezing", "choppy"},
            {"why is my pc slow", "pc is slow", "computer is slow"},
            {QRegularExpression("why.*(slow|lag|sluggish)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::WhatsWrong,
            {"what's wrong", "whats wrong", "issues", "problem", "broken"},
            {"any issues", "what is wrong"},
            {QRegularExpression("(what.?s\\s+wrong|any\\s+issues)", QRegularExpression::CaseInsensitiveOption)},
            0.85F);

    addRule(Intent::SystemStatus,
            {"status", "overview", "summary", "report", "health"},
            {"system status", "how is my pc", "system overview"},
            {QRegularExpression("(system|pc|computer).*(status|health|summary)", QRegularExpression::CaseInsensitiveOption)},
            0.88F);

    addRule(Intent::CpuQuestion,
            {"cpu", "processor", "core", "high cpu"},
            {"cpu usage", "cpu high", "cpu 100"},
            {QRegularExpression("(cpu|processor).*(high|usage|%)", QRegularExpression::CaseInsensitiveOption)},
            0.92F);

    addRule(Intent::RamQuestion,
            {"ram", "memory", "mem", "out of memory"},
            {"ram usage", "memory usage", "ram full", "memory pressure"},
            {QRegularExpression("(ram|memory).*(high|full|usage|pressure|%)", QRegularExpression::CaseInsensitiveOption)},
            0.92F);

    addRule(Intent::DiskQuestion,
            {"disk", "storage", "drive", "space", "ssd", "hdd"},
            {"disk full", "storage issue", "drive full"},
            {QRegularExpression("(disk|drive|storage).*(full|slow|usage|space|%)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::NetworkQuestion,
            {"network", "internet", "wifi", "latency", "ping", "dns"},
            {"internet slow", "network issue", "high ping"},
            {QRegularExpression("(internet|network|wifi|ping|latency|dns)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::TempQuestion,
            {"hot", "temperature", "thermal", "overheat", "overheating", "throttle"},
            {"pc is hot", "getting hot", "cpu temperature"},
            {QRegularExpression("(hot|temp|thermal|overheat|throttl)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::BatteryQuestion,
            {"battery", "charging", "power", "laptop battery"},
            {"battery life", "battery health"},
            {QRegularExpression("battery", QRegularExpression::CaseInsensitiveOption)},
            0.86F);

    addRule(Intent::ProcessQuestion,
            {"process", "task", "running app", "resource hog"},
            {"top process", "which process"},
            {QRegularExpression("(process|task).*(using|high|what|which)", QRegularExpression::CaseInsensitiveOption)},
            0.87F);

    addRule(Intent::HealthQuestion,
            {"health score", "health", "score"},
            {"what is my health score", "health score"},
            {QRegularExpression("health\\s+score", QRegularExpression::CaseInsensitiveOption)},
            0.88F);

    addRule(Intent::RunClean,
            {"clean", "cleanup", "junk", "temp", "free space", "remove junk"},
            {"clean my pc", "clean my system", "clear junk", "remove temp files"},
            {QRegularExpression("(clean|clear|remove).*(junk|temp|cache|space)", QRegularExpression::CaseInsensitiveOption)},
            0.93F);

    addRule(Intent::RunOptimize,
            {"optimize", "boost", "speed up", "tune"},
            {"optimize my pc", "optimize system", "speed up my pc"},
            {QRegularExpression("(optimize|boost|speed\\s*up)", QRegularExpression::CaseInsensitiveOption)},
            0.92F);

    addRule(Intent::RunGameMode,
            {"game mode", "gaming", "fps", "game boost"},
            {"enable game mode", "activate game mode"},
            {QRegularExpression("(enable|activate|start).*(game|gaming)", QRegularExpression::CaseInsensitiveOption)},
            0.94F);

    addRule(Intent::RunDevMode,
            {"developer mode", "dev mode", "build optimize", "coding mode"},
            {"enable dev mode", "developer mode"},
            {QRegularExpression("(dev(eloper)?\\s+mode|coding\\s+mode)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::RunNetworkFix,
            {"fix internet", "network optimize", "flush dns", "optimize tcp"},
            {"fix my internet", "optimize network"},
            {QRegularExpression("(fix|optimize).*(internet|network|dns|tcp|wifi)", QRegularExpression::CaseInsensitiveOption)},
            0.91F);

    addRule(Intent::RunStartupFix,
            {"startup", "boot", "slow boot", "startup optimize"},
            {"fix startup", "optimize startup", "startup manager"},
            {QRegularExpression("(startup|boot).*(fix|optimi|slow|delay)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::RunRamFree,
            {"free ram", "clear memory", "release memory", "trim ram"},
            {"free up ram", "optimize memory"},
            {QRegularExpression("(free|clear|release|optimize).*(ram|memory)", QRegularExpression::CaseInsensitiveOption)},
            0.93F);

    addRule(Intent::RunFullScan,
            {"full scan", "scan all", "scan everything", "deep scan"},
            {"run full scan", "scan my pc"},
            {QRegularExpression("(full|deep).*(scan)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::KillProcess,
            {"kill", "terminate", "close process", "end task", "stop process"},
            {"kill process", "terminate process", "close chrome"},
            {QRegularExpression("(kill|close|terminate|stop|end)\\s+([a-z0-9_\\-\\.]+)", QRegularExpression::CaseInsensitiveOption)},
            0.92F);

    addRule(Intent::CreateRestore,
            {"restore point", "backup registry", "create restore"},
            {"create restore point", "make restore point"},
            {QRegularExpression("(create|make).*(restore\\s+point|backup)", QRegularExpression::CaseInsensitiveOption)},
            0.9F);

    addRule(Intent::AskAboutFeature,
            {"how do i", "what does", "feature", "how to"},
            {"how do i use", "what does this do"},
            {QRegularExpression("(how\\s+do\\s+i|what\\s+does)", QRegularExpression::CaseInsensitiveOption)},
            0.82F);

    addRule(Intent::AskAboutScore,
            {"explain health score", "score meaning", "score calculation"},
            {"what does health score mean", "explain score"},
            {QRegularExpression("(explain|meaning).*(health\\s+score|score)", QRegularExpression::CaseInsensitiveOption)},
            0.87F);

    addRule(Intent::AskAboutProcess,
            {"what is process", "is this process safe", "process info"},
            {"what is chrome.exe", "is process safe"},
            {QRegularExpression("(what\\s+is|is).*(\\.exe|process)", QRegularExpression::CaseInsensitiveOption)},
            0.83F);

    addRule(Intent::AskHistory,
            {"history", "past actions", "recent optimizations", "log"},
            {"show my history", "action history"},
            {QRegularExpression("(history|past\\s+actions|recent\\s+actions|log)", QRegularExpression::CaseInsensitiveOption)},
            0.86F);

    addRule(Intent::AskForecast,
            {"forecast", "predict", "future", "when will"},
            {"health forecast", "predict issues"},
            {QRegularExpression("(forecast|predict|when\\s+will|how\\s+long)", QRegularExpression::CaseInsensitiveOption)},
            0.87F);

    addRule(Intent::ChangeSettings,
            {"settings", "configure", "change config", "preferences"},
            {"open settings", "change settings"},
            {QRegularExpression("(settings|configure|preferences)", QRegularExpression::CaseInsensitiveOption)},
            0.84F);
}

float IntentClassifier::scoreTfIdf(const QString &input, const IntentRule &rule) const {
    if (rule.keywords.isEmpty()) {
        return 0.0F;
    }

    const QStringList tokens = input.split(QRegularExpression("[^a-z0-9_\\.]+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return 0.0F;
    }

    float total = 0.0F;
    for (const QString &keyword : rule.keywords) {
        const QString normalizedKeyword = keyword.toLower().trimmed();
        if (normalizedKeyword.isEmpty()) {
            continue;
        }

        if (input.contains(normalizedKeyword, Qt::CaseInsensitive)) {
            float termWeight = 1.0F;
            if (normalizedKeyword.contains(' ')) {
                termWeight += 0.3F;
            }
            total += termWeight;
        }
    }

    const float normalized = total / static_cast<float>(rule.keywords.size());
    return rule.baseConfidence * std::min(1.0F, normalized * 0.9F);
}

QString IntentClassifier::extractEntity(const QString &input, Intent intent) const {
    if (intent == Intent::KillProcess || intent == Intent::AskAboutProcess || intent == Intent::ProcessQuestion) {
        const QRegularExpression explicitExe("([a-z0-9_\\-]+\\.exe)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = explicitExe.match(input);
        if (match.hasMatch()) {
            return match.captured(1).toLower();
        }

        const QRegularExpression verbEntity("(kill|close|terminate|stop|end|what\\s+is|about)\\s+([a-z0-9_\\-]+)",
                                            QRegularExpression::CaseInsensitiveOption);
        match = verbEntity.match(input);
        if (match.hasMatch()) {
            return match.captured(2).toLower();
        }
    }

    return {};
}

QMap<QString, QString> IntentClassifier::extractSlots(const QString &input, Intent intent) const {
    QMap<QString, QString> slotValues;

    if (intent == Intent::RunStartupFix) {
        const QRegularExpression delayExpr("(delay|after)\\s+(\\d+)\\s*(s|sec|seconds)?", QRegularExpression::CaseInsensitiveOption);
        const auto delayMatch = delayExpr.match(input);
        if (delayMatch.hasMatch()) {
            slotValues.insert("delay_seconds", delayMatch.captured(2));
        }
    }

    if (intent == Intent::AskForecast) {
        if (input.contains("disk", Qt::CaseInsensitive)) {
            slotValues.insert("metric", "disk");
        } else if (input.contains("ram", Qt::CaseInsensitive) || input.contains("memory", Qt::CaseInsensitive)) {
            slotValues.insert("metric", "ram");
        } else if (input.contains("cpu", Qt::CaseInsensitive)) {
            slotValues.insert("metric", "cpu");
        } else {
            slotValues.insert("metric", "health");
        }
    }

    if (intent == Intent::ChangeSettings) {
        if (input.contains("theme", Qt::CaseInsensitive)) {
            slotValues.insert("category", "appearance");
        } else if (input.contains("alert", Qt::CaseInsensitive)) {
            slotValues.insert("category", "alerts");
        } else if (input.contains("monitor", Qt::CaseInsensitive)) {
            slotValues.insert("category", "monitoring");
        } else {
            slotValues.insert("category", "general");
        }
    }

    const QString entity = extractEntity(input, intent);
    if (!entity.isEmpty()) {
        slotValues.insert("entity", entity);
    }

    return slotValues;
}

ClassificationResult IntentClassifier::classify(const QString &input) const {
    const QString normalized = input.toLower().trimmed();

    ClassificationResult best;
    for (const IntentRule &rule : m_rules) {
        float score = 0.0F;

        for (const QString &phrase : rule.phrases) {
            if (normalized.contains(phrase, Qt::CaseInsensitive)) {
                score = std::max(score, rule.baseConfidence);
            }
        }

        for (const QRegularExpression &pattern : rule.patterns) {
            if (pattern.match(normalized).hasMatch()) {
                score = std::max(score, rule.baseConfidence * 0.95F);
            }
        }

        score = std::max(score, scoreTfIdf(normalized, rule));
        if (score > best.confidence) {
            best.intent = rule.intent;
            best.confidence = std::min(1.0F, score);
            best.extractedEntity = extractEntity(normalized, rule.intent);
            best.slotValues = extractSlots(normalized, rule.intent);
        }
    }

    if (best.confidence < 0.35F) {
        best.intent = Intent::Unknown;
    }

    return best;
}

}  // namespace pulseboost
