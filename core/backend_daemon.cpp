#include "PulseBoostAI/core/backend_daemon.hpp"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "PulseBoostAI/ai/ai_diagnostics_engine.hpp"
#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/core/auto_updater.hpp"
#include "PulseBoostAI/core/game_optimizer.hpp"
#include "PulseBoostAI/core/license_manager.hpp"
#include "PulseBoostAI/core/pulse_bench.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"
#include "PulseBoostAI/core/system_advisor.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/data/telemetry_logger.hpp"
#include "PulseBoostAI/modules/defrag_trigger.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"
#include "PulseBoostAI/modules/network_optimizer.hpp"
#include "PulseBoostAI/modules/ram_optimizer.hpp"
#include "PulseBoostAI/modules/safety_guard.hpp"
#include "PulseBoostAI/modules/tweak_engine.hpp"

namespace pulseboost {
namespace {

struct CommandResult {
    int exitCode = 0;
    QString stdoutText;
    bool supported = true;
};

QString jsonString(const QJsonObject &object) {
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::int64_t toEpochMillis(const std::chrono::system_clock::time_point &tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

SystemSnapshot collectSnapshotSafe(SystemScanner &scanner) {
    SystemSnapshot snapshot;
    try { snapshot = scanner.scanVitals(); } catch (...) { snapshot = SystemSnapshot{}; }
    try { scanner.enrichStorage(snapshot); } catch (...) {}
    try { scanner.enrichProcesses(snapshot); } catch (...) {}
    try { scanner.enrichDrivers(snapshot); } catch (...) {}
    try { scanner.enrichGpuAndNetwork(snapshot); } catch (...) {}
    try { scanner.enrichThermals(snapshot); } catch (...) {}
    try { scanner.finalizeSnapshot(snapshot); } catch (...) {}
    return snapshot;
}

SystemSnapshot collectAdvisorSnapshotSafe(SystemScanner &scanner, StartupOptimizer &startupOptimizer) {
    SystemSnapshot snapshot;
    try { snapshot = scanner.scanVitals(); } catch (...) { snapshot = SystemSnapshot{}; }
    try {
        snapshot.startupItems = startupOptimizer.scanStartupItems();
        snapshot.startupPrograms = static_cast<int>(snapshot.startupItems.size());
    } catch (...) {}
    try { scanner.enrichThermals(snapshot); } catch (...) {}
    return snapshot;
}

QJsonObject snapshotToObject(const SystemSnapshot &snapshot) {
    QJsonObject object;
    object["cpuPercent"] = snapshot.cpuUsagePercent;
    object["ramPercent"] = snapshot.ramUsagePercent;
    object["ramUsedMb"] = snapshot.ramUsedMb;
    object["ramTotalMb"] = snapshot.ramTotalMb;
    object["diskPercent"] = snapshot.diskUsagePercent;
    object["diskUsedGb"] = snapshot.diskUsedGb;
    object["diskTotalGb"] = snapshot.diskTotalGb;
    object["netDownloadKbps"] = snapshot.networkMbps * 1024.0;
    object["netUploadKbps"] = snapshot.networkMbps * 256.0;
    object["gpuPercent"] = snapshot.gpuUsagePercent;
    object["temperatureCelsius"] = snapshot.cpuTempC;
    object["fanRpm"] = snapshot.fanSpeedRpm;
    object["healthScore"] = snapshot.healthScore;
    object["healthSummary"] = QString::fromStdString(snapshot.summary);
    object["timestamp"] = static_cast<qint64>(toEpochMillis(snapshot.capturedAt));

    QJsonArray issues;
    for (const auto &issue : snapshot.issues) {
        issues.append(QString::fromStdString(issue));
    }
    object["issues"] = issues;

    QJsonArray processes;
    for (const auto &process : snapshot.heavyProcesses) {
        QJsonObject item;
        item["pid"] = static_cast<qint64>(process.pid);
        item["name"] = QString::fromStdString(process.name);
        item["imagePath"] = QString::fromStdString(process.imagePath);
        item["cpuPercent"] = process.cpuPercent;
        item["ramMb"] = process.memoryMb;
        item["status"] = process.cpuPercent > 80.0 ? "high_cpu" : (process.memoryMb > 1500.0 ? "high_ram" : "running");
        item["priority"] = "normal";
        item["isCritical"] = process.isCritical;
        const std::string lowerPath = toLowerAscii(process.imagePath);
        item["riskLabel"] = process.isCritical ? "critical" : (lowerPath.find("\\windows\\") != std::string::npos || lowerPath.find("\\program files") != std::string::npos ? "safe" : (lowerPath.find("\\appdata\\") != std::string::npos || lowerPath.find("\\temp\\") != std::string::npos ? "suspicious" : "unknown"));
        processes.append(item);
    }
    object["processes"] = processes;

    QJsonArray startupItems;
    for (const auto &item : snapshot.startupItems) {
        QJsonObject row;
        row["name"] = QString::fromStdString(item.name);
        row["publisher"] = "unknown";
        row["location"] = QString::fromStdString(item.location);
        row["enabled"] = item.enabled;
        row["impact"] = item.impactScore >= 70 ? "high" : (item.impactScore >= 35 ? "medium" : "low");
        startupItems.append(row);
    }
    object["startupItems"] = startupItems;
    object["services"] = QJsonArray{};
    object["drivers"] = QJsonArray{};
    object["storageCategories"] = QJsonArray{};
    return object;
}

QJsonObject tweakToJson(const TweakDefinition &tweak) {
    QJsonObject object;
    object["id"] = QString::fromStdString(tweak.id);
    object["category"] = QString::fromStdString(tweak.category);
    object["name"] = QString::fromStdString(tweak.name);
    object["description"] = QString::fromStdString(tweak.description);
    object["detailedInfo"] = QString::fromStdString(tweak.detailedInfo);
    object["impact"] = QString::fromStdString(tweak.impact);
    object["risk"] = QString::fromStdString(tweak.riskLevel);
    object["requiresRestart"] = tweak.requiresRestart;
    object["isApplied"] = tweak.isApplied;
    object["isApplicable"] = tweak.isApplicable;
    if (!tweak.notApplicableReason.empty()) {
        object["notApplicableReason"] = QString::fromStdString(tweak.notApplicableReason);
    }
    return object;
}

QJsonObject benchmarkToJson(const BenchmarkResult &result) {
    QJsonObject object;
    object["cpuScore"] = result.cpuScore;
    object["ramBandwidthMBps"] = result.ramBandwidthMBps;
    object["diskReadMBps"] = result.diskReadMBps;
    object["networkLatencyMs"] = result.networkLatencyMs;
    object["gpuScore"] = result.gpuScore;
    object["compositeScore"] = result.compositeScore;
    object["pulseScore"] = result.pulseScore;
    object["grade"] = QString::fromStdString(result.grade);
    object["timestamp"] = static_cast<qint64>(result.timestamp);
    return object;
}

QJsonObject advisorToJson(const std::vector<AdvisorItem> &items) {
    QJsonArray array;
    for (const auto &item : items) {
        QJsonObject row;
        row["id"] = QString::fromStdString(item.id);
        row["category"] = QString::fromStdString(item.category);
        row["title"] = QString::fromStdString(item.title);
        row["description"] = QString::fromStdString(item.description);
        row["impact"] = QString::fromStdString(item.impact);
        row["status"] = QString::fromStdString(item.status);
        row["actionable"] = item.actionable;
        if (!item.actionLabel.empty()) row["actionLabel"] = QString::fromStdString(item.actionLabel);
        if (!item.actionId.empty()) row["actionId"] = QString::fromStdString(item.actionId);
        array.append(row);
    }
    return QJsonObject{{"ok", true}, {"items", array}};
}

QJsonObject gameProfileToJson(const GameProfile &profile) {
    QJsonObject object;
    object["executableName"] = QString::fromStdString(profile.executableName);
    object["displayName"] = QString::fromStdString(profile.displayName);
    object["installPath"] = QString::fromStdString(profile.installPath);
    object["launcher"] = QString::fromStdString(profile.launcher);
    object["isDetected"] = profile.isDetected;
    object["isOptimized"] = profile.isOptimized;
    object["isRunning"] = profile.isRunning;
    object["runningPid"] = static_cast<qint64>(profile.runningPid);
    object["tweaksAvailable"] = profile.tweaksAvailable;
    QJsonArray tweaksApplied;
    for (const auto &tweak : profile.tweaksApplied) tweaksApplied.append(QString::fromStdString(tweak));
    object["tweaksApplied"] = tweaksApplied;
    return object;
}

QJsonObject gameSessionToJson(const GameSessionResult &result) {
    QJsonObject object;
    object["ok"] = result.ok;
    object["pid"] = static_cast<qint64>(result.pid);
    object["displayName"] = QString::fromStdString(result.displayName);
    object["executableName"] = QString::fromStdString(result.executableName);
    object["aiSuspended"] = result.aiSuspended;
    object["networkTuned"] = result.networkTuned;
    object["powerProfileBoosted"] = result.powerProfileBoosted;
    object["launchersSuspended"] = result.launchersSuspended;
    if (!result.reason.empty()) object["reason"] = QString::fromStdString(result.reason);
    QJsonArray proof;
    for (const auto &entry : result.proof) proof.append(QString::fromStdString(entry));
    object["proof"] = proof;
    return object;
}

QJsonObject networkDiagnosticsToObject(const NetworkDiagnostics &diagnostics) {
    QJsonObject object;
    object["ok"] = true;
    object["connectionType"] = QString::fromStdString(diagnostics.connectionType);
    object["adapterName"] = QString::fromStdString(diagnostics.adapterName);
    object["adapterDescription"] = QString::fromStdString(diagnostics.adapterDescription);
    object["dnsSuffix"] = QString::fromStdString(diagnostics.dnsSuffix);
    QJsonArray dns;
    for (const auto &server : diagnostics.dnsServers) dns.append(QString::fromStdString(server));
    object["dnsServers"] = dns;
    QJsonArray pings;
    for (const auto &probe : diagnostics.probes) {
        pings.append(QJsonObject{{"name", QString::fromStdString(probe.name)}, {"host", QString::fromStdString(probe.host)}, {"latencyMs", probe.latencyMs}, {"status", QString::fromStdString(probe.status)}});
    }
    object["pings"] = pings;
    return object;
}

QJsonObject ramBreakdownToObject(const RamBreakdown &breakdown) {
    return QJsonObject{{"ok", true}, {"totalMb", breakdown.totalMb}, {"usedMb", breakdown.usedMb}, {"freeMb", breakdown.freeMb}, {"appsMb", breakdown.appsMb}, {"systemMb", breakdown.systemMb}, {"cachedMb", breakdown.cachedMb}, {"recoverableMb", breakdown.recoverableMb}};
}

QString maskedLicenseKey(const QString &rawKey) {
    const QString trimmed = rawKey.trimmed();
    if (trimmed.isEmpty()) return {};
    if (trimmed.size() <= 8) return QString(trimmed.size(), QChar('*'));
    return trimmed.left(6) + QStringLiteral("-****-****-") + trimmed.right(4);
}

CommandResult makeUnsupported() {
    return {2, QStringLiteral("{\"ok\":false,\"reason\":\"unsupported-daemon-command\"}"), false};
}

void writeJsonLine(QTcpSocket *socket, const QJsonObject &payload) {
    socket->write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    socket->write("\n");
    socket->flush();
}

CommandResult executeDaemonCommand(const QStringList &args,
                                   ProcessManager &processManager,
                                   StartupOptimizer &startupOptimizer,
                                   SystemScanner &scanner,
                                   OptimizationHistory &optimizationHistory,
                                   JunkCleaner &junkCleaner,
                                   NetworkOptimizer &networkOptimizer,
                                   SafetyGuard &safetyGuard,
                                   DefragTrigger &defragTrigger,
                                   TweakEngine &tweakEngine,
                                   PulseBench &pulseBench,
                                   SystemAdvisor &systemAdvisor,
                                   GameOptimizer &gameOptimizer) {
    if (args.isEmpty()) return makeUnsupported();
    const QString command = args.at(0);

    if (command == "--ping") return {0, QStringLiteral("{\"ok\":true,\"daemon\":true}"), true};
    if (command == "--snapshot-json" || command == "--refresh-all") return {0, jsonString(snapshotToObject(collectSnapshotSafe(scanner))), true};
    if (command == "--system-advisor") return {0, jsonString(advisorToJson(systemAdvisor.analyze(collectAdvisorSnapshotSafe(scanner, startupOptimizer), tweakEngine.listTweaks()))), true};
    if (command == "--detect-games") {
        QJsonArray games;
        for (const auto &game : gameOptimizer.detectInstalledGames()) games.append(gameProfileToJson(game));
        return {0, jsonString(QJsonObject{{"ok", true}, {"games", games}}), true};
    }
    if (command == "--list-tweaks") {
        const auto tweaks = tweakEngine.listTweaks();
        QJsonArray array;
        int available = 0;
        for (const auto &tweak : tweaks) { array.append(tweakToJson(tweak)); if (tweak.isApplicable) ++available; }
        return {0, jsonString(QJsonObject{{"ok", true}, {"appliedCount", tweakEngine.appliedCount()}, {"availableCount", available}, {"tweaks", array}}), true};
    }
    if (command == "--benchmark-history") {
        const auto history = pulseBench.loadHistory();
        QJsonArray array;
        for (const auto &entry : history) array.append(benchmarkToJson(entry));
        return {0, jsonString(QJsonObject{{"ok", true}, {"history", array}}), true};
    }
    if (command == "--quick-benchmark") { auto payload = benchmarkToJson(pulseBench.runQuick()); payload["ok"] = true; return {0, jsonString(payload), true}; }
    if (command == "--full-benchmark") { auto payload = benchmarkToJson(pulseBench.runFull()); payload["ok"] = true; return {0, jsonString(payload), true}; }
    if (command == "--optimize-game" && args.size() > 1) return {0, jsonString(gameSessionToJson(gameOptimizer.optimizeRunningGame(args.at(1).toStdString()))), true};
    if (command == "--launch-optimized-game" && args.size() > 1) return {0, jsonString(gameSessionToJson(gameOptimizer.launchAndOptimize(args.at(1).toStdString()))), true};
    if (command == "--revert-game-optimization" || command == "--disable-game-mode") return {0, jsonString(gameSessionToJson(gameOptimizer.revertOptimization())), true};
    if (command == "--game-mode-status") return {0, jsonString(gameSessionToJson(gameOptimizer.currentSessionStatus())), true};
    if (command == "--enable-game-mode") {
        const auto games = gameOptimizer.detectInstalledGames();
        const auto running = std::find_if(games.begin(), games.end(), [](const auto &game) { return game.isRunning; });
        if (running == games.end()) return {1, QStringLiteral("{\"ok\":false,\"reason\":\"no-running-game\"}"), true};
        return {0, jsonString(gameSessionToJson(gameOptimizer.optimizeRunningGame(running->executableName))), true};
    }
    if (command == "--optimize-ram" || command == "--flush-standby" || command == "--enable-ram-saver") {
        RamOptimizer optimizer(processManager);
        RamOptimizationResult result = command == "--optimize-ram" ? optimizer.optimizeWorkingSets() : (command == "--flush-standby" ? optimizer.flushStandbyList() : optimizer.enableRamSaverMode());
        const bool ok = result.processesOptimized > 0;
        QJsonObject payload;
        payload["ok"] = ok;
        payload["optimized"] = static_cast<qint64>(result.processesOptimized);
        payload["skipped"] = static_cast<qint64>(result.processesSkipped);
        QJsonArray proof;
        proof.append(QStringLiteral("Freed working sets across %1 processes.").arg(static_cast<qulonglong>(result.processesOptimized)));
        proof.append(QStringLiteral("Skipped %1 protected or lean processes.").arg(static_cast<qulonglong>(result.processesSkipped)));
        payload["proof"] = proof;
        return {ok ? 0 : 1, jsonString(payload), true};
    }
    if (command == "--network-diagnostics") return {0, jsonString(networkDiagnosticsToObject(networkOptimizer.diagnostics())), true};
    if (command == "--ram-breakdown") { RamOptimizer optimizer(processManager); return {0, jsonString(ramBreakdownToObject(optimizer.currentBreakdown())), true}; }
    if (command == "--estimate-junk") return {0, QStringLiteral("{\"ok\":true,\"bytes\":%1}").arg(static_cast<qulonglong>(junkCleaner.estimateRecoverableBytes())), true};
    if (command == "--clean") {
        const auto result = junkCleaner.cleanSafeTargets();
        ActionRecord record;
        record.timestampUtc = currentTimestampUtc();
        record.action = "cli-clean";
        record.details = "Recovered " + std::to_string(result.bytesRecovered) + " bytes";
        record.success = true;
        optimizationHistory.record(record);
        QJsonObject payload;
        payload["ok"] = true;
        payload["bytesRecovered"] = static_cast<qint64>(result.bytesRecovered);
        payload["filesRemoved"] = result.filesRemoved;
        QJsonArray proof;
        proof.append(QStringLiteral("Freed %1 of junk data.").arg(QString::fromStdString(formatBytes(result.bytesRecovered))));
        proof.append(QStringLiteral("Removed %1 files from safe cleanup targets.").arg(result.filesRemoved));
        payload["proof"] = proof;
        return {0, jsonString(payload), true};
    }
    if (command == "--recent-actions-json") {
        auto records = optimizationHistory.load();
        std::reverse(records.begin(), records.end());
        if (records.size() > 20) records.resize(20);
        QJsonArray actions;
        for (const auto &record : records) actions.append(QJsonObject{{"timestamp", QString::fromStdString(record.timestampUtc)}, {"action", QString::fromStdString(record.action)}, {"details", QString::fromStdString(record.details)}, {"success", record.success}});
        return {0, jsonString(QJsonObject{{"ok", true}, {"actions", actions}}), true};
    }
    if (command == "--get-ai-preferences") {
        QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
        settings.beginGroup(QStringLiteral("AiMode"));
        const QString mode = settings.value(QStringLiteral("mode"), QStringLiteral("local")).toString();
        const QString apiKey = settings.value(QStringLiteral("cloudApiKey"), QString()).toString();
        settings.endGroup();
        return {0, jsonString(QJsonObject{{"ok", true}, {"mode", mode}, {"cloudConfigured", !apiKey.trimmed().isEmpty()}, {"apiKeyMasked", maskedLicenseKey(apiKey)}, {"suspendedForGame", settings.value(QStringLiteral("TauriUi/aiSuspendedForGame"), false).toBool()}}), true};
    }
    if (command == "--set-ai-preferences" && args.size() > 1) {
        const QString mode = args.at(1).trimmed().toLower();
        const QString apiKey = args.size() > 2 ? args.at(2).trimmed() : QString();
        QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
        settings.beginGroup(QStringLiteral("AiMode"));
        settings.setValue(QStringLiteral("mode"), mode == QStringLiteral("cloud") ? QStringLiteral("cloud") : QStringLiteral("local"));
        settings.setValue(QStringLiteral("cloudApiKey"), apiKey);
        settings.endGroup(); settings.sync();
        return {0, jsonString(QJsonObject{{"ok", true}, {"mode", mode == QStringLiteral("cloud") ? QStringLiteral("cloud") : QStringLiteral("local")}, {"cloudConfigured", !apiKey.isEmpty()}, {"apiKeyMasked", maskedLicenseKey(apiKey)}}), true};
    }
    if (command == "--chat") {
        QString message;
        for (int index = 1; index < args.size(); ++index) { if (!message.isEmpty()) message.append(' '); message.append(args.at(index)); }
        QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
        if (settings.value(QStringLiteral("TauriUi/aiSuspendedForGame"), false).toBool()) return {0, QStringLiteral("PulseBoost AI is paused during the active game session to keep resource usage low. Revert the game optimization to wake it back up."), true};
        const auto snapshot = collectSnapshotSafe(scanner);
        AiDiagnosticsEngine diagnosticsEngine;
        return {0, QString::fromStdString(diagnosticsEngine.reason(message.toStdString(), snapshot, {}).reply), true};
    }
    return makeUnsupported();
}

}  // namespace

BackendDaemon::BackendDaemon(ProcessManager &processManager,
                             ServiceManager &serviceManager,
                             RegistryOptimizer &registryOptimizer,
                             StartupOptimizer &startupOptimizer,
                             MemoryAnalyzer &memoryAnalyzer,
                             DiskAnalyzer &diskAnalyzer,
                             SystemScanner &scanner,
                             TelemetryLogger &telemetryLogger,
                             OptimizationHistory &optimizationHistory,
                             JunkCleaner &junkCleaner,
                             DuplicateFileFinder &duplicateFileFinder,
                             LargeFileScanner &largeFileScanner,
                             GameMode &gameMode,
                             NetworkOptimizer &networkOptimizer,
                             SafetyGuard &safetyGuard,
                             DefragTrigger &defragTrigger,
                             TweakEngine &tweakEngine,
                             PulseBench &pulseBench,
                             SystemAdvisor &systemAdvisor,
                             GameOptimizer &gameOptimizer,
                             LicenseManager &licenseManager,
                             AutoUpdater &autoUpdater,
                             QObject *parent)
    : QObject(parent),
      processManager_(processManager),
      serviceManager_(serviceManager),
      registryOptimizer_(registryOptimizer),
      startupOptimizer_(startupOptimizer),
      memoryAnalyzer_(memoryAnalyzer),
      diskAnalyzer_(diskAnalyzer),
      scanner_(scanner),
      telemetryLogger_(telemetryLogger),
      optimizationHistory_(optimizationHistory),
      junkCleaner_(junkCleaner),
      duplicateFileFinder_(duplicateFileFinder),
      largeFileScanner_(largeFileScanner),
      gameMode_(gameMode),
      networkOptimizer_(networkOptimizer),
      safetyGuard_(safetyGuard),
      defragTrigger_(defragTrigger),
      tweakEngine_(tweakEngine),
      pulseBench_(pulseBench),
      systemAdvisor_(systemAdvisor),
      gameOptimizer_(gameOptimizer),
      licenseManager_(licenseManager),
      autoUpdater_(autoUpdater) {
    server_ = new QTcpServer(this);
    snapshotTimer_ = new QTimer(this);
    snapshotTimer_->setInterval(3000);
    connect(server_, &QTcpServer::newConnection, this, &BackendDaemon::handleNewConnection);
    connect(snapshotTimer_, &QTimer::timeout, this, &BackendDaemon::sendSnapshotEvent);
}

bool BackendDaemon::listen(quint16 port) {
    if (!server_->listen(QHostAddress::LocalHost, port)) return false;
    snapshotTimer_->start();
    return true;
}

void BackendDaemon::handleNewConnection() {
    while (server_->hasPendingConnections()) {
        auto *socket = server_->nextPendingConnection();
        if (socket == nullptr) continue;
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { handleReadyRead(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() { handleSocketDisconnected(socket); });
    }
}

void BackendDaemon::handleReadyRead(QTcpSocket *socket) {
    buffers_[socket].append(socket->readAll());
    QByteArray &buffer = buffers_[socket];
    while (true) {
        const int newlineIndex = buffer.indexOf('\n');
        if (newlineIndex < 0) break;
        const QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);
        if (line.isEmpty()) continue;

        QJsonParseError parseError {};
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            writeJsonLine(socket, QJsonObject{{"ok", false}, {"error", "invalid-request"}});
            socket->disconnectFromHost();
            return;
        }

        const QJsonObject request = document.object();
        const QString id = request.value("id").toString();
        if (request.value("subscribe").toString() == QStringLiteral("snapshot_ready")) {
            snapshotSubscribers_.insert(socket);
            writeJsonLine(socket, QJsonObject{{"id", id}, {"ok", true}, {"subscribed", "snapshot_ready"}});
            writeJsonLine(socket, QJsonObject{{"event", "snapshot_ready"}, {"payload", snapshotToObject(collectSnapshotSafe(scanner_))}});
            continue;
        }

        QStringList argv;
        for (const auto &value : request.value("argv").toArray()) argv.append(value.toString());
        if (argv.isEmpty()) {
            writeJsonLine(socket, QJsonObject{{"id", id}, {"ok", false}, {"error", "missing-argv"}});
            socket->disconnectFromHost();
            return;
        }

        if (argv.first() == QStringLiteral("--chat-stream")) {
            QString message;
            for (int index = 1; index < argv.size(); ++index) { if (!message.isEmpty()) message.append(' '); message.append(argv.at(index)); }
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            QString reply;
            if (settings.value(QStringLiteral("TauriUi/aiSuspendedForGame"), false).toBool()) {
                reply = QStringLiteral("PulseBoost AI is paused during the active game session to keep resource usage low. Revert the game optimization to wake it back up.");
            } else {
                const auto snapshot = collectSnapshotSafe(scanner_);
                AiDiagnosticsEngine diagnosticsEngine;
                reply = QString::fromStdString(diagnosticsEngine.reason(message.toStdString(), snapshot, {}).reply);
            }
            const QStringList words = reply.split(' ', Qt::SkipEmptyParts);
            for (const auto &word : words) writeJsonLine(socket, QJsonObject{{"id", id}, {"type", "chunk"}, {"chunk", word + QStringLiteral(" ")}});
            writeJsonLine(socket, QJsonObject{{"id", id}, {"type", "done"}, {"ok", true}, {"reply", reply}});
            socket->disconnectFromHost();
            return;
        }

        const auto result = executeDaemonCommand(argv, processManager_, startupOptimizer_, scanner_, optimizationHistory_, junkCleaner_, networkOptimizer_, safetyGuard_, defragTrigger_, tweakEngine_, pulseBench_, systemAdvisor_, gameOptimizer_);
        writeJsonLine(socket, QJsonObject{{"id", id}, {"ok", result.exitCode == 0}, {"exitCode", result.exitCode}, {"supported", result.supported}, {"stdout", result.stdoutText}});
        socket->disconnectFromHost();
        return;
    }
}

void BackendDaemon::handleSocketDisconnected(QTcpSocket *socket) {
    snapshotSubscribers_.remove(socket);
    buffers_.remove(socket);
    socket->deleteLater();
}

void BackendDaemon::sendSnapshotEvent() {
    if (snapshotSubscribers_.isEmpty()) return;
    const auto snapshot = collectSnapshotSafe(scanner_);
    const QJsonObject payload = snapshotToObject(snapshot);
    telemetryLogger_.append(snapshot);
    for (QTcpSocket *socket : snapshotSubscribers_) {
        if (socket != nullptr && socket->state() == QAbstractSocket::ConnectedState) {
            writeJsonLine(socket, QJsonObject{{"event", "snapshot_ready"}, {"payload", payload}});
        }
    }
}

}  // namespace pulseboost
