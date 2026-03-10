#include "PulseBoostAI/ui_backend/ui_controller.hpp"

#include <Windows.h>

#include <QVariantMap>

#include <algorithm>
#include <cmath>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

UiController::UiController(JunkCleaner &junkCleaner,
                           GameMode &gameMode,
                           ProcessManager &processManager,
                           SafetyGuard &safetyGuard,
                           TelemetryCache &telemetryCache,
                           OptimizationHistory &history,
                           QObject *parent)
    : QObject(parent),
      junkCleaner_(junkCleaner),
      gameMode_(gameMode),
      processManager_(processManager),
      safetyGuard_(safetyGuard),
      telemetryCache_(telemetryCache),
      history_(history) {}

QString UiController::healthLabel() const {
    if (snapshot_.healthScore >= 80) {
        return "Stable";
    }
    if (snapshot_.healthScore >= 60) {
        return "Attention";
    }
    return "Critical";
}

QVariantList UiController::chartSeries() const {
    return telemetryCache_.chartSeries(60);
}

void UiController::onSnapshotReady(const SystemSnapshot &snapshot) {
    snapshot_ = snapshot;
    emit metricsChanged();
    emit chartSeriesChanged();
    emit processListChanged();
    emit storageChanged();
}

QVariantList UiController::processList() const {
    return sortedProcesses("cpu", true);
}

QVariantList UiController::sortedProcesses(const QString &sortKey, bool descending) const {
    auto processes = snapshot_.heavyProcesses;
    std::sort(processes.begin(), processes.end(), [&](const ProcessInfo &left, const ProcessInfo &right) {
        if (sortKey == "name") {
            return descending ? left.name > right.name : left.name < right.name;
        }
        if (sortKey == "ram") {
            return descending ? left.memoryMb > right.memoryMb : left.memoryMb < right.memoryMb;
        }
        return descending ? left.cpuPercent > right.cpuPercent : left.cpuPercent < right.cpuPercent;
    });

    QVariantList list;
    for (const auto &process : processes) {
        QVariantMap map;
        map["pid"] = static_cast<int>(process.pid);
        map["name"] = QString::fromStdString(process.name);
        map["cpuPercent"] = process.cpuPercent;
        map["memoryMb"] = process.memoryMb;
        map["isCritical"] = process.isCritical;
        map["accent"] = process.cpuPercent > 20.0 ? "#f85149" : (process.memoryMb > 800.0 ? "#d29922" : "#3fb950");
        list.push_back(map);
    }
    return list;
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

    const bool verticalSplit = width >= height;
    double cursor = 0.0;
    for (std::size_t index = 0; index < snapshot_.storageCategories.size(); ++index) {
        const auto &category = snapshot_.storageCategories[index];
        const double fraction = static_cast<double>(category.bytes) / static_cast<double>(total);
        if (verticalSplit) {
            const double remainingWidth = std::max(0.0, static_cast<double>(width) - cursor);
            const double nodeWidth = index + 1 == snapshot_.storageCategories.size()
                                         ? remainingWidth
                                         : std::max(1.0, std::round(fraction * width));
            nodes.push_back(storageNodeToMap(category,
                                             cursor,
                                             0.0,
                                             std::min(nodeWidth, remainingWidth),
                                             static_cast<double>(height),
                                             fraction));
            cursor += nodeWidth;
        } else {
            const double remainingHeight = std::max(0.0, static_cast<double>(height) - cursor);
            const double nodeHeight = index + 1 == snapshot_.storageCategories.size()
                                          ? remainingHeight
                                          : std::max(1.0, std::round(fraction * height));
            nodes.push_back(storageNodeToMap(category,
                                             0.0,
                                             cursor,
                                             static_cast<double>(width),
                                             std::min(nodeHeight, remainingHeight),
                                             fraction));
            cursor += nodeHeight;
        }
    }

    return nodes;
}

QVariantList UiController::recentActions() const {
    QVariantList list;
    auto records = history_.load();
    std::reverse(records.begin(), records.end());
    for (const auto &record : records) {
        QVariantMap map;
        map["timestamp"] = QString::fromStdString(record.timestampUtc);
        map["action"] = QString::fromStdString(record.action);
        map["details"] = QString::fromStdString(record.details);
        map["success"] = record.success;
        list.push_back(map);
    }
    return list;
}

void UiController::appendAction(const std::string &action, const std::string &details, bool success) {
    history_.record(ActionRecord {
        .timestampUtc = currentTimestampUtc(),
        .action = action,
        .details = details,
        .success = success,
    });
    emit actionsChanged();
}

void UiController::runClean() {
    appendAction("junk-cleaner", "Starting safe cleanup", true);
    const auto result = junkCleaner_.cleanSafeTargets();
    const QString message = QString("Cleaned %1 files and recovered %2.")
                                .arg(result.filesRemoved)
                                .arg(QString::fromStdString(formatBytes(result.bytesRecovered)));
    appendAction("junk-cleaner", message.toStdString(), true);
    emit actionFeedback(message, true);
}

void UiController::runOptimize() {
    const bool restorePointOk = safetyGuard_.createRestorePoint(L"PulseBoost AI - optimize");
    appendAction("restore-point", "Pre-optimization restore point", restorePointOk);
    runClean();
}

void UiController::enableGameMode() {
    for (const auto &process : snapshot_.heavyProcesses) {
        if (process.isCritical || process.memoryMb < 512.0) {
            continue;
        }

        const bool ok = gameMode_.enableForProcess(process.pid);
        appendAction("game-mode", "Target " + process.name, ok);
        emit actionFeedback(ok ? "Game mode enabled for " + QString::fromStdString(process.name) + "."
                               : "Failed to enable game mode for " + QString::fromStdString(process.name) + ".",
                            ok);
        return;
    }

    emit actionFeedback("No suitable active game process was detected.", false);
}

void UiController::killProcess(int pid) {
    HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    bool success = false;
    if (processHandle != nullptr) {
        success = TerminateProcess(processHandle, 1) == TRUE;
        CloseHandle(processHandle);
    }

    appendAction("kill-process", "PID " + std::to_string(pid), success);
    emit actionFeedback(success ? QString("Terminated process %1.").arg(pid)
                                : QString("Could not terminate process %1.").arg(pid),
                        success);
}

void UiController::createRestorePoint() {
    const bool success = safetyGuard_.createRestorePoint(L"PulseBoost AI - manual restore point");
    appendAction("restore-point", "Manual restore point", success);
    emit actionFeedback(success ? "Windows restore point created."
                                : "Restore point creation failed.",
                        success);
}

}  // namespace pulseboost
