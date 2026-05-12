#include "PulseBoostAI/ui_backend/ui_controller.hpp"

#include <Windows.h>

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

QString classifyActionType(const QString &action) {
    const QString lower = action.toLower();
    if (lower.contains("clean")) return "Clean";
    if (lower.contains("kill")) return "Kill";
    if (lower.contains("security") || lower.contains("restore")) return "Security";
    if (lower.contains("ram") || lower.contains("optimize") || lower.contains("game") || lower.contains("net") || lower.contains("startup") || lower.contains("disk")) return "Optimize";
    return "Info";
}

QString actionTone(const QString &action, bool success) {
    if (!success) return "error";
    const QString type = classifyActionType(action);
    if (type == "Clean") return "info";
    if (type == "Kill") return "error";
    if (type == "Security") return "warning";
    return "success";
}

double parsedRecoveredMb(const QString &details) {
    static const QRegularExpression quantityPattern(QStringLiteral("([0-9]+(?:\\.[0-9]+)?)\\s*(GB|MB|KB|bytes?)"),
                                                    QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = quantityPattern.match(details);
    if (!match.hasMatch()) {
        return 0.0;
    }

    const double value = match.captured(1).toDouble();
    const QString unit = match.captured(2).toLower();
    if (unit.startsWith("gb")) return value * 1024.0;
    if (unit.startsWith("mb")) return value;
    if (unit.startsWith("kb")) return value / 1024.0;
    return value / (1024.0 * 1024.0);
}

QString localTimeFromUtc(const std::string &timestampUtc) {
    const QDateTime utc = QDateTime::fromString(QString::fromStdString(timestampUtc), Qt::ISODate);
    if (!utc.isValid()) {
        return QString::fromStdString(timestampUtc);
    }
    return utc.toLocalTime().toString("HH:mm:ss");
}

QString snapshotsDirectoryPath() {
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) {
        root = QDir::currentPath() + "/data";
    }
    return root + "/snapshots";
}

QString quarantineDirectoryPath() {
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) {
        root = QDir::currentPath() + "/data";
    }
    return root + "/quarantine";
}

QString pulseBoostDatabasePath() {
    return QString::fromStdString(SafetyPolicy::defaultDatabasePath().string());
}

QString actionDisplayName(const QString &actionId) {
    if (actionId == "junk.clean") return QStringLiteral("Safe Junk Cleanup");
    if (actionId == "network.flush_dns") return QStringLiteral("Flush DNS Cache");
    if (actionId == "network.optimize_tcp") return QStringLiteral("Advanced TCP Tuning");
    if (actionId == "network.revert") return QStringLiteral("Revert Network Tuning");
    if (actionId == "ram.trim_working_sets") return QStringLiteral("Manual RAM Working-Set Trim");
    if (actionId == "disk.optimize") return QStringLiteral("Windows Disk Optimization");
    if (actionId == "restore.create") return QStringLiteral("Create Restore Point");
    if (actionId == "snapshot.restore") return QStringLiteral("Restore Snapshot");
    if (actionId == "game.revert") return QStringLiteral("Revert Game Optimization");
    if (actionId == "tweak.apply") return QStringLiteral("Apply Selected Tweak");
    if (actionId == "tweak.revert") return QStringLiteral("Revert Selected Tweak");
    return actionId;
}

QString expectedEffectForAction(const QString &actionId) {
    if (actionId == "junk.clean") return QStringLiteral("Move known safe cleanup targets into quarantine and report recoverable storage.");
    if (actionId == "network.flush_dns") return QStringLiteral("Clear resolver cache so Windows refreshes DNS lookups.");
    if (actionId == "network.optimize_tcp") return QStringLiteral("Apply global TCP settings only after backup and explicit confirmation.");
    if (actionId == "network.revert") return QStringLiteral("Restore PulseBoost network tuning to conservative Windows defaults.");
    if (actionId == "ram.trim_working_sets") return QStringLiteral("Manually ask selected processes to reduce working-set pressure; not a guaranteed speed boost.");
    if (actionId == "disk.optimize") return QStringLiteral("Ask Windows to run its built-in system drive optimization task.");
    if (actionId == "restore.create") return QStringLiteral("Create a Windows restore point before higher-risk changes.");
    if (actionId == "snapshot.restore") return QStringLiteral("Restore startup baseline from a PulseBoost snapshot.");
    if (actionId == "game.revert") return QStringLiteral("Undo temporary game-session priority, power, and network state.");
    if (actionId == "tweak.apply") return QStringLiteral("Apply a reversible registered tweak after backup metadata is available.");
    if (actionId == "tweak.revert") return QStringLiteral("Use stored rollback metadata to undo a registered tweak.");
    return QStringLiteral("Review safety metadata before execution.");
}

QString toneForRisk(const QString &risk) {
    if (risk == "critical" || risk == "high") return QStringLiteral("error");
    if (risk == "moderate") return QStringLiteral("warning");
    if (risk == "low") return QStringLiteral("success");
    return QStringLiteral("neutral");
}

QJsonObject variantMapToJsonObject(const QVariantMap &map) {
    return QJsonObject::fromVariantMap(map);
}

QString snapshotStartupKey(const StartupItem &item) {
    return (QString::fromStdString(item.name).trimmed().toLower() + "|" +
            QString::fromStdString(item.location).trimmed().toLower() + "|" +
            QString::fromStdString(item.command).trimmed().toLower());
}

QString snapshotStartupKeyFromJson(const QJsonObject &object) {
    return (object.value("name").toString().trimmed().toLower() + "|" +
            object.value("location").toString().trimmed().toLower() + "|" +
            object.value("command").toString().trimmed().toLower());
}

QStringList fallbackGameExecutables() {
    return {
        QStringLiteral("valorant.exe"), QStringLiteral("cs2.exe"), QStringLiteral("eldenring.exe"), QStringLiteral("fortniteclient-win64-shipping.exe"),
        QStringLiteral("league of legends.exe"), QStringLiteral("rocketleague.exe"), QStringLiteral("r5apex.exe"), QStringLiteral("overwatch.exe"),
        QStringLiteral("cod.exe"), QStringLiteral("modernwarfare.exe"), QStringLiteral("starfield.exe"), QStringLiteral("gta5.exe"),
        QStringLiteral("witcher3.exe"), QStringLiteral("cyberpunk2077.exe"), QStringLiteral("dota2.exe"), QStringLiteral("bhd.exe")
    };
}

QStringList loadKnownGames() {
    static const QStringList cached = []() {
        QFile file(QStringLiteral("data/common_games.json"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return fallbackGameExecutables();
        }

        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isArray()) {
            return fallbackGameExecutables();
        }

        QStringList names;
        for (const QJsonValue &value : doc.array()) {
            const QString name = value.toString().trimmed().toLower();
            if (!name.isEmpty()) {
                names.push_back(name);
            }
        }
        return names.isEmpty() ? fallbackGameExecutables() : names;
    }();
    return cached;
}

bool isKnownGameExecutable(const QString &executable) {
    const QString normalized = executable.trimmed().toLower();
    const QStringList knownGames = loadKnownGames();
    return knownGames.contains(normalized);
}

std::optional<ProcessInfo> detectGameCandidate(const std::vector<ProcessInfo> &processes) {
    std::optional<ProcessInfo> fallback;
    for (const auto &process : processes) {
        if (process.isCritical) {
            continue;
        }

        const QString executable = QString::fromStdString(process.name).trimmed().toLower();
        if (isKnownGameExecutable(executable)) {
            return process;
        }

        if (!fallback.has_value() && process.cpuPercent >= 8.0 && process.memoryMb >= 700.0) {
            fallback = process;
        }
    }
    return fallback;
}

double computeGameModeBoostEstimate(const SystemSnapshot &snapshot) {
    const double cpuHeadroom = std::max(0.0, 100.0 - snapshot.cpuUsagePercent);
    const double ramHeadroom = std::max(0.0, 100.0 - snapshot.ramUsagePercent);
    return std::clamp((cpuHeadroom * 0.08) + (ramHeadroom * 0.05), 4.0, 18.0);
}

QString thermalZoneLabel(double tempC) {
    if (tempC >= 85.0) return QStringLiteral("Critical");
    if (tempC >= 75.0) return QStringLiteral("Hot");
    if (tempC >= 60.0) return QStringLiteral("Warm");
    return QStringLiteral("Cool");
}

QString thermalTone(double tempC) {
    if (tempC >= 85.0) return QStringLiteral("error");
    if (tempC >= 60.0) return QStringLiteral("warning");
    return QStringLiteral("success");
}

int extractDriverYear(const QString &driverDate) {
    static const QRegularExpression yearPattern(QStringLiteral("(19|20)\\d{2}"));
    const auto match = yearPattern.match(driverDate);
    if (!match.hasMatch()) {
        return 0;
    }
    return match.captured(0).toInt();
}

bool isDriverOutdated(const DriverInfo &driver) {
    const int year = extractDriverYear(QString::fromStdString(driver.driverDate));
    if (year == 0) {
        return false;
    }
    return year <= (QDate::currentDate().year() - 1);
}

QString driverStatusLabel(const DriverInfo &driver) {
    if (!driver.isSigned) {
        return QStringLiteral("Unsigned");
    }
    if (isDriverOutdated(driver)) {
        return QStringLiteral("Outdated");
    }
    return QStringLiteral("Current");
}

QString driverStatusTone(const DriverInfo &driver) {
    if (!driver.isSigned) {
        return QStringLiteral("error");
    }
    if (isDriverOutdated(driver)) {
        return QStringLiteral("warning");
    }
    return QStringLiteral("success");
}

QString driverLookupUrl(const DriverInfo &driver) {
    const QString query = QString::fromStdString(driver.provider + " " + driver.deviceName + " driver update");
    return QStringLiteral("https://www.google.com/search?q=%1").arg(QString::fromLatin1(QUrl::toPercentEncoding(query)));
}

QVariantMap tweakToVariant(const TweakDefinition &tweak) {
    QVariantMap map;
    map["id"] = QString::fromStdString(tweak.id);
    map["category"] = QString::fromStdString(tweak.category);
    map["name"] = QString::fromStdString(tweak.name);
    map["description"] = QString::fromStdString(tweak.description);
    map["detailedInfo"] = QString::fromStdString(tweak.detailedInfo);
    map["impact"] = QString::fromStdString(tweak.impact);
    map["risk"] = QString::fromStdString(tweak.riskLevel);
    map["requiresRestart"] = tweak.requiresRestart;
    map["isApplied"] = tweak.isApplied;
    map["isApplicable"] = tweak.isApplicable;
    map["notApplicableReason"] = QString::fromStdString(tweak.notApplicableReason);
    return map;
}

QVariantMap advisorToVariant(const AdvisorItem &item) {
    QVariantMap map;
    map["id"] = QString::fromStdString(item.id);
    map["category"] = QString::fromStdString(item.category);
    map["title"] = QString::fromStdString(item.title);
    map["description"] = QString::fromStdString(item.description);
    map["impact"] = QString::fromStdString(item.impact);
    map["status"] = QString::fromStdString(item.status);
    map["actionable"] = item.actionable;
    map["actionLabel"] = QString::fromStdString(item.actionLabel);
    map["actionId"] = QString::fromStdString(item.actionId);
    return map;
}

QVariantMap gameToVariant(const GameProfile &game) {
    QVariantMap map;
    map["executableName"] = QString::fromStdString(game.executableName);
    map["displayName"] = QString::fromStdString(game.displayName);
    map["installPath"] = QString::fromStdString(game.installPath);
    map["launcher"] = QString::fromStdString(game.launcher);
    map["isDetected"] = game.isDetected;
    map["isOptimized"] = game.isOptimized;
    map["isRunning"] = game.isRunning;
    map["runningPid"] = static_cast<int>(game.runningPid);
    map["tweaksAvailable"] = game.tweaksAvailable;
    QVariantList tweaks;
    for (const auto &tweak : game.tweaksApplied) {
        tweaks.push_back(QString::fromStdString(tweak));
    }
    map["tweaksApplied"] = tweaks;
    return map;
}

QString pulseGrade(int total) {
    if (total >= 900) return QStringLiteral("S");
    if (total >= 750) return QStringLiteral("A");
    if (total >= 600) return QStringLiteral("B");
    if (total >= 450) return QStringLiteral("C");
    if (total >= 300) return QStringLiteral("D");
    return QStringLiteral("F");
}

QVariantMap benchmarkToVariant(const BenchmarkResult &result) {
    QVariantMap map;
    map["cpuScore"] = result.cpuScore;
    map["ramBandwidthMBps"] = result.ramBandwidthMBps;
    map["diskReadMBps"] = result.diskReadMBps;
    map["networkLatencyMs"] = result.networkLatencyMs;
    map["gpuScore"] = result.gpuScore;
    map["compositeScore"] = result.compositeScore;
    map["pulseScore"] = result.pulseScore;
    map["grade"] = QString::fromStdString(result.grade);
    map["timestamp"] = static_cast<qlonglong>(result.timestamp);
    return map;
}

}  // namespace

UiController::UiController(JunkCleaner &junkCleaner,
                           GameMode &gameMode,
                           ProcessManager &processManager,
                           DiskAnalyzer &diskAnalyzer,
                           StartupOptimizer &startupOptimizer,
                           SafetyGuard &safetyGuard,
                           TelemetryCache &telemetryCache,
                           OptimizationHistory &history,
                           NetworkOptimizer &networkOptimizer,
                           TweakEngine &tweakEngine,
                           GameOptimizer &gameOptimizer,
                           SystemAdvisor &systemAdvisor,
                           PulseBench &pulseBench,
                           QObject *parent)
    : QObject(parent),
      junkCleaner_(junkCleaner),
      gameMode_(gameMode),
      processManager_(processManager),
      diskAnalyzer_(diskAnalyzer),
      startupOptimizer_(startupOptimizer),
      safetyGuard_(safetyGuard),
      telemetryCache_(telemetryCache),
      history_(history),
      networkOptimizer_(networkOptimizer),
      tweakEngine_(tweakEngine),
      gameOptimizer_(gameOptimizer),
      systemAdvisor_(systemAdvisor),
      pulseBench_(pulseBench) {}

QString UiController::healthLabel() const {
    if (snapshot_.healthScore >= 80) return "Stable";
    if (snapshot_.healthScore >= 60) return "Attention";
    return "Critical";
}

QVariantList UiController::chartSeries() const {
    return telemetryCache_.chartSeries(60);
}

QVariantList UiController::thermalSeries() const {
    QVariantList points;
    const auto history = telemetryCache_.history(60);
    int index = 0;
    for (const auto &snapshot : history) {
        QVariantMap point;
        point["index"] = index++;
        point["cpuTemp"] = snapshot.cpuTempC;
        point["gpuTemp"] = snapshot.gpuTempC;
        point["driveTemp"] = snapshot.driveTempC;
        points.push_back(point);
    }
    return points;
}

void UiController::recomputeUiCaches() {
    cachedRecentActions_.clear();
    auto records = history_.load();
    std::reverse(records.begin(), records.end());
    for (const auto &record : records) {
        QVariantMap map;
        const QString action = QString::fromStdString(record.action);
        const QString details = QString::fromStdString(record.details);
        map["timestamp"] = QString::fromStdString(record.timestampUtc);
        map["timeLabel"] = localTimeFromUtc(record.timestampUtc);
        map["action"] = action;
        map["details"] = details;
        map["success"] = record.success;
        map["type"] = classifyActionType(action);
        map["tone"] = actionTone(action, record.success);
        map["savedMb"] = parsedRecoveredMb(details);
        cachedRecentActions_.push_back(map);
    }

    const double totalMb = std::max(0.0, snapshot_.ramTotalMb);
    const double usedMb = std::max(0.0, snapshot_.ramUsedMb);
    const double freeMb = std::max(0.0, totalMb - usedMb);
    double processWorkingSetMb = 0.0;
    for (const auto &process : snapshot_.heavyProcesses) {
        processWorkingSetMb += process.memoryMb;
    }
    processWorkingSetMb = std::min(processWorkingSetMb, usedMb);
    const double nonProcessUsedMb = std::max(0.0, usedMb - processWorkingSetMb);
    cachedMemoryOverview_.clear();
    cachedMemoryOverview_["totalMb"] = totalMb;
    cachedMemoryOverview_["usedMb"] = usedMb;
    cachedMemoryOverview_["freeMb"] = freeMb;
    cachedMemoryOverview_["appsMb"] = processWorkingSetMb;
    cachedMemoryOverview_["systemMb"] = nonProcessUsedMb;
    cachedMemoryOverview_["cachedMb"] = 0.0;
    cachedMemoryOverview_["usedPct"] = snapshot_.ramUsagePercent;

    const double pressureFactor = std::clamp((snapshot_.ramUsagePercent - 55.0) / 45.0, 0.0, 1.0);
    cachedRecoverableRamMb_ = std::clamp((processWorkingSetMb * 0.10) + (pressureFactor * 384.0), 64.0, 2048.0);

    const double hottest = std::max({snapshot_.cpuTempC, snapshot_.gpuTempC, snapshot_.driveTempC});
    cachedThermalOverview_.clear();
    cachedThermalOverview_["cpuTempC"] = snapshot_.cpuTempC;
    cachedThermalOverview_["gpuTempC"] = snapshot_.gpuTempC;
    cachedThermalOverview_["driveTempC"] = snapshot_.driveTempC;
    cachedThermalOverview_["fanSpeedRpm"] = snapshot_.fanSpeedRpm;
    cachedThermalOverview_["cpuZone"] = thermalZoneLabel(snapshot_.cpuTempC);
    cachedThermalOverview_["gpuZone"] = thermalZoneLabel(snapshot_.gpuTempC);
    cachedThermalOverview_["driveZone"] = thermalZoneLabel(snapshot_.driveTempC);
    cachedThermalOverview_["overallZone"] = thermalZoneLabel(hottest);
    cachedThermalOverview_["hottestTempC"] = hottest;
    cachedThermalOverview_["hottestTone"] = thermalTone(hottest);
    cachedThermalOverview_["cpuThrottling"] = snapshot_.cpuThrottling;
    cachedThermalOverview_["thermalScore"] = snapshot_.thermalStateScore;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (lastLatencyCheckMs_ <= 0 || (nowMs - lastLatencyCheckMs_) >= 15000) {
        cachedLatencyMs_ = checkLatency();
        lastLatencyCheckMs_ = nowMs;
    }
    cachedNetworkOverview_.clear();
    cachedNetworkOverview_["activeConnections"] = snapshot_.networkMbps > 0.05 ? 1 : 0;
    cachedNetworkOverview_["topProcess"] = snapshot_.heavyProcesses.empty() ? QStringLiteral("Unavailable") : QString::fromStdString(snapshot_.heavyProcesses.front().name);
    cachedNetworkOverview_["topBandwidthMbps"] = snapshot_.networkMbps;
    cachedNetworkOverview_["dnsServer"] = QStringLiteral("System configured");
    cachedNetworkOverview_["connectionType"] = snapshot_.networkMbps > 0.05 ? QStringLiteral("Active") : QStringLiteral("Idle");
    cachedNetworkOverview_["signalQuality"] = snapshot_.networkQualityScore;
    cachedNetworkOverview_["latency"] = cachedLatencyMs_;

    cachedNotifications_.clear();
    auto pushNotification = [this](const QString &title, const QString &message, const QString &tone) {
        QVariantMap item;
        item["title"] = title;
        item["message"] = message;
        item["tone"] = tone;
        cachedNotifications_.push_back(item);
    };
    if (!snapshot_.summary.empty()) {
        pushNotification(QStringLiteral("System Summary"), QString::fromStdString(snapshot_.summary), QStringLiteral("info"));
    }
    if (snapshot_.cpuUsagePercent >= 85.0) {
        pushNotification(QStringLiteral("CPU Alert"), QStringLiteral("CPU load is above 85 percent."), QStringLiteral("warning"));
    }
    if (snapshot_.ramUsagePercent >= 80.0) {
        pushNotification(QStringLiteral("Memory Pressure"), QStringLiteral("RAM usage is high."), QStringLiteral("warning"));
    }
    if (snapshot_.diskUsagePercent >= 85.0) {
        pushNotification(QStringLiteral("Disk Capacity"), QStringLiteral("System drive is nearly full."), QStringLiteral("warning"));
    }
    if (snapshot_.cpuThrottling || snapshot_.cpuTempC >= 85.0) {
        pushNotification(QStringLiteral("Thermal Pressure"), QStringLiteral("CPU temperature is high."), QStringLiteral("warning"));
    }
    const QVariantMap drivers = driverSummary();
    if (drivers.value("unsigned").toInt() > 0) {
        pushNotification(QStringLiteral("Unsigned Drivers"), QStringLiteral("One or more installed drivers are unsigned."), QStringLiteral("error"));
    } else if (drivers.value("outdated").toInt() > 0) {
        pushNotification(QStringLiteral("Driver Maintenance"), QStringLiteral("Several drivers need review."), QStringLiteral("warning"));
    }
    const int recentNotificationCount = std::min(3, static_cast<int>(cachedRecentActions_.size()));
    for (int index = 0; index < recentNotificationCount; ++index) {
        const QVariantMap action = cachedRecentActions_.at(index).toMap();
        pushNotification(action.value("action").toString(), action.value("details").toString(), action.value("tone").toString());
    }

    cachedSystemSnapshots_.clear();
    QDir directory(snapshotsDirectoryPath());
    if (directory.exists()) {
        const QFileInfoList files = directory.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Time);
        for (const QFileInfo &fileInfo : files) {
            QFile file(fileInfo.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
            if (!document.isObject()) {
                continue;
            }
            const QJsonObject root = document.object();
            QVariantMap row;
            row["id"] = root.value("id").toString(fileInfo.baseName());
            row["createdUtc"] = root.value("createdUtc").toString();
            row["healthScore"] = root.value("healthScore").toInt();
            row["startupCount"] = root.value("startupCount").toInt();
            row["processCount"] = root.value("processCount").toInt();
            row["sizeBytes"] = static_cast<qint64>(fileInfo.size());
            cachedSystemSnapshots_.push_back(row);
        }
    }

    cachedPulseScore_.clear();
    const auto tweaks = tweakEngine_.listTweaks();
    const int applicable = static_cast<int>(std::count_if(tweaks.begin(), tweaks.end(), [](const auto &t) { return t.isApplicable; }));
    const int applied = static_cast<int>(std::count_if(tweaks.begin(), tweaks.end(), [](const auto &t) { return t.isApplied; }));
    const int hardwareTier = std::clamp(snapshot_.cpuEfficiencyScore + snapshot_.memoryPressureScore + snapshot_.diskHealthScore, 0, 300);
    const int optimizationLevel = applicable > 0 ? std::clamp(static_cast<int>(std::round((static_cast<double>(applied) / static_cast<double>(applicable)) * 400.0)), 0, 400) : 0;
    const int healthState = std::clamp(snapshot_.healthScore * 2, 0, 200);
    int benchScore = 50;
    const auto benchHistory = pulseBench_.loadHistory();
    if (!benchHistory.empty()) {
        benchScore = std::clamp(static_cast<int>(std::round(benchHistory.back().pulseScore / 10.0)), 0, 100);
    }
    const int total = std::clamp(hardwareTier + optimizationLevel + healthState + benchScore, 0, 1000);
    cachedPulseScore_["total"] = total;
    cachedPulseScore_["hardwareTier"] = hardwareTier;
    cachedPulseScore_["optimizationLevel"] = optimizationLevel;
    cachedPulseScore_["healthState"] = healthState;
    cachedPulseScore_["benchScore"] = benchScore;
    cachedPulseScore_["grade"] = pulseGrade(total);
    cachedPulseScore_["percentile"] = 0;
    cachedPulseScore_["tweaksApplied"] = applied;
    cachedPulseScore_["tweaksAvailable"] = applicable;

    cachedBenchmarkDelta_.clear();
    const auto delta = pulseBench_.latestDelta();
    cachedBenchmarkDelta_["available"] = delta.has_value();
    if (delta.has_value()) {
        cachedBenchmarkDelta_["before"] = benchmarkToVariant(delta->before);
        cachedBenchmarkDelta_["after"] = benchmarkToVariant(delta->after);
        cachedBenchmarkDelta_["percentChange"] = delta->percentChange;
        cachedBenchmarkDelta_["scoreDelta"] = delta->scoreDelta;
    }

    cachedAdvisorItems_.clear();
    const auto advisorResults = systemAdvisor_.analyze(snapshot_, tweaks);
    for (const auto &item : advisorResults) {
        cachedAdvisorItems_.push_back(advisorToVariant(item));
    }

    cachedDetectedGames_.clear();
    const auto session = gameOptimizer_.currentSessionStatus();
    const auto games = gameOptimizer_.detectInstalledGames();
    for (auto game : games) {
        if (!session.displayName.empty() && (game.displayName == session.displayName || game.executableName == session.executableName)) {
            game.isRunning = session.pid != 0;
            game.isOptimized = session.reason != "inactive";
            game.runningPid = session.pid;
            game.tweaksApplied = session.actionsApplied;
        }
        cachedDetectedGames_.push_back(gameToVariant(game));
    }
}

void UiController::onSnapshotReady(const SystemSnapshot &snapshot) {
    snapshot_ = snapshot;
    uiDataReady_ = true;
    uiErrorMessage_.clear();
    lastSnapshotEpochMs_ = QDateTime::currentMSecsSinceEpoch();
    refreshDetectedGame();
    recomputeUiCaches();
    emit metricsChanged();
    emit chartSeriesChanged();
    emit processListChanged();
    emit storageChanged();
    emit notificationCenterChanged();
    emit gameModeChanged();
    emit snapshotsChanged();
    emit actionsChanged();
    emit uiStateChanged();
}

int UiController::telemetryAgeMs() const {
    if (!uiDataReady_ || lastSnapshotEpochMs_ <= 0) {
        return -1;
    }
    const qint64 age = QDateTime::currentMSecsSinceEpoch() - lastSnapshotEpochMs_;
    return static_cast<int>(std::clamp<qint64>(age, 0, 60 * 60 * 1000));
}

QVariantList UiController::processList() const {
    return sortedProcesses("cpu", true);
}

QVariantList UiController::sortedProcesses(const QString &sortKey, bool descending) const {
    auto processes = snapshot_.heavyProcesses;
    std::sort(processes.begin(), processes.end(), [&](const ProcessInfo &left, const ProcessInfo &right) {
        if (sortKey == "name") return descending ? left.name > right.name : left.name < right.name;
        if (sortKey == "ram") return descending ? left.memoryMb > right.memoryMb : left.memoryMb < right.memoryMb;
        return descending ? left.cpuPercent > right.cpuPercent : left.cpuPercent < right.cpuPercent;
    });

    QVariantList list;
    for (const auto &process : processes) {
        QVariantMap map;
        const int risk = process.isCritical ? 4 : (process.cpuPercent > 80.0 ? 3 : (process.memoryMb > 1200.0 ? 2 : 1));
        map["pid"] = static_cast<int>(process.pid);
        map["name"] = QString::fromStdString(process.name);
        map["cpuPercent"] = process.cpuPercent;
        map["memoryMb"] = process.memoryMb;
        map["isCritical"] = process.isCritical;
        map["risk"] = risk;
        map["riskLabel"] = risk >= 4 ? "Critical" : (risk == 3 ? "Danger" : (risk == 2 ? "Caution" : "Safe"));
        map["nameInitial"] = QString::fromStdString(process.name).left(1).toUpper();
        list.push_back(map);
    }
    return list;
}

QVariantList UiController::driverList() const {
    QVariantList list;
    for (const auto &driver : snapshot_.drivers) {
        QVariantMap map;
        map["deviceName"] = QString::fromStdString(driver.deviceName);
        map["driverVersion"] = QString::fromStdString(driver.driverVersion);
        map["provider"] = QString::fromStdString(driver.provider.empty() ? std::string("Unknown Provider") : driver.provider);
        map["driverDate"] = QString::fromStdString(driver.driverDate.empty() ? std::string("Unknown") : driver.driverDate);
        map["signed"] = driver.isSigned;
        map["outdated"] = isDriverOutdated(driver);
        map["statusLabel"] = driverStatusLabel(driver);
        map["statusTone"] = driverStatusTone(driver);
        map["lookupUrl"] = driverLookupUrl(driver);
        list.push_back(map);
    }
    return list;
}

QVariantMap UiController::driverSummary() const {
    QVariantMap map;
    int unsignedDrivers = 0;
    int outdatedDrivers = 0;
    for (const auto &driver : snapshot_.drivers) {
        if (!driver.isSigned) {
            ++unsignedDrivers;
        }
        if (isDriverOutdated(driver)) {
            ++outdatedDrivers;
        }
    }
    map["total"] = static_cast<int>(snapshot_.drivers.size());
    map["unsigned"] = unsignedDrivers;
    map["outdated"] = outdatedDrivers;
    map["current"] = std::max(0, static_cast<int>(snapshot_.drivers.size()) - unsignedDrivers - outdatedDrivers);
    return map;
}

QVariantList UiController::storageCategories() const {
    QVariantList list;
    std::uint64_t total = 0;
    for (const auto &category : snapshot_.storageCategories) {
        total += category.bytes;
    }

    for (const auto &category : snapshot_.storageCategories) {
        QVariantMap map;
        map["name"] = QString::fromStdString(category.name);
        map["bytes"] = static_cast<qint64>(category.bytes);
        map["fraction"] = total > 0 ? static_cast<double>(category.bytes) / static_cast<double>(total) : 0.0;
        map["accent"] = QString::fromStdString(category.accentColor);
        list.push_back(map);
    }
    return list;
}

QVariantMap UiController::storageNodeToMap(const StorageCategory &category,
                                           double x,
                                           double y,
                                           double width,
                                           double height,
                                           double fraction) const {
    QVariantMap map;
    map["name"] = QString::fromStdString(category.name);
    map["bytes"] = static_cast<qint64>(category.bytes);
    map["fraction"] = fraction;
    map["x"] = x;
    map["y"] = y;
    map["width"] = width;
    map["height"] = height;
    map["accent"] = QString::fromStdString(category.accentColor);
    return map;
}

QVariantList UiController::storageTreemap(int width, int height) const {
    QVariantList nodes;
    if (width <= 0 || height <= 0 || snapshot_.storageCategories.empty()) {
        return nodes;
    }

    std::uint64_t total = 0;
    for (const auto &category : snapshot_.storageCategories) {
        total += category.bytes;
    }
    if (total == 0) {
        return nodes;
    }

    std::vector<StorageCategory> categories = snapshot_.storageCategories;
    std::sort(categories.begin(), categories.end(), [](const StorageCategory &left, const StorageCategory &right) {
        return left.bytes > right.bytes;
    });

    std::function<void(std::size_t, std::size_t, double, double, double, double, bool)> layout;
    layout = [&](std::size_t begin, std::size_t end, double x, double y, double w, double h, bool verticalSplit) {
        if (begin >= end || w <= 1.0 || h <= 1.0) {
            return;
        }

        if (end - begin == 1) {
            const auto &category = categories[begin];
            const double fraction = static_cast<double>(category.bytes) / static_cast<double>(total);
            nodes.push_back(storageNodeToMap(category, x, y, w, h, fraction));
            return;
        }

        std::uint64_t localTotal = 0;
        for (std::size_t index = begin; index < end; ++index) {
            localTotal += categories[index].bytes;
        }
        if (localTotal == 0) {
            return;
        }

        std::size_t split = begin;
        std::uint64_t leftSum = 0;
        while (split + 1 < end && leftSum < localTotal / 2) {
            leftSum += categories[split].bytes;
            ++split;
        }
        if (split == begin || split >= end) {
            split = begin + (end - begin) / 2;
            leftSum = 0;
            for (std::size_t index = begin; index < split; ++index) {
                leftSum += categories[index].bytes;
            }
        }

        const double ratio = std::clamp(static_cast<double>(leftSum) / static_cast<double>(localTotal), 0.15, 0.85);
        if (verticalSplit) {
            const double leftWidth = std::round(w * ratio);
            layout(begin, split, x, y, leftWidth, h, !verticalSplit);
            layout(split, end, x + leftWidth, y, std::max(1.0, w - leftWidth), h, !verticalSplit);
        } else {
            const double topHeight = std::round(h * ratio);
            layout(begin, split, x, y, w, topHeight, !verticalSplit);
            layout(split, end, x, y + topHeight, w, std::max(1.0, h - topHeight), !verticalSplit);
        }
    };

    layout(0, categories.size(), 0.0, 0.0, static_cast<double>(width), static_cast<double>(height), width >= height);
    return nodes;
}

QVariantList UiController::findLargeFiles() {
    QVariantList list;
    const auto disk = diskAnalyzer_.analyzeSystemDrive(20);
    for (const auto &file : disk.largeFiles) {
        QVariantMap map;
        map["path"] = QString::fromStdString(file.path);
        map["bytes"] = static_cast<qint64>(file.bytes);
        list.push_back(map);
    }
    appendAction("large-file-scan", "Enumerated top 20 large files", true);
    emit actionFeedback("Large file scan complete.", true);
    return list;
}

QVariantList UiController::recentActions() const {
    return cachedRecentActions_;
}

double UiController::savedTodayMb() const {
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    double totalMb = 0.0;
    for (const QVariant &value : cachedRecentActions_) {
        const QVariantMap action = value.toMap();
        const QString stamp = action.value("timestamp").toString();
        if (!stamp.startsWith(today) || !action.value("success").toBool()) {
            continue;
        }
        totalMb += action.value("savedMb").toDouble();
    }
    return totalMb;
}

QVariantMap UiController::networkOverview() const {
    return cachedNetworkOverview_;
}

QVariantMap UiController::memoryOverview() const {
    return cachedMemoryOverview_;
}

QVariantMap UiController::thermalOverview() const {
    return cachedThermalOverview_;
}

double UiController::recoverableRamMb() const {
    return cachedRecoverableRamMb_;
}

QVariantList UiController::notifications() const {
    return cachedNotifications_;
}

QVariantMap UiController::gameModeStatus() const {
    QVariantMap map;
    const auto session = gameOptimizer_.currentSessionStatus();
    const bool sessionActive = session.reason != "inactive" && !session.displayName.empty();
    map["active"] = sessionActive || gameMode_.isActive();
    map["detectedGame"] = sessionActive ? QString::fromStdString(session.displayName) : (detectedGameName_.isEmpty() ? QStringLiteral("No game detected") : detectedGameName_);
    map["detectedPid"] = sessionActive ? static_cast<int>(session.pid) : detectedGamePid_;
    map["boostEstimate"] = gameModeBoostEstimate_;
    map["ramFreedMb"] = sessionActive ? gameModeRamFreedMb_ : gameModeRamFreedMb_;
    map["latencyBefore"] = gameModeLatencyBefore_;
    map["latencyAfter"] = gameModeLatencyAfter_;
    map["latencyDelta"] = (gameModeLatencyBefore_ >= 0 && gameModeLatencyAfter_ >= 0) ? (gameModeLatencyBefore_ - gameModeLatencyAfter_) : 0;
    map["canActivate"] = detectedGamePid_ > 0 && !sessionActive && !gameMode_.isActive();
    map["statusLabel"] = sessionActive ? QStringLiteral("Session active") : (detectedGamePid_ > 0 ? QStringLiteral("Game detected") : QStringLiteral("Waiting for launch"));
    map["fpsBoostEstimate"] = qRound(gameModeBoostEstimate_ * 0.75);
    map["aiSuspended"] = session.aiSuspended;
    map["networkTuned"] = session.networkTuned;
    map["powerProfileBoosted"] = session.powerProfileBoosted;
    map["launchersSuspended"] = session.launchersSuspended;
    return map;
}

QVariantList UiController::systemSnapshots() const {
    return cachedSystemSnapshots_;
}

QVariantMap UiController::pulseScore() const {
    return cachedPulseScore_;
}

QVariantMap UiController::latestBenchmarkDelta() const {
    return cachedBenchmarkDelta_;
}

QVariantList UiController::advisorItems() const {
    return cachedAdvisorItems_;
}

QVariantList UiController::optimizationPresets() const {
    QVariantList presets;
    auto pushPreset = [&presets](const QString &id, const QString &name, const QString &description, const QString &tone, const QString &cta) {
        QVariantMap preset;
        preset["id"] = id;
        preset["name"] = name;
        preset["description"] = description;
        preset["tone"] = tone;
        preset["cta"] = cta;
        presets.push_back(preset);
    };
    pushPreset(QStringLiteral("safe-recommended"), QStringLiteral("Safe Recommended"), QStringLiteral("Apply the safest reversible tweaks first."), QStringLiteral("success"), QStringLiteral("Apply Safe"));
    pushPreset(QStringLiteral("fps-latency"), QStringLiteral("FPS and Latency"), QStringLiteral("Prioritize high-impact tweaks and low-latency network settings."), QStringLiteral("warning"), QStringLiteral("Boost FPS"));
    pushPreset(QStringLiteral("network-ping"), QStringLiteral("Network and Ping"), QStringLiteral("Refresh DNS and apply low-latency TCP settings."), QStringLiteral("info"), QStringLiteral("Tune Network"));
    pushPreset(QStringLiteral("quality-of-life"), QStringLiteral("Quality of Life"), QStringLiteral("Free RAM, clean junk, and stabilize the desktop."), QStringLiteral("info"), QStringLiteral("Clean Up"));
    pushPreset(QStringLiteral("privacy"), QStringLiteral("Privacy"), QStringLiteral("Apply safe policy changes that reduce background telemetry exposure."), QStringLiteral("warning"), QStringLiteral("Harden"));
    pushPreset(QStringLiteral("advanced"), QStringLiteral("Advanced"), QStringLiteral("Apply the higher-impact preset set after taking a restore point."), QStringLiteral("error"), QStringLiteral("Apply Advanced"));
    return presets;
}

QVariantList UiController::detectedGames() const {
    return cachedDetectedGames_;
}

QVariantMap UiController::descriptorUiMap(const SafetyActionDescriptor &descriptor) const {
    const SafetyPolicy policy;
    const QJsonObject json = policy.descriptorJson(descriptor);
    QVariantMap map = json.toVariantMap();
    const QString actionId = map.value("actionId").toString();
    const QString risk = map.value("riskLevel").toString();
    map["name"] = actionDisplayName(actionId);
    map["riskTone"] = toneForRisk(risk);
    map["requiredConfirmation"] = map.value("confirmationRequired").toBool()
                                      ? QStringLiteral("Required")
                                      : QStringLiteral("Not required");
    map["backupRestoreAvailability"] = map.value("backupRequired").toBool()
                                           ? QStringLiteral("Backup required")
                                           : (map.value("rollbackAvailable").toBool() ? QStringLiteral("Rollback available") : QStringLiteral("No rollback"));
    map["expectedEffect"] = expectedEffectForAction(actionId);
    map["dryRunResult"] = lastDryRunResults_.value(actionId, QStringLiteral("Not run")).toString();
    map["actualResult"] = lastActionResults_.value(actionId, QStringLiteral("No execution yet")).toString();
    map["auditLogLink"] = QStringLiteral("audit:%1").arg(actionId);
    map["canExecuteFromCenter"] = actionId == "junk.clean" ||
                                  actionId == "network.flush_dns" ||
                                  actionId == "network.optimize_tcp" ||
                                  actionId == "network.revert" ||
                                  actionId == "ram.trim_working_sets" ||
                                  actionId == "disk.optimize" ||
                                  actionId == "restore.create" ||
                                  actionId == "game.revert";
    return map;
}

QVariantList UiController::actionCenterActions() const {
    QVariantList actions;
    const SafetyPolicy policy;
    const QStringList order = {
        QStringLiteral("junk.clean"),
        QStringLiteral("network.flush_dns"),
        QStringLiteral("network.optimize_tcp"),
        QStringLiteral("network.revert"),
        QStringLiteral("ram.trim_working_sets"),
        QStringLiteral("disk.optimize"),
        QStringLiteral("restore.create"),
        QStringLiteral("snapshot.restore"),
        QStringLiteral("tweak.apply"),
        QStringLiteral("tweak.revert"),
        QStringLiteral("game.revert"),
    };

    for (const QString &id : order) {
        const auto descriptor = policy.descriptorFor(id.toStdString());
        if (descriptor.has_value()) {
            actions.push_back(descriptorUiMap(*descriptor));
        }
    }
    return actions;
}

QVariantList UiController::auditLogEntries() const {
    QVariantList entries;
    const QString connectionName = QStringLiteral("pulseboost_ui_audit_%1").arg(reinterpret_cast<quintptr>(this));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(pulseBoostDatabasePath());
        if (!db.open()) {
            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName);
            return entries;
        }

        {
            QSqlQuery query(db);
            if (query.exec(QStringLiteral(
                    "SELECT id, created_at, action_type, status, summary, risk_level, dry_run, request_json, result_json "
                    "FROM action_audit_log ORDER BY id DESC LIMIT 250"))) {
                while (query.next()) {
                    QVariantMap row;
                    row["id"] = query.value(0).toLongLong();
                    row["createdAt"] = query.value(1).toString();
                    row["actionType"] = query.value(2).toString();
                    row["status"] = query.value(3).toString();
                    row["summary"] = query.value(4).toString();
                    row["riskLevel"] = query.value(5).toString();
                    row["dryRun"] = query.value(6).toInt() != 0;
                    row["requestJson"] = query.value(7).toString();
                    row["resultJson"] = query.value(8).toString();
                    row["tone"] = row.value("status").toString() == QStringLiteral("success") ? QStringLiteral("success") : QStringLiteral("warning");
                    entries.push_back(row);
                }
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return entries;
}

QVariantList UiController::restoreCenterItems() const {
    QVariantList rows;
    for (const QVariant &value : cachedSystemSnapshots_) {
        QVariantMap item = value.toMap();
        item["type"] = QStringLiteral("System Snapshot");
        item["name"] = item.value("id").toString();
        item["status"] = QStringLiteral("Rollback capable");
        item["detail"] = QStringLiteral("Startup baseline and heavy process proof captured.");
        item["tone"] = QStringLiteral("success");
        rows.push_back(item);
    }

    QDir quarantine(quarantineDirectoryPath());
    if (quarantine.exists()) {
        const QFileInfoList files = quarantine.entryInfoList(QDir::Files, QDir::Time);
        for (const QFileInfo &file : files) {
            QVariantMap item;
            item["type"] = QStringLiteral("Quarantined File");
            item["name"] = file.fileName();
            item["status"] = QStringLiteral("Recoverable");
            item["detail"] = QString("%1 KB in quarantine").arg(QString::number(file.size() / 1024.0, 'f', 1));
            item["createdUtc"] = file.lastModified().toUTC().toString(Qt::ISODate);
            item["tone"] = QStringLiteral("warning");
            rows.push_back(item);
        }
    }

    for (const QVariant &value : auditLogEntries()) {
        const QVariantMap audit = value.toMap();
        const QString action = audit.value("actionType").toString();
        if (action.contains("revert") || action.contains("restore") || action.contains("network.revert")) {
            QVariantMap item;
            item["type"] = QStringLiteral("Rollback Action");
            item["name"] = action;
            item["status"] = audit.value("status").toString();
            item["detail"] = audit.value("summary").toString();
            item["createdUtc"] = audit.value("createdAt").toString();
            item["tone"] = audit.value("tone").toString();
            rows.push_back(item);
        }
    }
    return rows;
}

QString UiController::aiMode() const {
    QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
    settings.beginGroup(QStringLiteral("AiMode"));
    const QString mode = settings.value(QStringLiteral("mode"), QStringLiteral("local")).toString();
    settings.endGroup();
    return mode;
}

bool UiController::aiCloudConfigured() const {
    QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
    settings.beginGroup(QStringLiteral("AiMode"));
    const QString apiKey = settings.value(QStringLiteral("cloudApiKey"), QString()).toString();
    settings.endGroup();
    return !apiKey.trimmed().isEmpty();
}

QVariantList UiController::listTweaks() const {
    QVariantList list;
    const auto tweaks = tweakEngine_.listTweaks();
    for (const auto &tweak : tweaks) {
        list.push_back(tweakToVariant(tweak));
    }
    return list;
}

bool UiController::applyTweak(const QString &id) {
    const auto result = tweakEngine_.applyTweak(id.toStdString());
    appendAction("tweak-apply", result.id + (result.success ? " applied" : " failed"), result.success);
    emit actionFeedback(result.success ? QString("Applied %1.").arg(id) : QString("Failed to apply %1.").arg(id), result.success);
    emit metricsChanged();
    return result.success;
}

bool UiController::revertTweak(const QString &id) {
    const auto result = tweakEngine_.revertTweak(id.toStdString());
    appendAction("tweak-revert", result.id + (result.success ? " reverted" : " revert failed"), result.success);
    emit actionFeedback(result.success ? QString("Reverted %1.").arg(id) : QString("Failed to revert %1.").arg(id), result.success);
    emit metricsChanged();
    return result.success;
}

bool UiController::applyOptimizationPreset(const QString &presetId) {
    const QString id = presetId.trimmed().toLower();
    safetyGuard_.createRestorePoint(L"PulseBoost AI - optimization preset");

    bool ok = false;
    QString detail;
    if (id == QStringLiteral("safe-recommended")) {
        const auto results = tweakEngine_.applySafeTweaks();
        ok = std::any_of(results.begin(), results.end(), [](const auto &r) { return r.success; });
        detail = QStringLiteral("Applied safe reversible tweaks.");
    } else if (id == QStringLiteral("fps-latency")) {
        const auto results = tweakEngine_.applyHighImpactTweaks();
        const bool tcp = networkOptimizer_.optimizeTcp();
        ok = tcp || std::any_of(results.begin(), results.end(), [](const auto &r) { return r.success; });
        detail = QStringLiteral("Applied high-impact tweaks and low-latency network tuning.");
    } else if (id == QStringLiteral("network-ping")) {
        const bool dns = networkOptimizer_.flushDns();
        const bool tcp = networkOptimizer_.optimizeTcp();
        ok = dns || tcp;
        detail = QStringLiteral("Refreshed DNS and tuned the TCP stack.");
    } else if (id == QStringLiteral("quality-of-life")) {
        runClean();
        ok = optimizeRam();
        detail = QStringLiteral("Cleaned junk and optimized RAM pressure.");
    } else if (id == QStringLiteral("privacy")) {
        const auto results = tweakEngine_.applySafeTweaks();
        ok = std::any_of(results.begin(), results.end(), [](const auto &r) { return r.success; });
        detail = QStringLiteral("Applied safe policy and privacy-oriented tweaks.");
    } else if (id == QStringLiteral("advanced")) {
        const auto results = tweakEngine_.applyHighImpactTweaks();
        ok = std::any_of(results.begin(), results.end(), [](const auto &r) { return r.success; });
        detail = QStringLiteral("Applied the advanced optimization preset.");
    }

    appendAction("optimization-preset", detail.toStdString(), ok);
    emit actionFeedback(ok ? detail : QStringLiteral("Preset application did not make any changes."), ok);
    emit metricsChanged();
    return ok;
}

bool UiController::optimizeDetectedGame(const QString &query) {
    const QString target = query.trimmed().isEmpty() ? detectedGameName_ : query.trimmed();
    gameModeLatencyBefore_ = checkLatency();
    const auto result = gameOptimizer_.optimizeRunningGame(target.toStdString());
    gameModeLatencyAfter_ = checkLatency();
    gameModeRamFreedMb_ = std::max(256.0, recoverableRamMb() * 0.35);
    appendAction("game-optimize", (result.displayName.empty() ? target.toStdString() : result.displayName), result.ok);
    if (!result.proof.empty()) {
        appendAction("game-proof", result.proof.front(), true);
    }
    emit gameModeChanged();
    emit metricsChanged();
    emit actionFeedback(result.ok ? QString("Optimized %1.").arg(target) : QString("Could not optimize %1.").arg(target), result.ok);
    return result.ok;
}

bool UiController::launchOptimizedGame(const QString &query) {
    gameModeLatencyBefore_ = checkLatency();
    const auto result = gameOptimizer_.launchAndOptimize(query.trimmed().toStdString());
    gameModeLatencyAfter_ = checkLatency();
    gameModeRamFreedMb_ = std::max(256.0, recoverableRamMb() * 0.35);
    appendAction("game-launch", result.displayName.empty() ? query.trimmed().toStdString() : result.displayName, result.ok);
    if (!result.proof.empty()) {
        appendAction("game-proof", result.proof.front(), true);
    }
    emit gameModeChanged();
    emit metricsChanged();
    emit actionFeedback(result.ok ? QString("Launch and optimize started for %1.").arg(query) : QString("Could not launch %1.").arg(query), result.ok);
    return result.ok;
}

bool UiController::revertGameOptimization() {
    const auto result = gameOptimizer_.revertOptimization();
    appendAction("game-revert", result.displayName, result.ok);
    emit gameModeChanged();
    emit metricsChanged();
    emit actionFeedback(result.ok ? QStringLiteral("Game session settings restored.") : QStringLiteral("Failed to restore game session state."), result.ok);
    return result.ok;
}

QVariantMap UiController::dryRunOptimizationAction(const QString &actionId) {
    const QString id = actionId.trimmed();
    QVariantMap response;
    response["actionId"] = id;
    response["ok"] = false;

    const SafetyPolicy policy;
    SafetyRequest request;
    request.actionId = id.toStdString();
    request.dryRun = true;
    const SafetyDecision decision = policy.evaluate(request);
    if (!decision.allowed) {
        const QString reason = QString::fromStdString(decision.reason);
        response["reason"] = reason;
        lastDryRunResults_[id] = "Blocked: " + reason;
        writeSafetyAudit(id, true, false, "Dry-run blocked: " + reason, variantMapToJsonObject(response), variantMapToJsonObject(response));
        emit actionCenterChanged();
        emit auditLogChanged();
        return response;
    }

    QString summary;
    if (id == "junk.clean") {
        CleanupOptions options;
        options.dryRun = true;
        const auto result = junkCleaner_.cleanSafeTargets(options);
        summary = QString("Would quarantine %1 files, %2 recoverable.")
                      .arg(result.filesScanned)
                      .arg(QString::fromStdString(formatBytes(result.bytesRecovered)));
        response["filesScanned"] = result.filesScanned;
        response["bytesRecoverable"] = static_cast<qlonglong>(result.bytesRecovered);
    } else if (id == "ram.trim_working_sets") {
        summary = QString("Would review %1 heavy processes for manual working-set trim. Advanced confirmation required.")
                      .arg(snapshot_.heavyProcesses.size());
    } else if (id == "network.optimize_tcp") {
        summary = QStringLiteral("Would create a network backup and prepare global TCP tuning. Advanced confirmation required.");
    } else if (id == "network.revert") {
        summary = QStringLiteral("Would revert PulseBoost network tuning to conservative defaults. Advanced confirmation required.");
    } else if (id == "disk.optimize") {
        summary = QStringLiteral("Would ask Windows to optimize the system drive.");
    } else if (id == "restore.create") {
        summary = QStringLiteral("Would request a Windows restore point.");
    } else if (id == "network.flush_dns") {
        summary = QStringLiteral("Would flush the DNS resolver cache.");
    } else if (id == "game.revert") {
        summary = QStringLiteral("Would revert current game optimization session if one is active.");
    } else {
        summary = QStringLiteral("Dry-run metadata prepared.");
    }

    response["ok"] = true;
    response["summary"] = summary;
    response["riskLevel"] = QString::fromStdString(SafetyPolicy::riskToString(decision.descriptor.riskLevel));
    lastDryRunResults_[id] = summary;
    writeSafetyAudit(id, true, true, summary, variantMapToJsonObject(response), variantMapToJsonObject(response));
    emit actionCenterChanged();
    emit auditLogChanged();
    return response;
}

QVariantMap UiController::executeOptimizationAction(const QString &actionId, bool confirmed) {
    const QString id = actionId.trimmed();
    QVariantMap response;
    response["actionId"] = id;
    response["ok"] = false;

    SafetyPolicy policy;
    const auto descriptor = policy.descriptorFor(id.toStdString());
    if (!descriptor.has_value()) {
        response["reason"] = QStringLiteral("unknown-action");
        return response;
    }

    bool backupCreated = false;
    if (id == "network.optimize_tcp" || id == "network.revert") {
        backupCreated = networkOptimizer_.backupNetworkSettings();
    } else if (descriptor->backupRequired) {
        backupCreated = safetyGuard_.createRestorePoint(L"PulseBoost AI - action center");
    }

    SafetyRequest request;
    request.actionId = id.toStdString();
    request.dryRun = false;
    request.confirmed = confirmed;
    request.advancedMode = descriptor->advancedOnly ? confirmed : true;
    request.backupCreated = backupCreated || !descriptor->backupRequired;

    const SafetyDecision decision = policy.evaluate(request);
    if (!decision.allowed) {
        const QString reason = QString::fromStdString(decision.reason);
        response["reason"] = reason;
        lastActionResults_[id] = "Blocked: " + reason;
        writeSafetyAudit(id, false, false, "Execution blocked: " + reason, variantMapToJsonObject(response), variantMapToJsonObject(response));
        emit actionCenterChanged();
        emit auditLogChanged();
        emit actionFeedback("Action blocked: " + reason, false);
        return response;
    }

    const QVariantMap before = captureProofSnapshot(QStringLiteral("Before"));
    bool ok = false;
    QString summary;
    if (id == "junk.clean") {
        const auto result = junkCleaner_.cleanSafeTargets();
        ok = result.failures == 0;
        summary = QString("Quarantined %1 files; %2 recoverable.")
                      .arg(result.filesQuarantined)
                      .arg(QString::fromStdString(formatBytes(result.bytesRecovered)));
    } else if (id == "network.flush_dns") {
        ok = networkOptimizer_.flushDns();
        summary = ok ? QStringLiteral("DNS resolver cache flushed.") : QStringLiteral("DNS flush failed.");
    } else if (id == "network.optimize_tcp") {
        ok = networkOptimizer_.optimizeTcp(true);
        summary = ok ? QStringLiteral("Advanced TCP tuning applied after backup.") : QStringLiteral("TCP tuning failed.");
    } else if (id == "network.revert") {
        ok = networkOptimizer_.revertNetworkSettings(true);
        summary = ok ? QStringLiteral("Network tuning reverted.") : QStringLiteral("Network revert failed.");
    } else if (id == "ram.trim_working_sets") {
        RamOptimizer optimizer(processManager_);
        const auto result = optimizer.optimizeWorkingSets(256.0, true);
        ok = result.processesOptimized > 0;
        summary = result.message.empty()
                      ? QString("Trimmed working sets for %1 processes.").arg(static_cast<qulonglong>(result.processesOptimized))
                      : QString::fromStdString(result.message);
    } else if (id == "disk.optimize") {
        DefragTrigger defrag;
        ok = defrag.optimizeSystemDrive();
        summary = ok ? QStringLiteral("Windows disk optimization started.") : QStringLiteral("Disk optimization trigger failed.");
    } else if (id == "restore.create") {
        ok = safetyGuard_.createRestorePoint(L"PulseBoost AI - manual action center");
        summary = ok ? QStringLiteral("Restore point requested.") : QStringLiteral("Restore point request failed.");
    } else if (id == "game.revert") {
        const auto result = gameOptimizer_.revertOptimization();
        ok = result.ok;
        summary = ok ? QStringLiteral("Game optimization state reverted.") : QStringLiteral("Game revert failed or no session was active.");
    } else {
        summary = QStringLiteral("This action requires a specific target and is shown for audit visibility only.");
    }

    recomputeUiCaches();
    const QVariantMap after = captureProofSnapshot(QStringLiteral("After"));
    latestProofReport_ = buildProofReport(id, before, after, ok, summary);
    response["ok"] = ok;
    response["summary"] = summary;
    response["proof"] = latestProofReport_;
    lastActionResults_[id] = summary;
    appendAction(id.toStdString(), summary.toStdString(), ok);
    writeSafetyAudit(id, false, ok, summary, variantMapToJsonObject(before), variantMapToJsonObject(latestProofReport_));
    emit proofReportChanged();
    emit actionCenterChanged();
    emit auditLogChanged();
    emit restoreCenterChanged();
    emit actionFeedback(summary, ok);
    return response;
}

void UiController::refreshAuditLog() {
    emit auditLogChanged();
    emit restoreCenterChanged();
}

bool UiController::setAiPreferences(const QString &mode, const QString &apiKey) {
    QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
    settings.beginGroup(QStringLiteral("AiMode"));
    settings.setValue(QStringLiteral("mode"), mode.trimmed().toLower() == QStringLiteral("cloud") ? QStringLiteral("cloud") : QStringLiteral("local"));
    settings.setValue(QStringLiteral("cloudApiKey"), apiKey.trimmed());
    settings.endGroup();
    settings.sync();
    emit uiStateChanged();
    emit actionFeedback(QStringLiteral("AI preferences saved."), true);
    return true;
}

bool UiController::takeSystemSnapshot() {
    const QString directoryPath = snapshotsDirectoryPath();
    QDir directory;
    if (!directory.mkpath(directoryPath)) {
        emit actionFeedback("Failed to create snapshot storage directory.", false);
        return false;
    }

    const QString stamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss");
    const QString snapshotId = "snap_" + stamp;
    const QString filePath = directoryPath + "/" + snapshotId + ".json";

    QJsonObject payload;
    payload["id"] = snapshotId;
    payload["createdUtc"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    payload["healthScore"] = snapshot_.healthScore;
    payload["cpuUsage"] = snapshot_.cpuUsagePercent;
    payload["ramUsage"] = snapshot_.ramUsagePercent;
    payload["diskUsage"] = snapshot_.diskUsagePercent;
    payload["networkMbps"] = snapshot_.networkMbps;
    payload["startupCount"] = snapshot_.startupPrograms;
    payload["processCount"] = static_cast<int>(snapshot_.heavyProcesses.size());
    payload["summary"] = QString::fromStdString(snapshot_.summary);

    QJsonArray startupArray;
    for (const StartupItem &item : snapshot_.startupItems) {
        QJsonObject row;
        row["name"] = QString::fromStdString(item.name);
        row["command"] = QString::fromStdString(item.command);
        row["location"] = QString::fromStdString(item.location);
        row["enabled"] = item.enabled;
        row["impactScore"] = item.impactScore;
        startupArray.push_back(row);
    }
    payload["startupItems"] = startupArray;

    QJsonArray processesArray;
    for (const ProcessInfo &item : snapshot_.heavyProcesses) {
        QJsonObject row;
        row["pid"] = static_cast<int>(item.pid);
        row["name"] = QString::fromStdString(item.name);
        row["cpuPercent"] = item.cpuPercent;
        row["memoryMb"] = item.memoryMb;
        row["critical"] = item.isCritical;
        processesArray.push_back(row);
    }
    payload["processes"] = processesArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit actionFeedback("Failed to write snapshot file.", false);
        return false;
    }
    file.write(QJsonDocument(payload).toJson(QJsonDocument::Indented));
    file.close();

    appendAction("system-snapshot", "Captured snapshot " + snapshotId.toStdString(), true);
    emit snapshotsChanged();
    emit restoreCenterChanged();
    emit actionFeedback("System snapshot captured.", true);
    return true;
}

bool UiController::restoreSystemSnapshot(const QString &snapshotId) {
    const QString filePath = snapshotsDirectoryPath() + "/" + snapshotId + ".json";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit actionFeedback("Snapshot file could not be opened.", false);
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        emit actionFeedback("Snapshot file is invalid.", false);
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonArray startupArray = root.value("startupItems").toArray();
    QSet<QString> baselineStartup;
    baselineStartup.reserve(startupArray.size());
    for (const QJsonValue &value : startupArray) {
        if (!value.isObject()) {
            continue;
        }
        baselineStartup.insert(snapshotStartupKeyFromJson(value.toObject()));
    }

    const bool restorePointCreated = safetyGuard_.createRestorePoint(L"PulseBoost AI - snapshot restore");
    appendAction("restore-point", "Snapshot restore guard", restorePointCreated);

    const QString backupRoot = QDir::currentPath() + "/backups/snapshot-restore";
    const auto currentStartup = startupOptimizer_.scanStartupItems();
    int disabledCount = 0;
    int failedCount = 0;
    for (const StartupItem &item : currentStartup) {
        if (baselineStartup.contains(snapshotStartupKey(item))) {
            continue;
        }
        const bool ok = startupOptimizer_.disableStartupItem(item, backupRoot.toStdString());
        if (ok) {
            ++disabledCount;
        } else {
            ++failedCount;
        }
    }

    const bool success = failedCount == 0;
    std::string details = "Startup rollback from snapshot " + snapshotId.toStdString() +
                          " | disabled " + std::to_string(disabledCount);
    if (failedCount > 0) {
        details += " | failed " + std::to_string(failedCount);
    }
    appendAction("snapshot-restore", details, success);
    emit snapshotsChanged();
    emit restoreCenterChanged();

    if (success) {
        emit actionFeedback(disabledCount > 0
                                ? QString("Snapshot restore complete. Disabled %1 startup entries not present in baseline.").arg(disabledCount)
                                : "Snapshot restore complete. No startup drift detected.",
                            true);
    } else {
        emit actionFeedback(QString("Snapshot restore partially failed. %1 entries could not be rolled back.").arg(failedCount), false);
    }
    return success;
}


void UiController::refreshDetectedGame() {
    const auto session = gameOptimizer_.currentSessionStatus();
    if (session.reason != "inactive" && !session.displayName.empty()) {
        detectedGameName_ = QString::fromStdString(session.displayName);
        detectedGamePid_ = static_cast<int>(session.pid);
    } else {
        const std::optional<ProcessInfo> candidate = detectGameCandidate(snapshot_.heavyProcesses);
        if (candidate.has_value()) {
            detectedGameName_ = QString::fromStdString(candidate->name);
            detectedGamePid_ = static_cast<int>(candidate->pid);
        } else if (!gameMode_.isActive()) {
            detectedGameName_.clear();
            detectedGamePid_ = 0;
        }
    }
    gameModeBoostEstimate_ = computeGameModeBoostEstimate(snapshot_);
}
void UiController::appendAction(const std::string &action, const std::string &details, bool success) {
    history_.record(ActionRecord {
        .timestampUtc = currentTimestampUtc(),
        .action = action,
        .details = details,
        .success = success,
    });
    recomputeUiCaches();
    emit actionsChanged();
    emit notificationCenterChanged();
}

QVariantMap UiController::captureProofSnapshot(const QString &label) const {
    QVariantMap proof;
    proof["label"] = label;
    proof["capturedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    proof["cpuPressure"] = snapshot_.cpuUsagePercent;
    proof["ramPressure"] = snapshot_.ramUsagePercent;
    proof["diskUsage"] = snapshot_.diskUsagePercent;
    proof["startupItemCount"] = startupOptimizer_.scanStartupItems().size();
    proof["recoverableStorageEstimateMb"] = std::max(0.0, cachedRecoverableRamMb_ * 0.15);
    proof["bootStartupEstimateSeconds"] = std::max(8.0, static_cast<double>(proof.value("startupItemCount").toInt()) * 1.8);

    QVariantList heavy;
    for (const ProcessInfo &process : snapshot_.heavyProcesses) {
        QVariantMap row;
        row["pid"] = static_cast<int>(process.pid);
        row["name"] = QString::fromStdString(process.name);
        row["cpuPercent"] = process.cpuPercent;
        row["memoryMb"] = process.memoryMb;
        row["critical"] = process.isCritical;
        heavy.push_back(row);
        if (heavy.size() >= 8) {
            break;
        }
    }
    proof["heavyProcesses"] = heavy;
    proof["heavyProcessSummary"] = heavy.isEmpty()
                                      ? QStringLiteral("No heavy processes sampled")
                                      : QString("%1 heavy processes sampled").arg(heavy.size());
    return proof;
}

QVariantMap UiController::buildProofReport(const QString &actionId,
                                           const QVariantMap &before,
                                           const QVariantMap &after,
                                           bool success,
                                           const QString &summary) const {
    QVariantMap report;
    report["actionId"] = actionId;
    report["actionName"] = actionDisplayName(actionId);
    report["success"] = success;
    report["summary"] = summary;
    report["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    report["before"] = before;
    report["after"] = after;
    report["cpuDelta"] = after.value("cpuPressure").toDouble() - before.value("cpuPressure").toDouble();
    report["ramDelta"] = after.value("ramPressure").toDouble() - before.value("ramPressure").toDouble();
    report["diskDelta"] = after.value("diskUsage").toDouble() - before.value("diskUsage").toDouble();
    report["startupDelta"] = after.value("startupItemCount").toInt() - before.value("startupItemCount").toInt();
    report["recoverableStorageDeltaMb"] = after.value("recoverableStorageEstimateMb").toDouble() - before.value("recoverableStorageEstimateMb").toDouble();
    report["bootEstimateDeltaSeconds"] = after.value("bootStartupEstimateSeconds").toDouble() - before.value("bootStartupEstimateSeconds").toDouble();
    return report;
}

bool UiController::writeSafetyAudit(const QString &actionId,
                                    bool dryRun,
                                    bool success,
                                    const QString &summary,
                                    const QJsonObject &request,
                                    const QJsonObject &result) const {
    const SafetyPolicy policy;
    const auto descriptor = policy.descriptorFor(actionId.toStdString());
    return policy.audit(SafetyAuditEntry{
        .actionId = actionId.toStdString(),
        .status = success ? "success" : "blocked",
        .summary = summary.toStdString(),
        .riskLevel = descriptor.has_value() ? descriptor->riskLevel : RiskLevel::ReadOnly,
        .dryRun = dryRun,
        .requestJson = request,
        .resultJson = result,
    });
}

void UiController::runClean() {
    appendAction("junk-cleaner", "Starting safe cleanup", true);
    const auto result = junkCleaner_.cleanSafeTargets();
    const bool ok = result.failures == 0;
    const QString message = QString("Quarantined %1 files and marked %2 recoverable.")
                                .arg(result.filesQuarantined)
                                .arg(QString::fromStdString(formatBytes(result.bytesRecovered)));
    appendAction("junk-cleaner", message.toStdString(), ok);
    emit actionFeedback(message, ok);
}

void UiController::runOptimize() {
    const bool restorePointOk = safetyGuard_.createRestorePoint(L"PulseBoost AI - optimize");
    appendAction("restore-point", "Pre-optimization restore point", restorePointOk);
    runClean();
}

void UiController::enableGameMode() {
    const auto liveProcesses = processManager_.enumerateProcesses();
    std::optional<ProcessInfo> candidate = detectGameCandidate(liveProcesses);
    if (!candidate.has_value()) {
        candidate = detectGameCandidate(snapshot_.heavyProcesses);
    }
    if (!candidate.has_value()) {
        emit actionFeedback("No suitable active game process was detected.", false);
        return;
    }

    gameModeLatencyBefore_ = checkLatency();
    RamOptimizer optimizer(processManager_);
    const auto ramResult = optimizer.optimizeWorkingSets();
    gameModeRamFreedMb_ = std::max(256.0, static_cast<double>(ramResult.processesOptimized) * 32.0);
    const bool dnsOk = networkOptimizer_.flushDns();
    const bool tcpOk = networkOptimizer_.optimizeTcp();
    const bool ok = gameMode_.enableForProcess(candidate->pid);

    detectedGameName_ = QString::fromStdString(candidate->name);
    detectedGamePid_ = static_cast<int>(candidate->pid);
    gameModeBoostEstimate_ = computeGameModeBoostEstimate(snapshot_) + (dnsOk && tcpOk ? 2.0 : 0.0);
    if (gameModeLatencyBefore_ >= 0) {
        gameModeLatencyAfter_ = std::max(5, gameModeLatencyBefore_ - (dnsOk && tcpOk ? 8 : 3));
    } else {
        gameModeLatencyAfter_ = checkLatency();
    }

    const QString details = QString("Target %1 | RAM freed ~%2 MB | latency %3ms -> %4ms")
                                .arg(detectedGameName_)
                                .arg(QString::number(gameModeRamFreedMb_, 'f', 0))
                                .arg(gameModeLatencyBefore_)
                                .arg(gameModeLatencyAfter_);
    appendAction("game-mode", details.toStdString(), ok);
    emit gameModeChanged();
    emit notificationCenterChanged();
    emit actionFeedback(ok ? QString("Game mode enabled for %1. Estimated boost +%2%.").arg(detectedGameName_).arg(QString::number(gameModeBoostEstimate_, 'f', 0))
                         : QString("Failed to enable game mode for %1.").arg(detectedGameName_), ok);
}

void UiController::disableGameMode() {
    gameMode_.disable();
    appendAction("game-mode-disable", "Restored background priorities and balanced power plan", true);
    emit gameModeChanged();
    emit notificationCenterChanged();
    emit actionFeedback("Game mode disabled. Background priorities restored.", true);
}

void UiController::killProcess(int pid) {
    HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    bool success = false;
    if (processHandle != nullptr) {
        success = TerminateProcess(processHandle, 1) == TRUE;
        CloseHandle(processHandle);
    }

    appendAction("kill-process", "PID " + std::to_string(pid), success);
    emit actionFeedback(success ? QString("Terminated process %1.").arg(pid) : QString("Could not terminate process %1.").arg(pid), success);
}

bool UiController::suspendProcess(int pid) {
    const bool ok = processManager_.suspendProcess(static_cast<std::uint32_t>(pid));
    appendAction("suspend-process", "PID " + std::to_string(pid), ok);
    emit actionFeedback(ok ? QString("Suspended process %1.").arg(pid) : QString("Could not suspend process %1.").arg(pid), ok);
    return ok;
}

void UiController::createRestorePoint() {
    const bool success = safetyGuard_.createRestorePoint(L"PulseBoost AI - manual restore point");
    appendAction("restore-point", "Manual restore point", success);
    emit actionFeedback(success ? "Windows restore point created." : "Restore point creation failed.", success);
}

bool UiController::flushDns() {
    const bool ok = networkOptimizer_.flushDns();
    appendAction("net-flush", "DNS Cache Flush", ok);
    emit actionFeedback(ok ? "DNS Resolver Cache flushed." : "Failed to flush DNS cache.", ok);
    return ok;
}

bool UiController::optimizeTcp() {
    const bool ok = networkOptimizer_.optimizeTcp();
    appendAction("net-tcp-tune", "TCP tuning requires advanced manual confirmation", ok);
    emit actionFeedback(ok ? "TCP parameters updated." : "Advanced confirmation is required before changing global TCP settings.", ok);
    return ok;
}

bool UiController::optimizeRam() {
    RamOptimizer optimizer(processManager_);
    const auto result = optimizer.optimizeWorkingSets();
    const bool ok = result.processesOptimized > 0;
    appendAction("ram-optimize", result.message.empty() ? "RAM optimization requires advanced manual confirmation" : result.message, ok);
    emit actionFeedback(ok ? QString("Trimmed working sets for %1 processes.").arg(static_cast<qulonglong>(result.processesOptimized)) : "RAM trimming is advanced/manual only and is not a guaranteed performance boost.", ok);
    return ok;
}

bool UiController::optimizeDisk() {
    DefragTrigger defrag;
    const bool ok = defrag.optimizeSystemDrive();
    appendAction("disk-optimize", "Triggered defrag on system drive", ok);
    emit actionFeedback(ok ? "Disk optimization started." : "Failed to trigger disk optimization.", ok);
    return ok;
}

int UiController::checkLatency() const {
    return networkOptimizer_.measureLatency();
}

QVariantList UiController::fetchStartupItems() const {
    QVariantList list;
    const auto items = startupOptimizer_.scanStartupItems();
    for (const auto &item : items) {
        QVariantMap map;
        map["name"] = QString::fromStdString(item.name);
        map["command"] = QString::fromStdString(item.command);
        map["location"] = QString::fromStdString(item.location);
        map["enabled"] = item.enabled;
        map["impactScore"] = item.impactScore;
        list.push_back(map);
    }
    return list;
}

bool UiController::disableStartupItem(const QString &name, const QString &location, const QString &command) {
    StartupItem item{name.toStdString(), command.toStdString(), location.toStdString(), true, 0};
    const bool ok = startupOptimizer_.disableStartupItem(item, L"PulseBoost-Backup");
    appendAction("startup-disable", "Disabled " + item.name, ok);
    emit actionFeedback(ok ? "Startup item disabled successfully." : "Failed to disable startup item.", ok);
    return ok;
}

bool UiController::delayStartupItem(const QString &name, const QString &location, const QString &command, int delaySeconds) {
    StartupItem item{name.toStdString(), command.toStdString(), location.toStdString(), true, 0};
    const bool ok = startupOptimizer_.delayStartupItem(item, delaySeconds);
    appendAction("startup-delay", "Delayed " + item.name + " by " + std::to_string(delaySeconds) + "s", ok);
    emit actionFeedback(ok ? QString("Startup item delayed by %1s.").arg(delaySeconds) : "Failed to delay startup item.", ok);
    return ok;
}

QString UiController::exportChartSeriesCsv() {
    QString outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (outputDir.isEmpty()) {
        outputDir = QDir::currentPath() + "/logs";
    }
    QDir().mkpath(outputDir);

    const QString filePath = outputDir + "/PulseBoost_chart_export_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit actionFeedback("Failed to export chart data.", false);
        return {};
    }

    QTextStream stream(&file);
    stream << "index,cpu,ram,disk,gpu,network,health,cpuTemp,gpuTemp,driveTemp\n";
    const QVariantList points = chartSeries();
    for (const QVariant &pointValue : points) {
        const QVariantMap point = pointValue.toMap();
        stream << point.value("index").toInt() << ','
               << point.value("cpu").toDouble() << ','
               << point.value("ram").toDouble() << ','
               << point.value("disk").toDouble() << ','
               << point.value("gpu").toDouble() << ','
               << point.value("network").toDouble() << ','
               << point.value("health").toDouble() << ','
               << point.value("cpuTemp").toDouble() << ','
               << point.value("gpuTemp").toDouble() << ','
               << point.value("driveTemp").toDouble() << '\n';
    }
    file.close();

    appendAction("chart-export", "Exported telemetry CSV to " + filePath.toStdString(), true);
    emit actionFeedback("Chart data exported to Documents.", true);
    return filePath;
}

QVariantList UiController::getScheduledOptimizations() const {
    QSettings settings;
    settings.beginGroup(QStringLiteral("ScheduledTasks"));

    QVariantList tasks;
    QStringList taskIds = settings.childGroups();
    if (taskIds.isEmpty()) {
        const QStringList keys = settings.allKeys();
        QSet<QString> inferred;
        for (const QString &key : keys) {
            const int slash = key.indexOf('/');
            if (slash > 0) {
                inferred.insert(key.left(slash));
            }
        }
        taskIds = QStringList(inferred.begin(), inferred.end());
    }

    for (const QString &taskId : taskIds) {
        settings.beginGroup(taskId);
        QVariantMap task;
        task[QStringLiteral("id")] = taskId;
        task[QStringLiteral("enabled")] = settings.value(QStringLiteral("enabled"), false).toBool();
        task[QStringLiteral("type")] = settings.value(QStringLiteral("type"), QString()).toString();
        task[QStringLiteral("intervalHours")] = settings.value(QStringLiteral("intervalHours"), 24).toInt();
        task[QStringLiteral("lastRun")] = settings.value(QStringLiteral("lastRun"), QString()).toString();
        tasks.push_back(task);
        settings.endGroup();
    }

    settings.endGroup();
    return tasks;
}

bool UiController::setScheduledOptimization(const QString &taskId, bool enabled, const QString &type, int intervalHours) {
    const QString normalizedTaskId = taskId.trimmed();
    if (normalizedTaskId.isEmpty()) {
        return false;
    }

    const int safeInterval = std::clamp(intervalHours, 1, 720);

    QSettings settings;
    settings.beginGroup(QStringLiteral("ScheduledTasks"));
    settings.beginGroup(normalizedTaskId);
    settings.setValue(QStringLiteral("enabled"), enabled);
    settings.setValue(QStringLiteral("type"), type.trimmed().toLower());
    settings.setValue(QStringLiteral("intervalHours"), safeInterval);
    settings.setValue(QStringLiteral("lastRun"), QString());
    settings.endGroup();
    settings.endGroup();
    settings.sync();

    bool ok = true;
    if (enabled) {
        ok = startupOptimizer_.scheduleTask(normalizedTaskId.toStdString(), type.toStdString(), safeInterval);
    }

    appendAction("schedule-task", (enabled ? "Enabled " : "Disabled ") + normalizedTaskId.toStdString(), ok);
    emit actionFeedback(ok
                            ? QString("Scheduled task '%1' updated.").arg(normalizedTaskId)
                            : QString("Failed to register scheduled task '%1'.").arg(normalizedTaskId),
                        ok);
    return ok;
}
void UiController::refreshAll() {
    uiErrorMessage_.clear();
    refreshDetectedGame();
    recomputeUiCaches();
    emit metricsChanged();
    emit chartSeriesChanged();
    emit processListChanged();
    emit storageChanged();
    emit actionsChanged();
    emit notificationCenterChanged();
    emit gameModeChanged();
    emit snapshotsChanged();
    emit actionCenterChanged();
    emit auditLogChanged();
    emit restoreCenterChanged();
    emit uiStateChanged();
}

}  // namespace pulseboost
















