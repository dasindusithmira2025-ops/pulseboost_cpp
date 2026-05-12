#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <optional>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <QCoreApplication>
#include <QDateTime>
#include <QSet>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>
#include <QProcess>

#include "PulseBoostAI/ai/agent_engine.hpp"
#include "PulseBoostAI/ai/ai_diagnostics_engine.hpp"
#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/core/disk_analyzer.hpp"
#include "PulseBoostAI/core/license_manager.hpp"
#include "PulseBoostAI/core/memory_analyzer.hpp"
#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/core/registry_optimizer.hpp"
#include "PulseBoostAI/core/service_manager.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"
#include "PulseBoostAI/core/telemetry_engine.hpp"
#include "PulseBoostAI/core/crash_reporter.hpp"
#include "PulseBoostAI/core/auto_updater.hpp"
#include "PulseBoostAI/core/game_optimizer.hpp"
#include "PulseBoostAI/core/backend_daemon.hpp"
#include "PulseBoostAI/core/pulse_bench.hpp"
#include "PulseBoostAI/core/system_advisor.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/data/telemetry_cache.hpp"
#include "PulseBoostAI/data/telemetry_logger.hpp"
#include "PulseBoostAI/modules/developer_mode.hpp"
#include "PulseBoostAI/modules/defrag_trigger.hpp"
#include "PulseBoostAI/modules/game_mode.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"
#include "PulseBoostAI/modules/duplicate_file_finder.hpp"
#include "PulseBoostAI/modules/large_file_scanner.hpp"
#include "PulseBoostAI/modules/ram_optimizer.hpp"
#include "PulseBoostAI/modules/safety_guard.hpp"
#include "PulseBoostAI/modules/safety_policy.hpp"
#include "PulseBoostAI/modules/network_optimizer.hpp"
#include "PulseBoostAI/modules/tweak_engine.hpp"
#include "PulseBoostAI/ui_backend/ui_controller.hpp"
#include "PulseBoostAI/ui_backend/ui_preferences.hpp"
#include "PulseBoostAI/ui_backend/feature_gate.hpp"

namespace {

std::mutex g_guiLogMutex;

void appendGuiLog(const std::string &message) {
    std::lock_guard<std::mutex> lock(g_guiLogMutex);
    std::ofstream output("logs/gui_runtime.log", std::ios::app);
    output << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString() << ' ' << message << '\n';
}

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}
bool parseBooleanString(const std::string &raw) {
    const std::string normalized = toLowerAscii(raw);
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}
std::string escapeJson(const std::string &input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (const char ch : input) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch; break;
        }
    }
    return out;
}

std::int64_t toEpochMillis(const std::chrono::system_clock::time_point &tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

pulseboost::SystemSnapshot collectSnapshotSafe(pulseboost::SystemScanner &scanner) {
    pulseboost::SystemSnapshot snapshot;
    try {
        snapshot = scanner.scanVitals();
    } catch (...) {
        snapshot = pulseboost::SystemSnapshot{};
    }

    try { scanner.enrichStorage(snapshot); } catch (...) {}
    try { scanner.enrichProcesses(snapshot); } catch (...) {}
    try { scanner.enrichDrivers(snapshot); } catch (...) {}
    try { scanner.enrichGpuAndNetwork(snapshot); } catch (...) {}
    try { scanner.enrichThermals(snapshot); } catch (...) {}
    try { scanner.finalizeSnapshot(snapshot); } catch (...) {}
    return snapshot;
}

pulseboost::SystemSnapshot collectAdvisorSnapshotSafe(pulseboost::SystemScanner &scanner,
                                                     pulseboost::StartupOptimizer &startupOptimizer) {
    pulseboost::SystemSnapshot snapshot;
    try {
        snapshot = scanner.scanVitals();
    } catch (...) {
        snapshot = pulseboost::SystemSnapshot{};
    }

    try {
        snapshot.startupItems = startupOptimizer.scanStartupItems();
        snapshot.startupPrograms = static_cast<int>(snapshot.startupItems.size());
    } catch (...) {}

    try { scanner.enrichThermals(snapshot); } catch (...) {}

    return snapshot;
}
const pulseboost::StartupItem *findStartupByName(const std::vector<pulseboost::StartupItem> &items,
                                                 const std::string &name) {
    const std::string needle = toLowerAscii(name);
    for (const auto &item : items) {
        if (toLowerAscii(item.name) == needle) {
            return &item;
        }
    }
    return nullptr;
}

bool isCriticalProcessPid(pulseboost::ProcessManager &processManager, DWORD pid) {
    const auto processes = processManager.enumerateProcesses();
    for (const auto &process : processes) {
        if (process.pid == pid) {
            return process.isCritical;
        }
    }
    return false;
}

std::vector<std::filesystem::path> defaultDuplicateRoots() {
    std::vector<std::filesystem::path> roots;
    const char *userProfile = std::getenv("USERPROFILE");
    if (userProfile == nullptr) {
        return roots;
    }

    const std::filesystem::path base(userProfile);
    const char *folders[] = {"Desktop", "Documents", "Downloads", "Pictures", "Videos", "Music"};
    for (const char *folder : folders) {
        std::error_code error;
        const auto candidate = base / folder;
        if (std::filesystem::exists(candidate, error) && !error) {
            roots.push_back(candidate);
        }
    }
    return roots;
}

std::optional<std::filesystem::path> resolveUserFileTarget(const std::string &rawPath) {
    if (rawPath.empty()) {
        return std::nullopt;
    }

    std::error_code error;
    const auto candidate = std::filesystem::u8path(rawPath);
    if (!std::filesystem::exists(candidate, error) || error) {
        return std::nullopt;
    }

    const auto resolved = std::filesystem::weakly_canonical(candidate, error);
    if (error) {
        return std::nullopt;
    }

    const char *userProfile = std::getenv("USERPROFILE");
    if (userProfile == nullptr) {
        return std::nullopt;
    }

    const auto userRoot = std::filesystem::weakly_canonical(std::filesystem::path(userProfile), error);
    if (error) {
        return std::nullopt;
    }

    const std::string normalizedPath = toLowerAscii(resolved.string());
    const std::string normalizedRoot = toLowerAscii(userRoot.string());
    if (normalizedPath.rfind(normalizedRoot, 0) != 0) {
        return std::nullopt;
    }

    return resolved;
}

bool openFileLocation(const std::filesystem::path &path) {
    STARTUPINFOW startupInfo {};
    PROCESS_INFORMATION processInfo {};
    startupInfo.cb = sizeof(startupInfo);

    std::wstring commandLine = L"explorer.exe /select,";
    commandLine.push_back(L'\"');
    commandLine += path.wstring();
    commandLine.push_back(L'\"');

    const BOOL created = CreateProcessW(
        nullptr,
        commandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);
    if (!created) {
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

std::string duplicateGroupsToJson(const std::vector<pulseboost::DuplicateGroup> &groups) {
    std::ostringstream out;
    out << "{\"ok\":true,\"groups\":[";
    for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
        const auto &group = groups[groupIndex];
        if (groupIndex > 0) {
            out << ',';
        }
        out << "{"
            << "\"hash\":\"" << escapeJson(group.hash) << "\"," 
            << "\"reclaimableBytes\":" << group.reclaimableBytes << ","
            << "\"files\":[";
        for (std::size_t fileIndex = 0; fileIndex < group.files.size(); ++fileIndex) {
            const auto &file = group.files[fileIndex];
            if (fileIndex > 0) {
                out << ',';
            }
            out << "{"
                << "\"path\":\"" << escapeJson(file.path) << "\"," 
                << "\"bytes\":" << file.bytes
                << "}";
        }
        out << "]}";
    }
    out << "]}";
    return out.str();
}

std::string largeFilesToJson(const std::vector<pulseboost::FileEntry> &files) {
    std::ostringstream out;
    out << "{\"ok\":true,\"files\":[";
    for (std::size_t index = 0; index < files.size(); ++index) {
        const auto &file = files[index];
        if (index > 0) {
            out << ',';
        }
        out << "{"
            << "\"path\":\"" << escapeJson(file.path) << "\"," 
            << "\"bytes\":" << file.bytes
            << "}";
    }
    out << "]}";
    return out.str();
}

QString startupSnapshotKeyFromJsonObject(const QJsonObject &object) {
    const QString name = object.value(QStringLiteral("name")).toString().trimmed().toLower();
    const QString location = object.value(QStringLiteral("location")).toString().trimmed().toLower();
    return name + QStringLiteral("|") + location;
}

QString startupSnapshotKeyFromItem(const pulseboost::StartupItem &item) {
    return QString::fromStdString(item.name).trimmed().toLower() +
           QStringLiteral("|") +
           QString::fromStdString(item.location).trimmed().toLower();
}

QJsonObject restoreSnapshotFromFile(const std::filesystem::path &snapshotPath,
                                    pulseboost::StartupOptimizer &startupOptimizer,
                                    pulseboost::SafetyGuard &safetyGuard,
                                    pulseboost::OptimizationHistory &optimizationHistory) {
    QFile file(QString::fromStdString(snapshotPath.string()));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("snapshot-open-failed")}};
    }

    QJsonParseError parseError {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("snapshot-invalid")}};
    }

    const QJsonObject root = document.object();
    const QJsonArray startupArray = root.value(QStringLiteral("startupItems")).toArray();
    QSet<QString> baselineStartup;
    baselineStartup.reserve(startupArray.size());
    for (const QJsonValue &value : startupArray) {
        if (value.isObject()) {
            baselineStartup.insert(startupSnapshotKeyFromJsonObject(value.toObject()));
        }
    }

    const bool restorePointCreated = safetyGuard.createRestorePoint(L"PulseBoost AI - snapshot restore");
    const QString backupRoot = QDir::currentPath() + QStringLiteral("/backups/snapshot-restore");
    const auto currentStartup = startupOptimizer.scanStartupItems();

    int disabledCount = 0;
    int failedCount = 0;
    for (const auto &item : currentStartup) {
        if (baselineStartup.contains(startupSnapshotKeyFromItem(item))) {
            continue;
        }
        const bool ok = startupOptimizer.disableStartupItem(item, backupRoot.toStdString());
        if (ok) {
            ++disabledCount;
        } else {
            ++failedCount;
        }
    }

    const bool success = failedCount == 0;
    optimizationHistory.record({
        .timestampUtc = pulseboost::currentTimestampUtc(),
        .action = "snapshot-restore",
        .details = (std::string("CLI snapshot restore from ") + snapshotPath.filename().string() +
                    " | disabled " + std::to_string(disabledCount) +
                    (failedCount > 0 ? std::string(" | failed ") + std::to_string(failedCount) : std::string())),
        .success = success,
    });

    QJsonObject payload;
    payload[QStringLiteral("ok")] = success;
    payload[QStringLiteral("restorePointCreated")] = restorePointCreated;
    payload[QStringLiteral("disabledCount")] = disabledCount;
    payload[QStringLiteral("failedCount")] = failedCount;
    if (!success) {
        payload[QStringLiteral("reason")] = QStringLiteral("snapshot-restore-partial-failure");
    }
    return payload;
}

QJsonObject networkDiagnosticsToObject(const pulseboost::NetworkDiagnostics &diagnostics) {
    QJsonObject object;
    object[QStringLiteral("ok")] = true;
    object[QStringLiteral("connectionType")] = QString::fromStdString(diagnostics.connectionType);
    object[QStringLiteral("adapterName")] = QString::fromStdString(diagnostics.adapterName);
    object[QStringLiteral("adapterDescription")] = QString::fromStdString(diagnostics.adapterDescription);
    object[QStringLiteral("dnsSuffix")] = QString::fromStdString(diagnostics.dnsSuffix);

    QJsonArray dnsArray;
    for (const auto &server : diagnostics.dnsServers) {
        dnsArray.append(QString::fromStdString(server));
    }
    object[QStringLiteral("dnsServers")] = dnsArray;

    QJsonArray pingArray;
    for (const auto &probe : diagnostics.probes) {
        QJsonObject probeObject;
        probeObject[QStringLiteral("name")] = QString::fromStdString(probe.name);
        probeObject[QStringLiteral("host")] = QString::fromStdString(probe.host);
        probeObject[QStringLiteral("latencyMs")] = probe.latencyMs;
        probeObject[QStringLiteral("status")] = QString::fromStdString(probe.status);
        pingArray.append(probeObject);
    }
    object[QStringLiteral("pings")] = pingArray;
    return object;
}

QJsonObject ramBreakdownToObject(const pulseboost::RamBreakdown &breakdown) {
    QJsonObject object;
    object[QStringLiteral("ok")] = true;
    object[QStringLiteral("totalMb")] = breakdown.totalMb;
    object[QStringLiteral("usedMb")] = breakdown.usedMb;
    object[QStringLiteral("freeMb")] = breakdown.freeMb;
    object[QStringLiteral("appsMb")] = breakdown.appsMb;
    object[QStringLiteral("systemMb")] = breakdown.systemMb;
    object[QStringLiteral("cachedMb")] = breakdown.cachedMb;
    object[QStringLiteral("recoverableMb")] = breakdown.recoverableMb;
    return object;
}

std::string processRiskLabel(const pulseboost::ProcessInfo &process) {
    if (process.isCritical) {
        return "critical";
    }
    if (process.imagePath.empty()) {
        return "unknown";
    }

    const std::string lowerPath = toLowerAscii(process.imagePath);
    if (lowerPath.find("\\windows\\") != std::string::npos ||
        lowerPath.find("\\program files\\") != std::string::npos ||
        lowerPath.find("\\program files (x86)\\") != std::string::npos) {
        return "safe";
    }
    if (lowerPath.find("\\appdata\\") != std::string::npos ||
        lowerPath.find("\\temp\\") != std::string::npos ||
        lowerPath.find("\\downloads\\") != std::string::npos) {
        return "suspicious";
    }
    return "unknown";
}

std::string recentActionsToJson(const std::vector<pulseboost::ActionRecord> &records) {
    std::ostringstream out;
    out << "{\"ok\":true,\"actions\":[";
    for (std::size_t index = 0; index < records.size(); ++index) {
        const auto &record = records[index];
        if (index > 0) {
            out << ',';
        }
        out << "{"
            << "\"timestamp\":\"" << escapeJson(record.timestampUtc) << "\"," 
            << "\"action\":\"" << escapeJson(record.action) << "\"," 
            << "\"details\":\"" << escapeJson(record.details) << "\"," 
            << "\"success\":" << (record.success ? "true" : "false")
            << "}";
    }
    out << "]}";
    return out.str();
}

std::string snapshotToJson(const pulseboost::SystemSnapshot &snapshot) {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out << std::setprecision(2);

    const double netDownloadKbps = snapshot.networkMbps * 1024.0;
    const double netUploadKbps = snapshot.networkMbps * 256.0;

    out << "{";
    out << "\"ok\":true,";
    out << "\"cpuPercent\":" << snapshot.cpuUsagePercent << ",";
    out << "\"ramPercent\":" << snapshot.ramUsagePercent << ",";
    out << "\"ramUsedMb\":" << snapshot.ramUsedMb << ",";
    out << "\"ramTotalMb\":" << snapshot.ramTotalMb << ",";
    out << "\"diskPercent\":" << snapshot.diskUsagePercent << ",";
    out << "\"diskUsedGb\":" << snapshot.diskUsedGb << ",";
    out << "\"diskTotalGb\":" << snapshot.diskTotalGb << ",";
    out << "\"netDownloadKbps\":" << netDownloadKbps << ",";
    out << "\"netUploadKbps\":" << netUploadKbps << ",";
    out << "\"gpuPercent\":" << snapshot.gpuUsagePercent << ",";
    out << "\"temperatureCelsius\":" << snapshot.cpuTempC << ",";
    out << "\"fanRpm\":" << snapshot.fanSpeedRpm << ",";
    out << "\"healthScore\":" << snapshot.healthScore << ",";
    out << "\"healthSummary\":\"" << escapeJson(snapshot.summary) << "\",";

    out << "\"issues\":[";
    for (std::size_t i = 0; i < snapshot.issues.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "\"" << escapeJson(snapshot.issues[i]) << "\"";
    }
    out << "],";

    out << "\"processes\":[";
    for (std::size_t i = 0; i < snapshot.heavyProcesses.size(); ++i) {
        const auto &process = snapshot.heavyProcesses[i];
        if (i > 0) {
            out << ",";
        }
        const char *status = process.cpuPercent > 80.0
            ? "high_cpu"
            : (process.memoryMb > 1500.0 ? "high_ram" : "running");
        out << "{";
        out << "\"pid\":" << process.pid << ",";
        out << "\"name\":\"" << escapeJson(process.name) << "\",";
        out << "\"imagePath\":\"" << escapeJson(process.imagePath) << "\",";
        out << "\"cpuPercent\":" << process.cpuPercent << ",";
        out << "\"ramMb\":" << process.memoryMb << ",";
        out << "\"status\":\"" << status << "\",";
        out << "\"priority\":\"normal\",";
        out << "\"isCritical\":" << (process.isCritical ? "true" : "false") << ",";
        out << "\"riskLabel\":\"" << processRiskLabel(process) << "\"";
        out << "}";
    }
    out << "],";

    out << "\"startupItems\":[";
    for (std::size_t i = 0; i < snapshot.startupItems.size(); ++i) {
        const auto &item = snapshot.startupItems[i];
        if (i > 0) {
            out << ",";
        }
        const char *impact = item.impactScore >= 70 ? "high" : (item.impactScore >= 35 ? "medium" : "low");
        out << "{";
        out << "\"name\":\"" << escapeJson(item.name) << "\",";
        out << "\"publisher\":\"unknown\",";
        out << "\"location\":\"" << escapeJson(item.location) << "\",";
        out << "\"enabled\":" << (item.enabled ? "true" : "false") << ",";
        out << "\"impact\":\"" << impact << "\"";
        out << "}";
    }
    out << "],";

    out << "\"services\":[";
    for (std::size_t i = 0; i < snapshot.services.size(); ++i) {
        const auto &service = snapshot.services[i];
        if (i > 0) {
            out << ",";
        }
        out << "{";
        out << "\"name\":\"" << escapeJson(service.name) << "\",";
        out << "\"displayName\":\"" << escapeJson(service.displayName) << "\",";
        out << "\"status\":\"" << escapeJson(service.state) << "\",";
        out << "\"startType\":\"" << escapeJson(service.startMode) << "\"";
        out << "}";
    }
    out << "],";

    out << "\"drivers\":[";
    for (std::size_t i = 0; i < snapshot.drivers.size(); ++i) {
        const auto &driver = snapshot.drivers[i];
        if (i > 0) {
            out << ",";
        }
        std::string driverStatus = "warning";
        if (!driver.isSigned) {
            driverStatus = "error";
        } else {
            const std::string raw = toLowerAscii(driver.status);
            if (raw.find("ok") != std::string::npos || raw.find("running") != std::string::npos) {
                driverStatus = "ok";
            }
        }

        out << "{";
        out << "\"name\":\"" << escapeJson(driver.deviceName) << "\",";
        out << "\"manufacturer\":\"" << escapeJson(driver.provider) << "\",";
        out << "\"version\":\"" << escapeJson(driver.driverVersion) << "\",";
        out << "\"status\":\"" << driverStatus << "\",";
        out << "\"date\":\"" << escapeJson(driver.driverDate) << "\"";
        out << "}";
    }
    out << "],";

    out << "\"storageCategories\":[";
    for (std::size_t i = 0; i < snapshot.storageCategories.size(); ++i) {
        const auto &storage = snapshot.storageCategories[i];
        if (i > 0) {
            out << ",";
        }
        const double percent = snapshot.diskTotalGb > 0.0
            ? (static_cast<double>(storage.bytes) / (snapshot.diskTotalGb * 1024.0 * 1024.0 * 1024.0)) * 100.0
            : 0.0;
        out << "{";
        out << "\"name\":\"" << escapeJson(storage.name) << "\",";
        out << "\"sizeGb\":" << (static_cast<double>(storage.bytes) / (1024.0 * 1024.0 * 1024.0)) << ",";
        out << "\"percent\":" << percent;
        out << "}";
    }
    out << "],";

    out << "\"timestamp\":" << toEpochMillis(snapshot.capturedAt);
    out << "}";
    return out.str();
}
QJsonObject tweakToJson(const pulseboost::TweakDefinition &tweak) {
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

QJsonObject tweakActionToJson(const pulseboost::TweakActionResult &result) {
    QJsonObject object;
    object["id"] = QString::fromStdString(result.id);
    object["success"] = result.success;
    object["requiresRestart"] = result.requiresRestart;
    object["isApplied"] = result.isApplied;
    if (!result.error.empty()) {
        object["error"] = QString::fromStdString(result.error);
    }
    object["ok"] = result.success;
    return object;
}

QJsonObject tweakBatchToJson(const std::vector<pulseboost::TweakActionResult> &results) {
    QJsonObject object;
    QJsonArray actions;
    int successCount = 0;
    int restartCount = 0;
    for (const auto &result : results) {
        actions.append(tweakActionToJson(result));
        if (result.success) {
            ++successCount;
        }
        if (result.requiresRestart) {
            ++restartCount;
        }
    }
    object["ok"] = successCount == static_cast<int>(results.size());
    object["results"] = actions;
    object["successCount"] = successCount;
    object["failureCount"] = static_cast<int>(results.size()) - successCount;
    object["restartCount"] = restartCount;
    return object;
}

QJsonObject benchmarkToJson(const pulseboost::BenchmarkResult &result) {
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

QJsonObject advisorItemToJson(const pulseboost::AdvisorItem &item) {
    QJsonObject object;
    object["id"] = QString::fromStdString(item.id);
    object["category"] = QString::fromStdString(item.category);
    object["title"] = QString::fromStdString(item.title);
    object["description"] = QString::fromStdString(item.description);
    object["impact"] = QString::fromStdString(item.impact);
    object["status"] = QString::fromStdString(item.status);
    object["actionable"] = item.actionable;
    if (!item.actionLabel.empty()) {
        object["actionLabel"] = QString::fromStdString(item.actionLabel);
    }
    if (!item.actionId.empty()) {
        object["actionId"] = QString::fromStdString(item.actionId);
    }
    return object;
}

QJsonObject gameProfileToJson(const pulseboost::GameProfile &profile) {
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
    for (const auto &tweak : profile.tweaksApplied) {
        tweaksApplied.append(QString::fromStdString(tweak));
    }
    object["tweaksApplied"] = tweaksApplied;
    return object;
}

QJsonArray loadSnapshotRecords() {
    QJsonArray snapshots;
    const auto directory = std::filesystem::path("data") / "snapshots";
    std::error_code error;
    if (!std::filesystem::exists(directory, error) || error) {
        return snapshots;
    }

    struct SnapshotRecord {
        QJsonObject object;
        qint64 createdAt = 0;
    };
    std::vector<SnapshotRecord> records;

    for (const auto &entry : std::filesystem::directory_iterator(directory, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error || !entry.is_regular_file(error) || entry.path().extension() != ".json") {
            continue;
        }
        std::ifstream input(entry.path());
        if (!input.is_open()) {
            continue;
        }
        std::stringstream buffer;
        buffer << input.rdbuf();
        QJsonParseError parseError {};
        const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(buffer.str()), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            continue;
        }
        const QJsonObject snapshot = document.object();
        QJsonObject record;
        record["id"] = QString::fromStdString(entry.path().stem().string());
        record["path"] = QString::fromStdString(entry.path().string());
        record["createdAt"] = static_cast<qint64>(snapshot.value("timestamp").toVariant().toLongLong());
        record["healthScore"] = snapshot.value("healthScore").toDouble();
        records.push_back({record, record.value("createdAt").toVariant().toLongLong()});
    }

    std::sort(records.begin(), records.end(), [](const auto &left, const auto &right) {
        return left.createdAt > right.createdAt;
    });
    for (const auto &record : records) {
        snapshots.append(record.object);
    }
    return snapshots;
}

QJsonArray loadErrorLogEntries(int maxEntries = 40) {
    struct ErrorEntry {
        QString source;
        QString message;
    };

    std::vector<ErrorEntry> entries;
    const std::vector<std::pair<QString, std::filesystem::path>> sources = {
        {QStringLiteral("gui_runtime"), std::filesystem::path("logs") / "gui_runtime.log"},
        {QStringLiteral("self_test"), std::filesystem::path("logs") / "self_test.log"}
    };

    for (const auto &[source, filePath] : sources) {
        std::ifstream input(filePath);
        if (!input.is_open()) {
            continue;
        }
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }
            entries.push_back({source, QString::fromStdString(line)});
        }
    }

    const int start = static_cast<int>(entries.size()) > maxEntries
        ? static_cast<int>(entries.size()) - maxEntries
        : 0;

    QJsonArray payload;
    for (int index = start; index < static_cast<int>(entries.size()); ++index) {
        QJsonObject item;
        item["source"] = entries[static_cast<std::size_t>(index)].source;
        item["message"] = entries[static_cast<std::size_t>(index)].message;
        payload.append(item);
    }
    return payload;
}

QJsonObject exportErrorLogReport() {
    std::error_code error;
    std::filesystem::create_directories("logs", error);
    const auto path = std::filesystem::path("logs") / (
        QStringLiteral("error_report_%1.txt").arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss"))).toStdString());

    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output.is_open()) {
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("unable-to-open-export-file")}};
    }

    output << "PulseBoost AI local error report\n";
    output << "Generated: " << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString() << "\n\n";

    const QJsonArray entries = loadErrorLogEntries(200);
    for (const auto &value : entries) {
        const QJsonObject item = value.toObject();
        output << '[' << item.value("source").toString().toStdString() << "] "
               << item.value("message").toString().toStdString() << "\n";
    }

    output.close();
    QJsonObject payload;
    payload["ok"] = true;
    payload["path"] = QString::fromStdString(path.string());
    return payload;
}

std::string escapeHtml(const std::string &input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (const char ch : input) {
        switch (ch) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += ch; break;
        }
    }
    return out;
}

std::string pulseScoreGrade(int total) {
    if (total >= 900) {
        return "S";
    }
    if (total >= 750) {
        return "A";
    }
    if (total >= 600) {
        return "B";
    }
    if (total >= 450) {
        return "C";
    }
    if (total >= 300) {
        return "D";
    }
    return "F";
}

QJsonObject pulseScoreToJson(const pulseboost::SystemSnapshot &snapshot,
                             const std::vector<pulseboost::TweakDefinition> &tweaks,
                             const std::vector<pulseboost::BenchmarkResult> &history,
                             const std::vector<pulseboost::AdvisorItem> &advisorItems) {
    const int tweaksApplied = static_cast<int>(std::count_if(tweaks.begin(), tweaks.end(), [](const auto &tweak) {
        return tweak.isApplicable && tweak.isApplied;
    }));
    const int tweaksAvailable = static_cast<int>(std::count_if(tweaks.begin(), tweaks.end(), [](const auto &tweak) {
        return tweak.isApplicable;
    }));

    const double ramGb = snapshot.ramTotalMb / 1024.0;
    int hardwareTier = 0;
    hardwareTier += static_cast<int>(std::clamp(ramGb * 4.0, 35.0, 140.0));
    hardwareTier += snapshot.diskTotalGb >= 1024.0 ? 45 : (snapshot.diskTotalGb >= 512.0 ? 30 : 15);
    hardwareTier += snapshot.gpuUsagePercent > 0.0 ? 70 : 30;
    hardwareTier += snapshot.healthScore >= 80 ? 35 : (snapshot.healthScore >= 60 ? 25 : 15);
    hardwareTier = std::clamp(hardwareTier, 0, 300);

    const int optimizationLevel = tweaksAvailable > 0
        ? static_cast<int>(std::round((static_cast<double>(tweaksApplied) / static_cast<double>(tweaksAvailable)) * 400.0))
        : 0;
    const int healthState = std::clamp(snapshot.healthScore * 2, 0, 200);
    const int benchScore = history.empty()
        ? 50
        : std::clamp(static_cast<int>(std::round(history.back().pulseScore / 10.0)), 0, 100);

    const int total = std::clamp(hardwareTier + optimizationLevel + healthState + benchScore, 0, 1000);
    const int percentile = std::clamp(static_cast<int>(std::round((static_cast<double>(total) - 250.0) / 7.5)), 1, 99);

    QJsonObject object;
    object["total"] = total;
    object["hardwareTier"] = hardwareTier;
    object["optimizationLevel"] = optimizationLevel;
    object["healthState"] = healthState;
    object["benchScore"] = benchScore;
    object["grade"] = QString::fromStdString(pulseScoreGrade(total));
    object["percentile"] = percentile;
    object["tweaksApplied"] = tweaksApplied;
    object["tweaksAvailable"] = tweaksAvailable;
    object["benchmarkRuns"] = static_cast<int>(history.size());

    QJsonArray advisors;
    for (const auto &item : advisorItems) {
        advisors.append(advisorItemToJson(item));
    }
    object["advisorItems"] = advisors;

    if (!history.empty()) {
        object["latestBenchmark"] = benchmarkToJson(history.back());
    }
    return object;
}

std::string jsonString(const QJsonObject &object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}

bool hasCliFlag(int argc, char *argv[], const std::string &flag) {
    for (int index = 2; index < argc; ++index) {
        if (argv[index] == flag) {
            return true;
        }
    }
    return false;
}

QJsonObject strictResponse(bool ok, const QString &reason = {}) {
    QJsonObject payload;
    payload["ok"] = ok;
    if (!ok && !reason.isEmpty()) {
        payload["reason"] = reason;
    }
    return payload;
}

bool auditSafety(pulseboost::SafetyPolicy &policy,
                 const pulseboost::SafetyDecision &decision,
                 bool dryRun,
                 bool success,
                 const std::string &summary,
                 const QJsonObject &request,
                 const QJsonObject &result) {
    if (decision.descriptor.actionId.empty() || !decision.descriptor.auditRequired) {
        return true;
    }
    return policy.audit(pulseboost::SafetyAuditEntry {
        .actionId = decision.descriptor.actionId,
        .status = success ? "success" : "failure",
        .summary = summary,
        .riskLevel = decision.descriptor.riskLevel,
        .dryRun = dryRun,
        .requestJson = request,
        .resultJson = result,
    });
}

QJsonObject evaluateSafety(pulseboost::SafetyPolicy &policy,
                           const std::string &actionId,
                           bool dryRun,
                           bool confirmed,
                           bool backupCreated,
                           bool advancedMode,
                           const std::string &target,
                           pulseboost::SafetyDecision *decisionOut) {
    pulseboost::SafetyRequest request;
    request.actionId = actionId;
    request.dryRun = dryRun;
    request.confirmed = confirmed;
    request.backupCreated = backupCreated;
    request.advancedMode = advancedMode;
    request.actor = "cli";
    request.target = target;
    request.requestJson = QJsonObject{
        {"actionId", QString::fromStdString(actionId)},
        {"dryRun", dryRun},
        {"confirmed", confirmed},
        {"backupCreated", backupCreated},
        {"advancedMode", advancedMode},
        {"target", QString::fromStdString(target)},
    };

    const auto decision = policy.evaluate(request);
    if (decisionOut != nullptr) {
        *decisionOut = decision;
    }
    if (decision.allowed) {
        return {};
    }

    QJsonObject payload = strictResponse(false, QString::fromStdString(decision.reason));
    if (!decision.descriptor.actionId.empty()) {
        payload["policy"] = policy.descriptorJson(decision.descriptor);
    }
    auditSafety(policy, decision, dryRun, false, "Blocked by safety policy", request.requestJson, payload);
    return payload;
}

QString maskedLicenseKey(const QString &value) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    if (trimmed.size() <= 8) {
        return QString(trimmed.size(), QChar('*'));
    }
    return trimmed.left(6) + QStringLiteral("-****-****-") + trimmed.right(4);
}

}  // namespace

int main(int argc, char *argv[]) {
    pulseboost::CrashReporter::initialize("logs/crash");

    try {
    if (argc > 1) {
        QCoreApplication app(argc, argv);
        app.setApplicationName("PulseBoost AI");
        app.setOrganizationName("PulseBoost");
        app.setApplicationVersion("1.0.0");
        QSettings::setDefaultFormat(QSettings::IniFormat);

        pulseboost::ProcessManager processManager;
        pulseboost::ServiceManager serviceManager;
        pulseboost::RegistryOptimizer registryOptimizer;
        pulseboost::StartupOptimizer startupOptimizer(registryOptimizer);
        pulseboost::MemoryAnalyzer memoryAnalyzer(processManager);
        pulseboost::DiskAnalyzer diskAnalyzer;
        pulseboost::SystemScanner scanner(processManager,
                                          memoryAnalyzer,
                                          diskAnalyzer,
                                          startupOptimizer,
                                          serviceManager);
        pulseboost::TelemetryLogger telemetryLogger;
        pulseboost::OptimizationHistory optimizationHistory;
        pulseboost::JunkCleaner junkCleaner;
        pulseboost::DuplicateFileFinder duplicateFileFinder;
        pulseboost::LargeFileScanner largeFileScanner;
        pulseboost::GameMode gameMode(processManager, serviceManager);
        pulseboost::NetworkOptimizer networkOptimizer;
        pulseboost::SafetyGuard safetyGuard;
        pulseboost::SafetyPolicy safetyPolicy;
        pulseboost::DefragTrigger defragTrigger;
        pulseboost::TweakEngine tweakEngine(registryOptimizer, serviceManager);
        pulseboost::PulseBench pulseBench;
        pulseboost::SystemAdvisor systemAdvisor;
        pulseboost::GameOptimizer gameOptimizer(processManager, networkOptimizer);
        pulseboost::LicenseManager licenseManager;
        pulseboost::AutoUpdater autoUpdater;

        std::ofstream selfTestLog("logs/self_test.log", std::ios::app);
        const std::string command = argv[1];

        if (command == "--daemon") {
            pulseboost::BackendDaemon daemon(processManager,
                                             serviceManager,
                                             registryOptimizer,
                                             startupOptimizer,
                                             memoryAnalyzer,
                                             diskAnalyzer,
                                             scanner,
                                             telemetryLogger,
                                             optimizationHistory,
                                             junkCleaner,
                                             duplicateFileFinder,
                                             largeFileScanner,
                                             gameMode,
                                             networkOptimizer,
                                             safetyGuard,
                                             defragTrigger,
                                             tweakEngine,
                                             pulseBench,
                                             systemAdvisor,
                                             gameOptimizer,
                                             licenseManager,
                                             autoUpdater);
            if (!daemon.listen()) {
                return 1;
            }
            return app.exec();
        }

        if (command == "--safety-policy") {
            QJsonArray actions;
            for (const auto &descriptor : safetyPolicy.descriptors()) {
                actions.append(safetyPolicy.descriptorJson(descriptor));
            }
            QJsonObject payload;
            payload["ok"] = true;
            payload["actions"] = actions;
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--validate-qml") {
            const QStringList screens = {
                QStringLiteral("Home.qml"),
                QStringLiteral("ActionCenter.qml"),
                QStringLiteral("Optimizations.qml"),
                QStringLiteral("AuditLog.qml"),
                QStringLiteral("RestoreCenter.qml"),
                QStringLiteral("BoostUp.qml"),
                QStringLiteral("Games.qml"),
                QStringLiteral("Backup.qml"),
                QStringLiteral("AiChat.qml"),
                QStringLiteral("Settings.qml"),
            };
            QJsonArray loaded;
            QJsonArray errors;
            for (const QString &screen : screens) {
                QFile file(QStringLiteral(":/ui/qml/screens/%1").arg(screen));
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    errors.append(QJsonObject{{"screen", screen}, {"errors", QJsonArray{QStringLiteral("resource-open-failed")}}});
                    continue;
                }
                const QByteArray source = file.readAll();
                if (!source.contains("import QtQuick") || !source.contains("SystemCtrl")) {
                    errors.append(QJsonObject{{"screen", screen}, {"errors", QJsonArray{QStringLiteral("screen-source-validation-failed")}}});
                    continue;
                }
                loaded.append(screen);
            }
            QJsonObject payload;
            payload["ok"] = errors.isEmpty();
            payload["loaded"] = loaded;
            payload["errors"] = errors;
            std::cout << jsonString(payload) << '\n';
            return errors.isEmpty() ? 0 : 1;
        }

        if (command == "--list-tweaks") {
            const auto tweaks = tweakEngine.listTweaks();
            QJsonArray tweakArray;
            int availableCount = 0;
            for (const auto &tweak : tweaks) {
                tweakArray.append(tweakToJson(tweak));
                if (tweak.isApplicable) {
                    ++availableCount;
                }
            }
            QJsonObject payload;
            payload["ok"] = true;
            payload["appliedCount"] = tweakEngine.appliedCount();
            payload["availableCount"] = availableCount;
            payload["tweaks"] = tweakArray;
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--apply-tweak" && argc > 2) {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "tweak.apply", dryRun, hasCliFlag(argc, argv, "--confirm"), true, hasCliFlag(argc, argv, "--advanced"), argv[2], &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            QJsonObject payload;
            bool ok = true;
            if (dryRun) {
                payload = strictResponse(true);
                payload["dryRun"] = true;
                payload["target"] = QString::fromUtf8(argv[2]);
            } else {
                const auto result = tweakEngine.applyTweak(argv[2]);
                payload = tweakActionToJson(result);
                ok = result.success;
            }
            auditSafety(safetyPolicy, decision, dryRun, ok, "Apply tweak", QJsonObject{{"target", QString::fromUtf8(argv[2])}}, payload);
            std::cout << jsonString(payload) << '\n';
            return ok ? 0 : 1;
        }

        if (command == "--revert-tweak" && argc > 2) {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "tweak.revert", dryRun, hasCliFlag(argc, argv, "--confirm"), true, hasCliFlag(argc, argv, "--advanced"), argv[2], &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            QJsonObject payload;
            bool ok = true;
            if (dryRun) {
                payload = strictResponse(true);
                payload["dryRun"] = true;
                payload["target"] = QString::fromUtf8(argv[2]);
            } else {
                const auto result = tweakEngine.revertTweak(argv[2]);
                payload = tweakActionToJson(result);
                ok = result.success;
            }
            auditSafety(safetyPolicy, decision, dryRun, ok, "Revert tweak", QJsonObject{{"target", QString::fromUtf8(argv[2])}}, payload);
            std::cout << jsonString(payload) << '\n';
            return ok ? 0 : 1;
        }

        if (command == "--apply-safe-tweaks") {
            const auto results = tweakEngine.applySafeTweaks();
            std::cout << jsonString(tweakBatchToJson(results)) << '\n';
            return 0;
        }

        if (command == "--apply-high-impact-tweaks") {
            const auto results = tweakEngine.applyHighImpactTweaks();
            std::cout << jsonString(tweakBatchToJson(results)) << '\n';
            return 0;
        }

        if (command == "--revert-all-tweaks") {
            const auto results = tweakEngine.revertAllTweaks();
            std::cout << jsonString(tweakBatchToJson(results)) << '\n';
            return 0;
        }

        if (command == "--quick-benchmark") {
            const auto result = pulseBench.runQuick();
            QJsonObject payload = benchmarkToJson(result);
            payload["ok"] = true;
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--full-benchmark") {
            const auto result = pulseBench.runFull();
            QJsonObject payload = benchmarkToJson(result);
            payload["ok"] = true;
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--benchmark-history") {
            const auto history = pulseBench.loadHistory();
            QJsonArray historyArray;
            for (const auto &entry : history) {
                historyArray.append(benchmarkToJson(entry));
            }
            QJsonObject payload;
            payload["ok"] = true;
            payload["history"] = historyArray;
            if (const auto delta = pulseBench.latestDelta(); delta.has_value()) {
                QJsonObject deltaObject;
                deltaObject["before"] = benchmarkToJson(delta->before);
                deltaObject["after"] = benchmarkToJson(delta->after);
                deltaObject["percentChange"] = delta->percentChange;
                deltaObject["scoreDelta"] = delta->scoreDelta;
                payload["latestDelta"] = deltaObject;
            }
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--system-advisor") {
            const auto snapshot = collectAdvisorSnapshotSafe(scanner, startupOptimizer);
            const auto tweaks = tweakEngine.listTweaks();
            const auto items = systemAdvisor.analyze(snapshot, tweaks);
            QJsonArray itemArray;
            for (const auto &item : items) {
                itemArray.append(advisorItemToJson(item));
            }
            QJsonObject payload;
            payload["ok"] = true;
            payload["items"] = itemArray;
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--detect-games") {
            const auto games = gameOptimizer.detectInstalledGames();
            QJsonArray gameArray;
            for (const auto &game : games) {
                gameArray.append(gameProfileToJson(game));
            }
            QJsonObject payload;
            payload["ok"] = true;
            payload["games"] = gameArray;
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--get-ai-preferences") {
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.beginGroup(QStringLiteral("AiMode"));
            const QString mode = settings.value(QStringLiteral("mode"), QStringLiteral("local")).toString();
            const QString apiKey = settings.value(QStringLiteral("cloudApiKey"), QString()).toString();
            settings.endGroup();

            QJsonObject payload;
            payload["ok"] = true;
            payload["mode"] = mode;
            payload["cloudConfigured"] = !apiKey.trimmed().isEmpty();
            payload["apiKeyMasked"] = maskedLicenseKey(apiKey);
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--set-ai-preferences" && argc > 2) {
            const QString mode = QString::fromUtf8(argv[2]).trimmed().toLower();
            const QString apiKey = argc > 3 ? QString::fromUtf8(argv[3]).trimmed() : QString();
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.beginGroup(QStringLiteral("AiMode"));
            settings.setValue(QStringLiteral("mode"), mode == QStringLiteral("cloud") ? QStringLiteral("cloud") : QStringLiteral("local"));
            if (!apiKey.isNull()) {
                settings.setValue(QStringLiteral("cloudApiKey"), apiKey);
            }
            settings.endGroup();
            settings.sync();

            QJsonObject payload;
            payload["ok"] = true;
            payload["mode"] = mode == QStringLiteral("cloud") ? QStringLiteral("cloud") : QStringLiteral("local");
            payload["cloudConfigured"] = !apiKey.isEmpty();
            payload["apiKeyMasked"] = maskedLicenseKey(apiKey);
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--license-info") {
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoostAI"));
            settings.beginGroup(QStringLiteral("license"));
            const QString storedKey = settings.value(QStringLiteral("licenseKey"), QString()).toString();
            settings.endGroup();

            QJsonObject payload;
            payload["ok"] = true;
            payload["isPro"] = licenseManager.currentTier() != pulseboost::LicenseManager::Tier::Free;
            payload["isTrial"] = licenseManager.currentTier() == pulseboost::LicenseManager::Tier::Free && !licenseManager.isTrialExpired();
            payload["trialDaysLeft"] = licenseManager.daysRemainingInTrial();
            payload["tierLabel"] = licenseManager.tierLabel();
            payload["licenseKey"] = maskedLicenseKey(storedKey);
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--activate-license" && argc > 2) {
            const auto tier = licenseManager.validateKey(argv[2]);
            QJsonObject payload;
            payload["ok"] = tier != pulseboost::LicenseManager::Tier::Free;
            payload["isPro"] = tier != pulseboost::LicenseManager::Tier::Free;
            payload["trialDaysLeft"] = licenseManager.daysRemainingInTrial();
            payload["tierLabel"] = licenseManager.tierLabel();
            if (tier == pulseboost::LicenseManager::Tier::Free) {
                payload["reason"] = QStringLiteral("invalid-license-key");
            }
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--check-for-updates") {
            QJsonObject payload;
            payload["ok"] = true;
            payload["currentVersion"] = QStringLiteral(APP_VERSION);
            payload["updateAvailable"] = autoUpdater.checkForUpdates(APP_VERSION);
            payload["latestVersion"] = QString::fromStdString(autoUpdater.latestVersion());
            payload["downloadUrl"] = QString::fromStdString(autoUpdater.downloadUrl());
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--error-log") {
            QJsonObject payload;
            payload["ok"] = true;
            payload["entries"] = loadErrorLogEntries();
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--export-error-log") {
            std::cout << jsonString(exportErrorLogReport()) << '\n';
            return 0;
        }

        if (command == "--pulse-score") {
            const auto snapshot = collectSnapshotSafe(scanner);
            const auto tweaks = tweakEngine.listTweaks();
            const auto history = pulseBench.loadHistory();
            const auto advisorItems = systemAdvisor.analyze(snapshot, tweaks);
            QJsonObject payload = pulseScoreToJson(snapshot, tweaks, history, advisorItems);
            payload["ok"] = true;
            std::cout << jsonString(payload) << '\n';
            return 0;
        }

        if (command == "--scan") {
            const auto snapshot = collectSnapshotSafe(scanner);
            telemetryLogger.append(snapshot);
            selfTestLog << "scan," << pulseboost::currentTimestampUtc() << ',' << snapshot.healthScore << '\n';
            std::cout << "{\"ok\":true,\"healthScore\":" << snapshot.healthScore << "}\n";
            return 0;
        }

        if (command == "--status") {
            const auto snapshot = collectSnapshotSafe(scanner);
            std::cout << "{\"ok\":true,\"healthScore\":" << snapshot.healthScore << ","
                      << "\"cpuUsage\":" << snapshot.cpuUsagePercent << ","
                      << "\"ramUsage\":" << snapshot.ramUsagePercent << ","
                      << "\"diskUsage\":" << snapshot.diskUsagePercent << "}\n";
            return 0;
        }

        if (command == "--snapshot-json" || command == "--refresh-all") {
            const auto snapshot = collectSnapshotSafe(scanner);
            std::cout << snapshotToJson(snapshot) << '\n';
            return 0;
        }

        if (command == "--optimize-ram") {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            const bool advanced = hasCliFlag(argc, argv, "--advanced");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "ram.trim_working_sets", dryRun, hasCliFlag(argc, argv, "--confirm"), false, advanced, {}, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            pulseboost::RamOptimizer optimizer(processManager);
            const auto result = dryRun ? pulseboost::RamOptimizationResult{0, 0, false, "Dry run: working-set trim would require advanced manual confirmation."}
                                       : optimizer.optimizeWorkingSets(256.0, advanced);
            const bool ok = dryRun || result.processesOptimized > 0;
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("no-processes-trimmed"));
            payload["dryRun"] = dryRun;
            payload["advancedOnly"] = true;
            payload["optimized"] = static_cast<qint64>(result.processesOptimized);
            payload["skipped"] = static_cast<qint64>(result.processesSkipped);
            payload["message"] = QString::fromStdString(result.message);
            auditSafety(safetyPolicy, decision, dryRun, ok, "RAM working-set trim", {}, payload);
            std::cout << jsonString(payload) << '\n';
            return ok ? 0 : 1;
        }

        if (command == "--flush-standby") {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            const bool advanced = hasCliFlag(argc, argv, "--advanced");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "ram.flush_standby", dryRun, hasCliFlag(argc, argv, "--confirm"), false, advanced, {}, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            pulseboost::RamOptimizer optimizer(processManager);
            const auto result = dryRun ? pulseboost::RamOptimizationResult{0, 0, false, "Dry run: standby relief would require advanced manual confirmation."}
                                       : optimizer.flushStandbyList(advanced);
            const bool ok = dryRun || result.processesOptimized > 0;
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("no-processes-trimmed"));
            payload["dryRun"] = dryRun;
            payload["advancedOnly"] = true;
            payload["optimized"] = static_cast<qint64>(result.processesOptimized);
            payload["skipped"] = static_cast<qint64>(result.processesSkipped);
            payload["message"] = QString::fromStdString(result.message);
            auditSafety(safetyPolicy, decision, dryRun, ok, "RAM standby relief", {}, payload);
            std::cout << jsonString(payload) << '\n';
            return ok ? 0 : 1;
        }

        if (command == "--enable-ram-saver") {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            const bool advanced = hasCliFlag(argc, argv, "--advanced");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "ram.saver", dryRun, hasCliFlag(argc, argv, "--confirm"), false, advanced, {}, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            pulseboost::RamOptimizer optimizer(processManager);
            const auto result = dryRun ? pulseboost::RamOptimizationResult{0, 0, false, "Dry run: RAM saver would require advanced manual confirmation."}
                                       : optimizer.enableRamSaverMode(advanced);
            const bool ok = dryRun || result.processesOptimized > 0;
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("no-processes-trimmed"));
            payload["dryRun"] = dryRun;
            payload["advancedOnly"] = true;
            payload["optimized"] = static_cast<qint64>(result.processesOptimized);
            payload["skipped"] = static_cast<qint64>(result.processesSkipped);
            payload["message"] = QString::fromStdString(result.message);
            auditSafety(safetyPolicy, decision, dryRun, ok, "RAM saver", {}, payload);
            std::cout << jsonString(payload) << '\n';
            return ok ? 0 : 1;
        }

        if (command == "--optimize-disk") {
            pulseboost::SafetyDecision decision;
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            const auto blocked = evaluateSafety(safetyPolicy, "disk.optimize", dryRun, hasCliFlag(argc, argv, "--confirm"), false, hasCliFlag(argc, argv, "--advanced"), {}, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            const bool ok = dryRun || defragTrigger.optimizeSystemDrive();
            QJsonObject payload = strictResponse(dryRun || ok, ok || dryRun ? QString() : QStringLiteral("disk-optimizer-failed"));
            payload["dryRun"] = dryRun;
            auditSafety(safetyPolicy, decision, dryRun, dryRun || ok, "Disk optimize", {}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--flush-dns") {
            const bool ok = networkOptimizer.flushDns();
            std::cout << "{\"ok\":" << (ok ? "true" : "false") << "}\n";
            return ok ? 0 : 1;
        }

        if (command == "--optimize-tcp") {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            const bool advanced = hasCliFlag(argc, argv, "--advanced");
            const bool backupCreated = dryRun || networkOptimizer.backupNetworkSettings();
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "network.optimize_tcp", dryRun, hasCliFlag(argc, argv, "--confirm"), backupCreated, advanced, {}, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            const bool ok = dryRun || networkOptimizer.optimizeTcp(advanced);
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("netsh-failed"));
            payload["dryRun"] = dryRun;
            payload["advancedOnly"] = true;
            payload["backupCreated"] = backupCreated;
            auditSafety(safetyPolicy, decision, dryRun, ok, "Network TCP tune", {}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--revert-network") {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            const bool advanced = hasCliFlag(argc, argv, "--advanced");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "network.revert", dryRun, hasCliFlag(argc, argv, "--confirm"), true, advanced, {}, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            const bool ok = dryRun || networkOptimizer.revertNetworkSettings(advanced);
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("network-revert-failed"));
            payload["dryRun"] = dryRun;
            payload["advancedOnly"] = true;
            auditSafety(safetyPolicy, decision, dryRun, ok, "Network revert", {}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--check-latency") {
            const int latency = networkOptimizer.measureLatency();
            std::cout << "{\"ok\":" << (latency >= 0 ? "true" : "false") << ",\"latency\":" << latency << "}\n";
            return latency >= 0 ? 0 : 1;
        }

        if (command == "--network-diagnostics") {
            std::cout << jsonString(networkDiagnosticsToObject(networkOptimizer.diagnostics())) << '\n';
            return 0;
        }

        if (command == "--ram-breakdown") {
            pulseboost::RamOptimizer optimizer(processManager);
            std::cout << jsonString(ramBreakdownToObject(optimizer.currentBreakdown())) << '\n';
            return 0;
        }

        if (command == "--open-file-location" && argc > 2) {
            const auto path = resolveUserFileTarget(argv[2]);
            if (!path.has_value()) {
                std::cout << "{\"ok\":false,\"reason\":\"invalid-path\"}\n";
                return 1;
            }
            const bool ok = openFileLocation(*path);
            std::cout << "{\"ok\":" << (ok ? "true" : "false") << "}\n";
            return ok ? 0 : 1;
        }

        if (command == "--delete-file" && argc > 2) {
            const auto path = resolveUserFileTarget(argv[2]);
            if (!path.has_value() || !std::filesystem::is_regular_file(*path)) {
                std::cout << "{\"ok\":false,\"reason\":\"invalid-path\"}\n";
                return 1;
            }
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "file.delete", dryRun, hasCliFlag(argc, argv, "--confirm"), false, hasCliFlag(argc, argv, "--advanced"), path->string(), &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            std::error_code sizeError;
            const auto bytes = std::filesystem::file_size(*path, sizeError);
            std::error_code removeError;
            const bool ok = dryRun || std::filesystem::remove(*path, removeError);
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("delete-failed"));
            payload["dryRun"] = dryRun;
            payload["bytes"] = static_cast<qint64>(sizeError ? 0 : bytes);
            auditSafety(safetyPolicy, decision, dryRun, ok, "Delete file", QJsonObject{{"path", QString::fromStdString(path->string())}}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--kill-pid" && argc > 2) {
            const auto pid = static_cast<DWORD>(std::strtoul(argv[2], nullptr, 10));
            if (isCriticalProcessPid(processManager, pid)) {
                std::cout << "{\"ok\":false,\"reason\":\"critical-process\"}\n";
                return 0;
            }
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "process.kill", false, hasCliFlag(argc, argv, "--confirm"), false, hasCliFlag(argc, argv, "--advanced"), std::to_string(pid), &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }

            HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            bool success = false;
            if (processHandle != nullptr) {
                success = TerminateProcess(processHandle, 1) == TRUE;
                CloseHandle(processHandle);
            }
            QJsonObject payload = strictResponse(success, success ? QString() : QStringLiteral("terminate-failed"));
            payload["pid"] = static_cast<int>(pid);
            auditSafety(safetyPolicy, decision, false, success, "Kill process", QJsonObject{{"pid", static_cast<int>(pid)}}, payload);
            std::cout << jsonString(payload) << "\n";
            return success ? 0 : 1;
        }

        if (command == "--resume-pid" && argc > 2) {
            const auto pid = static_cast<std::uint32_t>(std::strtoul(argv[2], nullptr, 10));
            const bool ok = processManager.resumeProcess(pid);
            std::cout << "{\"ok\":" << (ok ? "true" : "false") << "}\n";
            return ok ? 0 : 1;
        }

        if (command == "--create-restore-point") {
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "restore.create", false, true, false, false, {}, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            const bool ok = safetyGuard.createRestorePoint(L"PulseBoost AI - CLI Restore Point");
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("restore-point-failed"));
            auditSafety(safetyPolicy, decision, false, ok, "Create restore point", {}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--enable-game-mode") {
            const auto snapshot = collectSnapshotSafe(scanner);
            std::optional<pulseboost::ProcessInfo> candidate;
            for (const auto &process : snapshot.heavyProcesses) {
                if (!process.isCritical && (!candidate.has_value() || process.cpuPercent > candidate->cpuPercent)) {
                    candidate = process;
                }
            }
            if (!candidate.has_value()) {
                std::cout << "{\"ok\":false,\"reason\":\"no-candidate\"}\n";
                return 1;
            }
            const bool ok = gameMode.enableForProcess(candidate->pid);
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.setValue(QStringLiteral("QtUi/gameModeActive"), ok);
            settings.setValue(QStringLiteral("QtUi/gameModeTargetPid"), static_cast<int>(candidate->pid));
            settings.setValue(QStringLiteral("QtUi/gameModeTargetName"), QString::fromStdString(candidate->name));
            settings.setValue(QStringLiteral("QtUi/gameModeTargetExecutable"), QString::fromStdString(candidate->name));
            settings.sync();
            std::cout << "{\"ok\":" << (ok ? "true" : "false")
                      << ",\"pid\":" << candidate->pid
                      << ",\"name\":\"" << escapeJson(candidate->name) << "\"}\n";
            return ok ? 0 : 1;
        }

        if (command == "--optimize-game" && argc > 2) {
            const auto profile = gameOptimizer.findGameProfile(argv[2]);
            if (!profile.has_value()) {
                std::cout << "{\"ok\":false,\"reason\":\"game-not-found\"}\n";
                return 0;
            }
            const auto pid = gameOptimizer.runningPidForExecutable(profile->executableName);
            if (!pid.has_value()) {
                std::cout << "{\"ok\":false,\"reason\":\"game-not-running\"}\n";
                return 0;
            }
            const bool ok = gameMode.enableForProcess(*pid);
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.setValue(QStringLiteral("QtUi/gameModeActive"), ok);
            settings.setValue(QStringLiteral("QtUi/gameModeTargetPid"), static_cast<int>(*pid));
            settings.setValue(QStringLiteral("QtUi/gameModeTargetName"), QString::fromStdString(profile->displayName));
            settings.setValue(QStringLiteral("QtUi/gameModeTargetExecutable"), QString::fromStdString(profile->executableName));
            settings.sync();
            std::cout << "{\"ok\":" << (ok ? "true" : "false")
                      << ",\"pid\":" << *pid
                      << ",\"name\":\"" << escapeJson(profile->displayName) << "\""
                      << ",\"executableName\":\"" << escapeJson(profile->executableName) << "\"}\n";
            return ok ? 0 : 1;
        }

        if (command == "--launch-optimized-game" && argc > 2) {
            const auto profile = gameOptimizer.findGameProfile(argv[2]);
            if (!profile.has_value() || !profile->isDetected || profile->installPath.empty()) {
                std::cout << "{\"ok\":false,\"reason\":\"game-not-launchable\"}\n";
                return 0;
            }

            const QString executablePath = QDir::toNativeSeparators(QString::fromStdString(profile->installPath + "\\" + profile->executableName));
            const bool launched = QProcess::startDetached(executablePath, {}, QString::fromStdString(profile->installPath));
            if (!launched) {
                std::cout << "{\"ok\":false,\"reason\":\"launch-failed\"}\n";
                return 0;
            }

            std::optional<std::uint32_t> pid;
            for (int attempt = 0; attempt < 30; ++attempt) {
                pid = gameOptimizer.runningPidForExecutable(profile->executableName);
                if (pid.has_value()) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            bool ok = true;
            if (pid.has_value()) {
                ok = gameMode.enableForProcess(*pid);
            }

            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.setValue(QStringLiteral("QtUi/gameModeActive"), ok && pid.has_value());
            settings.setValue(QStringLiteral("QtUi/gameModeTargetPid"), static_cast<int>(pid.value_or(0)));
            settings.setValue(QStringLiteral("QtUi/gameModeTargetName"), QString::fromStdString(profile->displayName));
            settings.setValue(QStringLiteral("QtUi/gameModeTargetExecutable"), QString::fromStdString(profile->executableName));
            settings.sync();

            std::cout << "{\"ok\":" << (ok ? "true" : "false")
                      << ",\"launched\":true"
                      << ",\"pending\":" << (pid.has_value() ? "false" : "true")
                      << ",\"pid\":" << pid.value_or(0)
                      << ",\"name\":\"" << escapeJson(profile->displayName) << "\""
                      << ",\"executableName\":\"" << escapeJson(profile->executableName) << "\"}\n";
            return 0;
        }

        if (command == "--disable-game-mode" || command == "--revert-game-optimization") {
            gameMode.disable();
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.setValue(QStringLiteral("QtUi/gameModeActive"), false);
            settings.remove(QStringLiteral("QtUi/gameModeTargetPid"));
            settings.remove(QStringLiteral("QtUi/gameModeTargetName"));
            settings.remove(QStringLiteral("QtUi/gameModeTargetExecutable"));
            settings.sync();
            std::cout << "{\"ok\":true}\n";
            return 0;
        }

        if (command == "--game-mode-status") {
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            bool active = settings.value(QStringLiteral("QtUi/gameModeActive"), false).toBool();
            int pid = settings.value(QStringLiteral("QtUi/gameModeTargetPid"), 0).toInt();
            const auto name = settings.value(QStringLiteral("QtUi/gameModeTargetName"), QString()).toString().toStdString();
            const auto executable = settings.value(QStringLiteral("QtUi/gameModeTargetExecutable"), QString()).toString().toStdString();
            if (active && pid > 0) {
                const auto livePid = gameOptimizer.runningPidForExecutable(executable.empty() ? name : executable);
                if (!livePid.has_value()) {
                    active = false;
                    pid = 0;
                    settings.setValue(QStringLiteral("QtUi/gameModeActive"), false);
                    settings.remove(QStringLiteral("QtUi/gameModeTargetPid"));
                    settings.sync();
                } else {
                    pid = static_cast<int>(*livePid);
                }
            }
            std::cout << "{\"ok\":true,\"active\":" << (active ? "true" : "false")
                      << ",\"pid\":" << pid
                      << ",\"name\":\"" << escapeJson(name) << "\""
                      << ",\"executableName\":\"" << escapeJson(executable) << "\"}\n";
            return 0;
        }

                if (command == "--disable-startup-by-name" && argc > 2) {
            const auto startupItems = startupOptimizer.scanStartupItems();
            const auto *item = findStartupByName(startupItems, argv[2]);
            if (item == nullptr) {
                std::cout << "{\"ok\":false,\"reason\":\"not-found\"}\n";
                return 1;
            }
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            const auto backupDir = std::filesystem::path("logs/startup_backups");
            std::error_code ec;
            std::filesystem::create_directories(backupDir, ec);
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "startup.disable", dryRun, hasCliFlag(argc, argv, "--confirm"), !ec, hasCliFlag(argc, argv, "--advanced"), item->name, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            const bool ok = dryRun || startupOptimizer.disableStartupItem(*item, backupDir);
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("startup-disable-failed"));
            payload["dryRun"] = dryRun;
            payload["name"] = QString::fromStdString(item->name);
            auditSafety(safetyPolicy, decision, dryRun, ok, "Disable startup item", QJsonObject{{"name", QString::fromStdString(item->name)}}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--enable-startup-by-name" && argc > 2) {
            const auto startupItems = startupOptimizer.scanStartupItems();
            const auto *item = findStartupByName(startupItems, argv[2]);
            if (item == nullptr) {
                std::cout << "{\"ok\":false,\"reason\":\"not-found\"}\n";
                return 1;
            }
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "startup.enable", dryRun, hasCliFlag(argc, argv, "--confirm"), false, hasCliFlag(argc, argv, "--advanced"), item->name, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            const bool ok = dryRun || startupOptimizer.enableStartupItem(*item);
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("startup-enable-failed"));
            payload["dryRun"] = dryRun;
            payload["name"] = QString::fromStdString(item->name);
            auditSafety(safetyPolicy, decision, dryRun, ok, "Enable startup item", QJsonObject{{"name", QString::fromStdString(item->name)}}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--delay-startup-by-name" && argc > 3) {
            const auto startupItems = startupOptimizer.scanStartupItems();
            const auto *item = findStartupByName(startupItems, argv[2]);
            if (item == nullptr) {
                std::cout << "{\"ok\":false,\"reason\":\"not-found\"}\n";
                return 1;
            }
            const int delaySeconds = std::max(1, std::atoi(argv[3]));
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy, "startup.delay", dryRun, hasCliFlag(argc, argv, "--confirm"), false, hasCliFlag(argc, argv, "--advanced"), item->name, &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }
            const bool ok = dryRun || startupOptimizer.delayStartupItem(*item, delaySeconds);
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("startup-delay-failed"));
            payload["dryRun"] = dryRun;
            payload["name"] = QString::fromStdString(item->name);
            payload["delaySeconds"] = delaySeconds;
            auditSafety(safetyPolicy, decision, dryRun, ok, "Delay startup item", QJsonObject{{"name", QString::fromStdString(item->name)}, {"delaySeconds", delaySeconds}}, payload);
            std::cout << jsonString(payload) << "\n";
            return ok ? 0 : 1;
        }

        if (command == "--get-scheduled-tasks") {
            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.beginGroup(QStringLiteral("ScheduledTasks"));

            const QStringList taskIds = settings.childGroups();
            std::ostringstream response;
            response << "{\"tasks\":[";
            for (int index = 0; index < taskIds.size(); ++index) {
                const QString &taskId = taskIds.at(index);
                settings.beginGroup(taskId);
                if (index > 0) {
                    response << ",";
                }
                response << "{"
                         << "\"id\":\"" << escapeJson(taskId.toStdString()) << "\"," 
                         << "\"enabled\":" << (settings.value(QStringLiteral("enabled"), false).toBool() ? "true" : "false") << ","
                         << "\"type\":\"" << escapeJson(settings.value(QStringLiteral("type"), QString()).toString().toStdString()) << "\"," 
                         << "\"intervalHours\":" << settings.value(QStringLiteral("intervalHours"), 24).toInt() << ","
                         << "\"lastRun\":\"" << escapeJson(settings.value(QStringLiteral("lastRun"), QString()).toString().toStdString()) << "\""
                         << "}";
                settings.endGroup();
            }
            response << "]}";
            settings.endGroup();

            std::cout << response.str() << '\n';
            return 0;
        }

        if (command == "--set-scheduled-task" && argc > 5) {
            const QString taskId = QString::fromUtf8(argv[2]).trimmed();
            const bool enabled = parseBooleanString(argv[3]);
            const QString taskType = QString::fromUtf8(argv[4]).trimmed().toLower();
            const int intervalHours = std::max(1, std::atoi(argv[5]));

            if (taskId.isEmpty()) {
                std::cout << "{\"ok\":false,\"reason\":\"invalid-task-id\"}" << '\n';
                return 1;
            }

            QSettings settings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
            settings.beginGroup(QStringLiteral("ScheduledTasks"));
            settings.beginGroup(taskId);
            settings.setValue(QStringLiteral("enabled"), enabled);
            settings.setValue(QStringLiteral("type"), taskType);
            settings.setValue(QStringLiteral("intervalHours"), intervalHours);
            settings.setValue(QStringLiteral("lastRun"), QString());
            settings.endGroup();
            settings.endGroup();
            settings.sync();

            bool ok = true;
            if (enabled) {
                ok = startupOptimizer.scheduleTask(taskId.toStdString(), taskType.toStdString(), intervalHours);
            }

            std::cout << "{\"ok\":" << (ok ? "true" : "false") << "}" << '\n';
            return ok ? 0 : 1;
        }
        if (command == "--take-snapshot") {
            const auto snapshot = collectSnapshotSafe(scanner);
            const auto baseDir = std::filesystem::path("data/snapshots");
            std::error_code ec;
            std::filesystem::create_directories(baseDir, ec);
            std::string safeTimestamp = pulseboost::currentTimestampUtc();
            std::replace(safeTimestamp.begin(), safeTimestamp.end(), ':', '-');
            std::replace(safeTimestamp.begin(), safeTimestamp.end(), 'T', '_');
            const auto fileName = std::string("snapshot_") + safeTimestamp + ".json";
            const auto path = baseDir / fileName;
            std::ofstream output(path, std::ios::trunc);
            if (!output.is_open()) {
                std::cout << "{\"ok\":false}\n";
                return 1;
            }
            output << snapshotToJson(snapshot);
            output.close();
            std::cout << "{\"ok\":true,\"path\":\"" << escapeJson(path.string()) << "\"}\n";
            return 0;
        }

        if (command == "--restore-snapshot" && argc > 2) {
            std::filesystem::path snapshotPath = argv[2];
            std::error_code error;
            if (!std::filesystem::exists(snapshotPath, error) || error) {
                snapshotPath = std::filesystem::path("data/snapshots") / argv[2];
            }
            const QJsonObject payload = restoreSnapshotFromFile(snapshotPath, startupOptimizer, safetyGuard, optimizationHistory);
            std::cout << jsonString(payload) << '\n';
            return payload.value(QStringLiteral("ok")).toBool() ? 0 : 1;
        }

        if (command == "--recent-actions-json") {
            auto records = optimizationHistory.load();
            std::reverse(records.begin(), records.end());
            if (records.size() > 20) {
                records.resize(20);
            }
            std::cout << recentActionsToJson(records) << '\n';
            return 0;
        }

        if (command == "--scan-large-files") {
            std::vector<std::filesystem::path> roots;
            for (int index = 2; index < argc; ++index) {
                std::error_code error;
                const std::filesystem::path candidate = argv[index];
                if (std::filesystem::exists(candidate, error) && !error) {
                    roots.push_back(candidate);
                }
            }
            if (roots.empty()) {
                roots = defaultDuplicateRoots();
            }

            std::vector<pulseboost::FileEntry> files;
            for (const auto &root : roots) {
                const auto scanned = largeFileScanner.scan(root, 256ULL * 1024ULL * 1024ULL, 20);
                files.insert(files.end(), scanned.begin(), scanned.end());
            }
            std::sort(files.begin(), files.end(), [](const auto &left, const auto &right) {
                return left.bytes > right.bytes;
            });
            if (files.size() > 50) {
                files.resize(50);
            }
            std::cout << largeFilesToJson(files) << '\n';
            return 0;
        }

        if (command == "--run-security-scan") {
            const auto snapshot = collectSnapshotSafe(scanner);
            std::cout << "{\"ok\":true,\"securityScore\":" << snapshot.securityScore << "}\n";
            return 0;
        }

                if (command == "--find-duplicates") {
            std::vector<std::filesystem::path> roots;
            for (int index = 2; index < argc; ++index) {
                std::error_code error;
                const std::filesystem::path candidate = argv[index];
                if (std::filesystem::exists(candidate, error) && !error) {
                    roots.push_back(candidate);
                }
            }
            if (roots.empty()) {
                roots = defaultDuplicateRoots();
            }

            std::vector<pulseboost::DuplicateGroup> groups;
            for (const auto &root : roots) {
                const auto rootGroups = duplicateFileFinder.scan(root);
                groups.insert(groups.end(), rootGroups.begin(), rootGroups.end());
            }

            std::cout << duplicateGroupsToJson(groups) << '\n';
            return 0;
        }
        if (command == "--estimate-junk") {
            const auto bytes = junkCleaner.estimateRecoverableBytes();
            std::cout << "{\"ok\":true,\"bytes\":" << bytes << "}\n";
            return 0;
        }

        if (command == "--clean") {
            const bool dryRun = hasCliFlag(argc, argv, "--dry-run");
            pulseboost::CleanupOptions options;
            options.dryRun = dryRun;
            options.mode = hasCliFlag(argc, argv, "--recycle")
                ? pulseboost::CleanupMode::Recycle
                : (hasCliFlag(argc, argv, "--permanent-delete") ? pulseboost::CleanupMode::PermanentDelete : pulseboost::CleanupMode::Quarantine);
            const bool permanentDelete = options.mode == pulseboost::CleanupMode::PermanentDelete;
            pulseboost::SafetyDecision decision;
            const auto blocked = evaluateSafety(safetyPolicy,
                                                permanentDelete ? "junk.clean.permanent" : "junk.clean",
                                                dryRun,
                                                hasCliFlag(argc, argv, "--confirm"),
                                                !permanentDelete,
                                                hasCliFlag(argc, argv, "--advanced"),
                                                {},
                                                &decision);
            if (!blocked.isEmpty()) {
                std::cout << jsonString(blocked) << '\n';
                return 1;
            }

            const auto result = junkCleaner.cleanSafeTargets(options);
            const bool ok = result.failures == 0;
            optimizationHistory.record(pulseboost::ActionRecord {
                .timestampUtc = pulseboost::currentTimestampUtc(),
                .action = "cli-clean",
                .details = "Recovered " + std::to_string(result.bytesRecovered) + " bytes",
                .success = ok,
            });
            selfTestLog << "clean," << pulseboost::currentTimestampUtc() << ',' << result.filesRemoved << '\n';
            QJsonObject payload = strictResponse(ok, ok ? QString() : QStringLiteral("cleanup-partial-failure"));
            payload["dryRun"] = result.dryRun;
            payload["bytesRecovered"] = static_cast<qint64>(result.bytesRecovered);
            payload["filesScanned"] = result.filesScanned;
            payload["filesRemoved"] = result.filesRemoved;
            payload["filesQuarantined"] = result.filesQuarantined;
            payload["filesRecycled"] = result.filesRecycled;
            payload["failures"] = result.failures;
            payload["permanentDelete"] = result.permanentDelete;
            auditSafety(safetyPolicy, decision, dryRun, ok, "Junk cleanup", {}, payload);
            std::cout << jsonString(payload) << '\n';
            return ok ? 0 : 1;
        }


        if (command == "--self-test") {
            const auto snapshot = collectSnapshotSafe(scanner);
            telemetryLogger.append(snapshot);

            double diskUsedPercent = 0.0;
            std::size_t startupCount = 0;
            try {
                const auto disk = diskAnalyzer.analyzeSystemDrive();
                diskUsedPercent = disk.usedPercent;
            } catch (...) {
                diskUsedPercent = snapshot.diskUsagePercent;
            }
            try {
                startupCount = startupOptimizer.scanStartupItems().size();
            } catch (...) {
                startupCount = static_cast<std::size_t>(snapshot.startupPrograms);
            }

            selfTestLog << "self-test," << pulseboost::currentTimestampUtc() << ',' << snapshot.healthScore << ','
                        << diskUsedPercent << ',' << startupCount << '\n';
            QJsonObject payload = strictResponse(true);
            payload["healthScore"] = snapshot.healthScore;
            payload["diskUsedPercent"] = diskUsedPercent;
            payload["startupCount"] = static_cast<qint64>(startupCount);
            std::cout << jsonString(payload) << '\n';
            return 0;
        }
        if (command == "--chat") {
            std::string message;
            for (int index = 2; index < argc; ++index) {
                if (!message.empty()) {
                    message += ' ';
                }
                message += argv[index];
            }

            const auto snapshot = collectSnapshotSafe(scanner);
            pulseboost::AiDiagnosticsEngine diagnosticsEngine;
            const auto decision = diagnosticsEngine.reason(message, snapshot, {});
            QJsonObject payload = strictResponse(!decision.reply.empty(), decision.reply.empty() ? QStringLiteral("empty-ai-response") : QString());
            payload["reply"] = QString::fromStdString(decision.reply);
            std::cout << jsonString(payload) << '\n';
            selfTestLog << "chat," << pulseboost::currentTimestampUtc() << ',' << snapshot.healthScore << '\n';
            return decision.reply.empty() ? 1 : 0;
        }
        std::cout << "{\"ok\":false,\"reason\":\"unknown-command\"}\n";
        return 2;
    }

    appendGuiLog("GUI bootstrap start");
    QQuickStyle::setStyle("Basic");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);

    QGuiApplication app(argc, argv);
    app.setApplicationName("PulseBoost AI");
    app.setOrganizationName("PulseBoost");
    app.setApplicationVersion("1.0.0");

    QSettings::setDefaultFormat(QSettings::IniFormat);

    const QStringList fontResources {
        QStringLiteral(":/fonts/Rajdhani-Regular.ttf"),
        QStringLiteral(":/fonts/Rajdhani-Medium.ttf"),
        QStringLiteral(":/fonts/Rajdhani-SemiBold.ttf"),
        QStringLiteral(":/fonts/Rajdhani-Bold.ttf"),
        QStringLiteral(":/fonts/IBMPlexSans-Regular.ttf"),
        QStringLiteral(":/fonts/IBMPlexSans-Medium.ttf"),
        QStringLiteral(":/fonts/IBMPlexSans-SemiBold.ttf"),
        QStringLiteral(":/fonts/IBMPlexMono-Regular.ttf"),
        QStringLiteral(":/fonts/IBMPlexMono-Medium.ttf"),
    };
    for (const QString &resource : fontResources) {
        QFontDatabase::addApplicationFont(resource);
    }
    const QStringList loadedFamilies = QFontDatabase::families();
    qDebug() << "Rajdhani loaded:" << loadedFamilies.contains("Rajdhani");
    qDebug() << "IBM Plex Sans loaded:" << loadedFamilies.contains("IBM Plex Sans");
    qDebug() << "IBM Plex Mono loaded:" << loadedFamilies.contains("IBM Plex Mono");

    pulseboost::ProcessManager processManager;
    pulseboost::ServiceManager serviceManager;
    pulseboost::RegistryOptimizer registryOptimizer;
    pulseboost::StartupOptimizer startupOptimizer(registryOptimizer);
    pulseboost::MemoryAnalyzer memoryAnalyzer(processManager);
    pulseboost::DiskAnalyzer diskAnalyzer;
    pulseboost::SystemScanner scanner(processManager,
                                      memoryAnalyzer,
                                      diskAnalyzer,
                                      startupOptimizer,
                                      serviceManager);
    pulseboost::TelemetryLogger telemetryLogger;
    pulseboost::OptimizationHistory optimizationHistory;
    pulseboost::JunkCleaner junkCleaner;
    pulseboost::GameMode gameMode(processManager, serviceManager);
    pulseboost::DeveloperMode developerMode(processManager, serviceManager);
    pulseboost::SafetyGuard safetyGuard;
    pulseboost::TelemetryCache telemetryCache(300);
    pulseboost::NetworkOptimizer networkOptimizer;
    pulseboost::TweakEngine tweakEngine(registryOptimizer, serviceManager);
    pulseboost::PulseBench pulseBench;
    pulseboost::SystemAdvisor systemAdvisor;
    pulseboost::GameOptimizer gameOptimizer(processManager, networkOptimizer);
    pulseboost::LicenseManager licenseManager;
    pulseboost::FeatureGate featureGate(licenseManager);
    pulseboost::UiPreferences uiPreferences;

    const bool crashDetected = pulseboost::CrashReporter::hasPendingCrashReport();
    const QString crashReportPath = QString::fromStdString(pulseboost::CrashReporter::pendingCrashReportPath());
    if (crashDetected) {
        pulseboost::CrashReporter::clearPendingCrashReport();
    }

    pulseboost::AutoUpdater autoUpdater;
    const bool updateAvailable = autoUpdater.checkForUpdates(app.applicationVersion().toStdString());
    const QString updateVersion = QString::fromStdString(autoUpdater.latestVersion());
    const QString updateUrl = QString::fromStdString(autoUpdater.downloadUrl());
    pulseboost::UiController uiController(junkCleaner,
                                          gameMode,
                                          processManager,
                                          diskAnalyzer,
                                          startupOptimizer,
                                          safetyGuard,
                                          telemetryCache,
                                          optimizationHistory,
                                          networkOptimizer,
                                          tweakEngine,
                                          gameOptimizer,
                                          systemAdvisor,
                                          pulseBench);

    pulseboost::AgentEngine agentEngine(scanner,
                                        telemetryCache,
                                        junkCleaner,
                                        processManager,
                                        startupOptimizer,
                                        gameMode,
                                        developerMode,
                                        optimizationHistory,
                                        safetyGuard,
                                        networkOptimizer);

    pulseboost::TelemetryEngine telemetryEngine(scanner);

    QObject::connect(&telemetryEngine,
                     &pulseboost::TelemetryEngine::snapshotReady,
                     &uiController,
                     [&](const pulseboost::SystemSnapshot &snap) {
                         telemetryCache.push(snap);
                         uiController.onSnapshotReady(snap);
                         telemetryLogger.append(snap);
                         agentEngine.updateSnapshot(snap);
                     });

    QQmlApplicationEngine engine;
    appendGuiLog("QQmlApplicationEngine created");

    QObject::connect(&engine, &QQmlEngine::warnings, &app, [](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) {
            appendGuiLog("QML warning: " + warning.toString().toStdString());
        }
    });

    engine.rootContext()->setContextProperty("SystemCtrl", &uiController);
    engine.rootContext()->setContextProperty("AgentEngine", &agentEngine);
    engine.rootContext()->setContextProperty("FeatureGate", &featureGate);
    engine.rootContext()->setContextProperty("UiPrefs", &uiPreferences);
    engine.rootContext()->setContextProperty("StartupCrashDetected", crashDetected);
    engine.rootContext()->setContextProperty("StartupCrashReportPath", crashReportPath);
    engine.rootContext()->setContextProperty("StartupUpdateAvailable", updateAvailable);
    engine.rootContext()->setContextProperty("StartupUpdateVersion", updateVersion);
    engine.rootContext()->setContextProperty("StartupUpdateUrl", updateUrl);
    appendGuiLog("Context properties registered");

    engine.addImportPath(QStringLiteral("qrc:/ui/qml"));
    engine.addImportPath(QStringLiteral("qrc:/ui/qml/style"));

    const QUrl url(QStringLiteral("qrc:/ui/qml/main.qml"));
    QObject::connect(&engine,
                     &QQmlApplicationEngine::objectCreated,
                     &app,
                     [url](QObject *obj, const QUrl &objUrl) {
                         appendGuiLog(std::string("objectCreated: ")
                                      + (obj ? "success " : "failure ")
                                      + objUrl.toString().toStdString());
                         if (!obj && url == objUrl) {
                             QCoreApplication::exit(-1);
                         }
                     },
                     Qt::QueuedConnection);

    appendGuiLog("Loading QML: " + url.toString().toStdString());
    engine.load(url);
    appendGuiLog("Root object count: " + std::to_string(engine.rootObjects().size()));

    telemetryEngine.start();
    appendGuiLog("Telemetry engine started");

    const int exitCode = app.exec();
    appendGuiLog("Event loop exited with code " + std::to_string(exitCode));
    telemetryEngine.stop();
    return exitCode;
    } catch (const std::exception &ex) {
        std::ofstream selfTestLog("logs/self_test.log", std::ios::app);
        selfTestLog << "error," << pulseboost::currentTimestampUtc() << "," << ex.what() << '\n';
        if (argc > 1) {
            std::cout << "{\"ok\":false,\"reason\":\"exception\",\"message\":\"" << escapeJson(ex.what()) << "\"}\n";
        }
        return 1;
    } catch (...) {
        std::ofstream selfTestLog("logs/self_test.log", std::ios::app);
        selfTestLog << "error," << pulseboost::currentTimestampUtc() << ",unknown\n";
        if (argc > 1) {
            std::cout << "{\"ok\":false,\"reason\":\"unknown-exception\"}\n";
        }
        return 1;
    }
}
















































