#include "PulseBoostAI/modules/game_mode.hpp"

#include <Windows.h>

#include <unordered_set>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

DWORD queryPriority(std::uint32_t pid) {
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (processHandle == nullptr) {
        return NORMAL_PRIORITY_CLASS;
    }
    const DWORD priority = GetPriorityClass(processHandle);
    CloseHandle(processHandle);
    return priority == 0 ? NORMAL_PRIORITY_CLASS : priority;
}

}  // namespace

GameMode::GameMode(ProcessManager &processManager, ServiceManager &serviceManager)
    : processManager_(processManager), serviceManager_(serviceManager) {}

bool GameMode::enableForProcess(std::uint32_t gamePid) {
    if (active_) {
        return true;
    }

    previousPriorities_[gamePid] = queryPriority(gamePid);
    processManager_.setPriority(gamePid, HIGH_PRIORITY_CLASS);

    const std::unordered_set<std::string> backgroundApps = {"Discord.exe", "Teams.exe", "OneDrive.exe", "SteamWebHelper.exe",
                                                            "Chrome.exe", "msedge.exe"};
    for (const auto &process : processManager_.enumerateProcesses()) {
        if (process.pid == gamePid || process.isCritical || !backgroundApps.contains(process.name)) {
            continue;
        }

        previousPriorities_[process.pid] = queryPriority(process.pid);
        processManager_.setPriority(process.pid, BELOW_NORMAL_PRIORITY_CLASS);
    }

    runProcessHidden(L"powercfg /setactive SCHEME_MIN");
    active_ = true;
    return true;
}

void GameMode::disable() {
    for (const auto &[pid, priority] : previousPriorities_) {
        processManager_.setPriority(pid, priority);
    }

    previousPriorities_.clear();
    runProcessHidden(L"powercfg /setactive SCHEME_BALANCED");
    active_ = false;
}

bool GameMode::isActive() const {
    return active_;
}

}  // namespace pulseboost
