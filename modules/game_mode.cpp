#include "PulseBoostAI/modules/game_mode.hpp"

#include <Windows.h>

#include <unordered_map>
#include <unordered_set>

#include <QSettings>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

QSettings sessionSettings() {
    return QSettings(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoost AI"));
}

DWORD queryPriority(std::uint32_t pid) {
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (processHandle == nullptr) {
        return NORMAL_PRIORITY_CLASS;
    }
    const DWORD priority = GetPriorityClass(processHandle);
    CloseHandle(processHandle);
    return priority == 0 ? NORMAL_PRIORITY_CLASS : priority;
}

void persistSession(const std::unordered_map<std::uint32_t, DWORD> &priorities) {
    auto settings = sessionSettings();
    settings.beginGroup(QStringLiteral("GameModeSession"));
    settings.remove(QString());
    settings.setValue(QStringLiteral("active"), !priorities.empty());
    settings.beginWriteArray(QStringLiteral("priorities"));
    int index = 0;
    for (const auto &[pid, priority] : priorities) {
        settings.setArrayIndex(index++);
        settings.setValue(QStringLiteral("pid"), static_cast<qulonglong>(pid));
        settings.setValue(QStringLiteral("priority"), static_cast<qulonglong>(priority));
    }
    settings.endArray();
    settings.endGroup();
    settings.sync();
}

std::unordered_map<std::uint32_t, DWORD> loadSession() {
    std::unordered_map<std::uint32_t, DWORD> priorities;
    auto settings = sessionSettings();
    settings.beginGroup(QStringLiteral("GameModeSession"));
    const int count = settings.beginReadArray(QStringLiteral("priorities"));
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        const auto pid = static_cast<std::uint32_t>(settings.value(QStringLiteral("pid"), 0).toULongLong());
        const auto priority = static_cast<DWORD>(settings.value(QStringLiteral("priority"), NORMAL_PRIORITY_CLASS).toULongLong());
        if (pid != 0) {
            priorities[pid] = priority;
        }
    }
    settings.endArray();
    settings.endGroup();
    return priorities;
}

void clearSession() {
    auto settings = sessionSettings();
    settings.beginGroup(QStringLiteral("GameModeSession"));
    settings.remove(QString());
    settings.endGroup();
    settings.sync();
}

}  // namespace

GameMode::GameMode(ProcessManager &processManager, ServiceManager &serviceManager)
    : processManager_(processManager), serviceManager_(serviceManager) {}

bool GameMode::enableForProcess(std::uint32_t gamePid) {
    if (gamePid == 0) {
        return false;
    }

    if (active_ || !previousPriorities_.empty() || !loadSession().empty()) {
        disable();
    }

    previousPriorities_.clear();
    previousPriorities_[gamePid] = queryPriority(gamePid);
    bool ok = processManager_.setPriority(gamePid, HIGH_PRIORITY_CLASS);

    const std::unordered_set<std::string> backgroundApps = {"Discord.exe", "Teams.exe", "OneDrive.exe", "SteamWebHelper.exe",
                                                            "Chrome.exe", "msedge.exe"};
    for (const auto &process : processManager_.enumerateProcesses()) {
        if (process.pid == gamePid || process.isCritical || !backgroundApps.contains(process.name)) {
            continue;
        }

        previousPriorities_[process.pid] = queryPriority(process.pid);
        ok = processManager_.setPriority(process.pid, BELOW_NORMAL_PRIORITY_CLASS) && ok;
    }

    runProcessHidden(L"powercfg /setactive SCHEME_MIN");
    persistSession(previousPriorities_);
    active_ = true;
    return ok;
}

void GameMode::disable() {
    if (previousPriorities_.empty()) {
        previousPriorities_ = loadSession();
    }

    for (const auto &[pid, priority] : previousPriorities_) {
        processManager_.setPriority(pid, priority);
    }

    previousPriorities_.clear();
    clearSession();
    runProcessHidden(L"powercfg /setactive SCHEME_BALANCED");
    active_ = false;
}

bool GameMode::isActive() const {
    return active_;
}

}  // namespace pulseboost
