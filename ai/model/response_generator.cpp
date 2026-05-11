#include "PulseBoostAI/ai/model/response_generator.hpp"

#include <QDateTime>
#include <QRandomGenerator>
#include <QStringList>

#include <algorithm>

namespace pulseboost {

ResponseGenerator::ResponseGenerator() {
    buildTemplates();
}

void ResponseGenerator::buildTemplates() {
    auto add = [this](const QString &id,
                      Intent intent,
                      const QString &text,
                      std::function<bool(const FusedContext &)> condition = {},
                      float weight = 1.0F) {
        ResponseTemplate tmpl;
        tmpl.id = id;
        tmpl.intent = intent;
        tmpl.templateText = text;
        tmpl.condition = condition ? std::move(condition) : [](const FusedContext &) { return true; };
        tmpl.weight = weight;
        m_templates.push_back(std::move(tmpl));
    };

    add("greet_good",
        Intent::Greeting,
        "Hey. System health is {health}/100 ({health_grade}). CPU is {cpu}% and RAM is {ram}%. What do you want to optimize first?",
        [](const FusedContext &c) { return c.healthScore >= 65.0; });
    add("greet_warn",
        Intent::Greeting,
        "Hi. I am seeing {primary_issue}. Current state: CPU {cpu}% | RAM {ram}% | Disk {disk}%. Want me to fix this now?",
        [](const FusedContext &c) { return c.healthScore < 65.0; });
    add("how_are_you",
        Intent::HowAreYou,
        "Running local and stable. PulseModel is active with health {health}/100. Main issue right now: {primary_issue}.",
        {},
        0.95F);
    add("thanks",
        Intent::ThankYou,
        "Good result. System is now {health_grade} at {health}/100. I can keep monitoring and alert you before it degrades again.");
    add("goodbye", Intent::Goodbye, "Understood. I will keep monitoring in the background. Come back any time.");

    add("slow_cpu",
        Intent::WhyIsSlow,
        "The slowdown is mostly CPU pressure at {cpu}%. Top process: {top_cpu_process}. This reduces headroom for foreground tasks.",
        [](const FusedContext &c) { return c.cpuPct >= c.ramPct && c.cpuPct >= 75.0; });
    add("slow_ram",
        Intent::WhyIsSlow,
        "The slowdown is memory pressure. RAM is {ram}% and paging is likely active. Top memory process is {top_ram_process}.",
        [](const FusedContext &c) { return c.ramPct >= c.cpuPct && c.ramPct >= 78.0; });
    add("slow_disk",
        Intent::WhyIsSlow,
        "The slowdown is storage pressure. Disk usage is {disk}% and I/O contention is likely.",
        [](const FusedContext &c) { return c.diskPct >= 85.0; });
    add("slow_multi",
        Intent::WhyIsSlow,
        "Multiple pressure sources are active: CPU {cpu}%, RAM {ram}%, Disk {disk}%. This combined load causes the lag you feel.",
        [](const FusedContext &c) { return (c.cpuPct > 70.0 && c.ramPct > 75.0) || (c.ramPct > 75.0 && c.diskPct > 85.0); });

    add("status_excellent",
        Intent::SystemStatus,
        "System status is excellent. Health {health}/100. CPU {cpu}% | RAM {ram}% | Disk {disk}%. No urgent optimization needed.",
        [](const FusedContext &c) { return c.healthScore >= 85.0; });
    add("status_good",
        Intent::SystemStatus,
        "System status is good. Health {health}/100. Primary issue: {primary_issue}. Trend: {health_trend}.",
        [](const FusedContext &c) { return c.healthScore >= 65.0 && c.healthScore < 85.0; });
    add("status_poor",
        Intent::SystemStatus,
        "System needs action. Health {health}/100 ({health_grade}). CPU {cpu}% | RAM {ram}% | Disk {disk}%.",
        [](const FusedContext &c) { return c.healthScore < 65.0; });

    add("cpu_detail",
        Intent::CpuQuestion,
        "CPU is at {cpu}% now. Top CPU process is {top_cpu_process}. {primary_issue}");
    add("ram_detail",
        Intent::RamQuestion,
        "RAM is at {ram}% now. Top RAM process is {top_ram_process}. Memory pressure status: {ram_pressure}.");
    add("disk_detail",
        Intent::DiskQuestion,
        "Disk usage is {disk}% with current health score {health}. Main storage concern: {primary_issue}.");
    add("net_detail",
        Intent::NetworkQuestion,
        "Network throughput is {network} Mbps. If browsing feels slow, I can run DNS and TCP optimization.");
    add("temp_detail",
        Intent::TempQuestion,
        "Estimated thermal state is {cpu_temp}C. Throttling status: {throttling}.");
    add("battery_detail", Intent::BatteryQuestion, "Battery and power profile checks are available in Power settings.");
    add("process_detail", Intent::ProcessQuestion, "Top process pressure is currently {top_cpu_process}.");
    add("health_detail",
        Intent::HealthQuestion,
        "Health score is {health}/100 ({health_grade}). Computed from CPU, memory, disk, startup and process pressure.");

    add("clean_confirm",
        Intent::RunClean,
        "I can run a safe cleanup now: temp files, browser caches, log caches and update residue. Personal files are excluded.");
    add("optimize_confirm",
        Intent::RunOptimize,
        "I can run a full optimization pass now. I will create a restore point first, then apply safe system optimizations.");
    add("game_confirm",
        Intent::RunGameMode,
        "I can enable Game Mode now by prioritizing active game workload and reducing background contention.");
    add("dev_confirm",
        Intent::RunDevMode,
        "I can enable Developer Mode now to prioritize build tools and reduce background interference.");
    add("network_confirm",
        Intent::RunNetworkFix,
        "I can run network optimization now: DNS flush and TCP tuning.");
    add("startup_confirm",
        Intent::RunStartupFix,
        "I can audit startup items and recommend safe disable/delay changes.");
    add("ram_confirm",
        Intent::RunRamFree,
        "I can trim non-critical working sets now to quickly recover RAM headroom.");
    add("scan_confirm",
        Intent::RunFullScan,
        "I can run a full scan now and return a ranked plan by impact and risk.");
    add("kill_confirm",
        Intent::KillProcess,
        "I can terminate {entity} if it is not critical. I will block known critical system processes.");
    add("restore_confirm",
        Intent::CreateRestore,
        "I can create a restore point now before any high-impact operation.");

    add("feature_help",
        Intent::AskAboutFeature,
        "I can help with cleanup, game mode, dev mode, network tuning, startup optimization and RAM optimization.");
    add("score_help",
        Intent::AskAboutScore,
        "Health score summarizes CPU, memory, disk, startup and process hygiene into a 0-100 signal.");
    add("process_help",
        Intent::AskAboutProcess,
        "Process analysis checks CPU, memory footprint and criticality before recommending any action.");
    add("history_help",
        Intent::AskHistory,
        "Recent actions are stored locally and can be reviewed in Health History.");
    add("forecast_help",
        Intent::AskForecast,
        "Forecasts use recent telemetry trends. Current trend signal: {health_trend}.");
    add("settings_help",
        Intent::ChangeSettings,
        "Open Settings to configure monitoring, alerts, appearance, and PulseModel learning options.");

    add("unknown_warn",
        Intent::Unknown,
        "I did not classify that clearly. Current telemetry: CPU {cpu}% | RAM {ram}% | Disk {disk}% | Health {health}/100.",
        [](const FusedContext &c) { return c.healthScore < 80.0; });
    add("unknown_stable",
        Intent::Unknown,
        "I did not catch that exactly. System is currently {health_grade} at {health}/100. Try: \"why is my pc slow\" or \"clean my system\".",
        [](const FusedContext &c) { return c.healthScore >= 80.0; });

    struct AutoPack {
        Intent intent = Intent::Unknown;
        QString key;
        QString core;
    };

    const QVector<AutoPack> packs {
        {Intent::Greeting, "greet", "Local telemetry is active. Health is {health}/100 and main issue is {primary_issue}."},
        {Intent::WhyIsSlow, "slow", "Current bottleneck is {primary_issue}. CPU {cpu}% | RAM {ram}% | Disk {disk}%."},
        {Intent::SystemStatus, "status", "Snapshot: CPU {cpu}% RAM {ram}% Disk {disk}% Network {network} Mbps Health {health}/100."},
        {Intent::CpuQuestion, "cpu", "CPU detail: {cpu}% and top process {top_cpu_process}."},
        {Intent::RamQuestion, "ram", "RAM detail: {ram}% with pressure flag {ram_pressure} and top process {top_ram_process}."},
        {Intent::DiskQuestion, "disk", "Disk detail: {disk}% used and storage health linked issue is {primary_issue}."},
        {Intent::NetworkQuestion, "network", "Network detail: throughput {network} Mbps with local stack optimization available."},
        {Intent::TempQuestion, "temp", "Thermal detail: estimated {cpu_temp}C with throttling flag {throttling}."},
        {Intent::RunClean, "run_clean", "Safe cleanup can run now and usually improves storage pressure quickly."},
        {Intent::RunOptimize, "run_opt", "Full optimization can run now with restore point safety first."},
        {Intent::RunGameMode, "run_game", "Game mode can run now and prioritize foreground gameplay workload."},
        {Intent::RunDevMode, "run_dev", "Developer mode can run now for build-heavy sessions."},
        {Intent::RunStartupFix, "run_startup", "Startup optimization can audit and reduce boot drag."},
        {Intent::RunNetworkFix, "run_net", "Network optimization can flush DNS and retune TCP parameters."},
        {Intent::RunRamFree, "run_ram", "RAM optimization can trim background working sets immediately."},
        {Intent::RunFullScan, "run_scan", "Full scan can produce a ranked action list with safety context."},
        {Intent::KillProcess, "kill", "Process action can terminate {entity} when safe."},
        {Intent::CreateRestore, "restore", "Restore point action is available and recommended before major changes."},
        {Intent::AskForecast, "forecast", "Forecast currently suggests {health_trend} with primary risk {primary_issue}."},
        {Intent::AskHistory, "history", "History is local-only and captures your recent optimization outcomes."},
        {Intent::AskAboutFeature, "feature", "Feature guidance is available for each optimization module."},
        {Intent::ChangeSettings, "settings", "Settings changes are available from the Settings screen immediately."}
    };

    const QStringList prefixes {
        "Analysis",
        "Signal",
        "Update",
        "Status",
        "Insight",
        "Telemetry note",
        "Recommendation",
        "PulseModel"
    };
    const QStringList suffixes {
        "Want me to continue?",
        "I can execute this now.",
        "This is local and adaptive.",
        "No cloud dependency is involved.",
        "I can provide a deeper breakdown.",
        "Let me know if you want action now.",
        "This result is based on live telemetry.",
        "I can keep monitoring this trend."
    };

    for (const AutoPack &pack : packs) {
        for (int index = 0; index < prefixes.size(); ++index) {
            add(QString("%1_auto_%2").arg(pack.key).arg(index),
                pack.intent,
                QString("%1: %2 %3").arg(prefixes[index], pack.core, suffixes[index]),
                {},
                0.75F);
        }
    }
}

ResponseTemplate *ResponseGenerator::selectBestTemplate(const QVector<ResponseTemplate *> &candidates,
                                                        const QHash<QString, float> &externalWeights) {
    if (candidates.isEmpty()) {
        return nullptr;
    }

    ResponseTemplate *best = candidates.front();
    float bestScore = -1.0F;
    for (ResponseTemplate *candidate : candidates) {
        const float learnedWeight = externalWeights.value(candidate->id, 1.0F);
        const float jitter = static_cast<float>(QRandomGenerator::global()->bounded(100)) / 1000.0F;
        const float score = std::max(0.05F, candidate->weight * learnedWeight) + jitter;
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }
    return best;
}

GeneratedResponse ResponseGenerator::fallbackUnknown(const FusedContext &ctx) const {
    GeneratedResponse response;
    response.text = fillSlots("I am not fully sure what you asked. Current state: CPU {cpu}%, RAM {ram}%, Disk {disk}%, Health {health}/100.",
                              ctx);
    response.templateId = "fallback_unknown";
    response.followUpSuggestions = {"Why is my PC slow?", "Clean my system", "Show system status"};
    return response;
}

void ResponseGenerator::decorateAction(GeneratedResponse &response, const FusedContext &ctx) const {
    const Intent intent = ctx.intent.intent;
    if (intent == Intent::RunClean) {
        response.hasAction = true;
        response.actionId = "clean_junk";
        response.actionLabel = "Run Safe Cleanup";
    } else if (intent == Intent::RunOptimize) {
        response.hasAction = true;
        response.actionId = "optimize_all";
        response.actionLabel = "Run Full Optimize";
    } else if (intent == Intent::RunGameMode) {
        response.hasAction = true;
        response.actionId = "enable_game_mode";
        response.actionLabel = "Enable Game Mode";
    } else if (intent == Intent::RunDevMode) {
        response.hasAction = true;
        response.actionId = "optimize_developer_mode";
        response.actionLabel = "Enable Dev Mode";
    } else if (intent == Intent::RunNetworkFix) {
        response.hasAction = true;
        response.actionId = "optimize_network";
        response.actionLabel = "Optimize Network";
    } else if (intent == Intent::RunStartupFix) {
        response.hasAction = true;
        response.actionId = "analyze_startup";
        response.actionLabel = "Analyze Startup";
    } else if (intent == Intent::RunRamFree) {
        response.hasAction = true;
        response.actionId = "optimize_ram";
        response.actionLabel = "Optimize RAM";
    } else if (intent == Intent::RunFullScan) {
        response.hasAction = true;
        response.actionId = "full_scan";
        response.actionLabel = "Run Full Scan";
    } else if (intent == Intent::KillProcess && !ctx.intent.extractedEntity.isEmpty()) {
        response.hasAction = true;
        response.actionId = "kill_process:" + ctx.intent.extractedEntity;
        response.actionLabel = "Kill " + ctx.intent.extractedEntity;
    } else if (intent == Intent::CreateRestore) {
        response.hasAction = true;
        response.actionId = "create_restore_point";
        response.actionLabel = "Create Restore Point";
    }

    if (response.followUpSuggestions.isEmpty()) {
        response.followUpSuggestions = {"Why is my PC slow?", "System status", "Optimize RAM"};
    }
}

QString ResponseGenerator::fillSlots(const QString &templateText, const FusedContext &ctx) const {
    QString text = templateText;
    text.replace("{cpu}", QString::number(ctx.cpuPct, 'f', 1));
    text.replace("{ram}", QString::number(ctx.ramPct, 'f', 1));
    text.replace("{disk}", QString::number(ctx.diskPct, 'f', 1));
    text.replace("{network}", QString::number(ctx.networkMbps, 'f', 1));
    text.replace("{health}", QString::number(static_cast<int>(ctx.healthScore)));
    text.replace("{health_grade}", ctx.healthGrade);
    text.replace("{cpu_temp}", QString::number(ctx.cpuTempC, 'f', 1));
    text.replace("{top_cpu_process}", ctx.topCpuProcess.isEmpty() ? "background process" : ctx.topCpuProcess);
    text.replace("{top_ram_process}", ctx.topRamProcess.isEmpty() ? "background process" : ctx.topRamProcess);
    text.replace("{primary_issue}", ctx.primaryIssue.isEmpty() ? "no critical issue detected" : ctx.primaryIssue);
    text.replace("{entity}", ctx.intent.extractedEntity.isEmpty() ? "selected process" : ctx.intent.extractedEntity);
    text.replace("{ram_pressure}", ctx.isRamPressured ? "high" : "normal");
    text.replace("{throttling}", ctx.isCpuThrottling ? "active" : "not active");
    text.replace("{health_trend}", ctx.healthTrending ? "improving" : "declining");
    return text;
}

GeneratedResponse ResponseGenerator::generate(const FusedContext &ctx, const QHash<QString, float> &externalWeights) {
    QVector<ResponseTemplate *> candidates;
    candidates.reserve(m_templates.size());

    for (ResponseTemplate &tmpl : m_templates) {
        if (tmpl.intent == ctx.intent.intent && tmpl.condition(ctx)) {
            candidates.push_back(&tmpl);
        }
    }

    if (candidates.isEmpty()) {
        for (ResponseTemplate &tmpl : m_templates) {
            if (tmpl.intent == Intent::Unknown && tmpl.condition(ctx)) {
                candidates.push_back(&tmpl);
            }
        }
    }

    ResponseTemplate *selected = selectBestTemplate(candidates, externalWeights);
    if (selected == nullptr) {
        return fallbackUnknown(ctx);
    }

    selected->useCount += 1;

    GeneratedResponse response;
    response.templateId = selected->id;
    response.text = fillSlots(selected->templateText, ctx);
    decorateAction(response, ctx);
    return response;
}

}  // namespace pulseboost
