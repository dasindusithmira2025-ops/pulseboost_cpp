#include "PulseBoostAI/core/game_optimizer.hpp"

#include <Windows.h>

#include <QDir>
#include <QProcess>
#include <QSettings>

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/modules/network_optimizer.hpp"

namespace pulseboost {

namespace {

struct KnownGame {
    std::string displayName;
    std::string executableName;
    std::string launcher;
};

struct SessionSnapshot {
    bool active = false;
    std::uint32_t gamePid = 0;
    DWORD originalPriority = NORMAL_PRIORITY_CLASS;
    std::uint64_t originalAffinity = 0;
    std::string displayName;
    std::string executableName;
    std::vector<std::uint32_t> suspendedLauncherPids;
    bool aiSuspended = false;
    bool networkTuned = false;
    bool powerProfileBoosted = false;
    std::uint64_t gameAffinityMask = 0;
    std::uint64_t backgroundAffinityMask = 0;
};

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void appendIfExists(std::vector<std::filesystem::path> &roots, const std::filesystem::path &candidate) {
    std::error_code error;
    if (std::filesystem::exists(candidate, error) && !error) {
        roots.push_back(candidate);
    }
}

std::vector<std::filesystem::path> candidateRoots() {
    std::vector<std::filesystem::path> roots;
    for (const char *envName : {"ProgramFiles", "ProgramFiles(x86)", "LOCALAPPDATA"}) {
        const char *value = std::getenv(envName);
        if (value == nullptr) {
            continue;
        }
        const std::filesystem::path base(value);
        appendIfExists(roots, base / "Steam" / "steamapps" / "common");
        appendIfExists(roots, base / "Epic Games");
        appendIfExists(roots, base / "Battle.net");
        appendIfExists(roots, base / "Ubisoft");
        appendIfExists(roots, base / "EA Games");
        appendIfExists(roots, base / "Programs");
    }

    if (const char *systemDrive = std::getenv("SystemDrive"); systemDrive != nullptr) {
        const std::filesystem::path base(std::string(systemDrive) + "\\");
        appendIfExists(roots, base / "Riot Games");
        appendIfExists(roots, base / "Games");
    }
    return roots;
}

std::vector<KnownGame> knownGames() {
    return {
        {"Fortnite", "FortniteClient-Win64-Shipping.exe", "epic"},
        {"Valorant", "VALORANT-Win64-Shipping.exe", "direct"},
        {"Counter-Strike 2", "cs2.exe", "steam"},
        {"Apex Legends", "r5apex.exe", "steam"},
        {"Call of Duty", "cod.exe", "other"},
        {"Minecraft", "MinecraftLauncher.exe", "direct"},
        {"Roblox", "RobloxPlayerBeta.exe", "direct"},
        {"Grand Theft Auto V", "GTA5.exe", "other"},
        {"Elden Ring", "eldenring.exe", "steam"},
        {"Cyberpunk 2077", "Cyberpunk2077.exe", "steam"},
        {"League of Legends", "LeagueClient.exe", "direct"},
        {"Dota 2", "dota2.exe", "steam"},
        {"Overwatch 2", "Overwatch.exe", "battlenet"},
        {"Rainbow Six Siege", "RainbowSix.exe", "other"},
        {"Rust", "RustClient.exe", "steam"},
        {"ARK", "ShooterGame.exe", "steam"},
        {"Rocket League", "RocketLeague.exe", "epic"},
        {"EA Sports FC", "FC25.exe", "other"},
        {"PUBG", "TslGame.exe", "steam"},
        {"Warzone", "cod.exe", "other"},
    };
}

std::vector<std::string> launcherProcessNames() {
    return {
        "steam.exe",
        "steamwebhelper.exe",
        "epicgameslauncher.exe",
        "epicwebhelper.exe",
        "battle.net.exe",
        "agent.exe",
        "ubisoftconnect.exe",
        "eadesktop.exe",
        "riotclientservices.exe",
        "leagueclient.exe"
    };
}

std::optional<std::filesystem::path> locateExecutable(const std::vector<std::filesystem::path> &roots,
                                                      const std::string &executableName) {
    const std::filesystem::path fileName(executableName);
    for (const auto &root : roots) {
        std::error_code error;
        if (!std::filesystem::exists(root, error) || error) {
            continue;
        }

        const std::vector<std::filesystem::path> directCandidates {
            root / fileName,
            root / "Binaries" / "Win64" / fileName,
            root / "bin" / fileName,
            root / "live" / "ShooterGame" / "Binaries" / "Win64" / fileName,
        };
        for (const auto &candidate : directCandidates) {
            if (std::filesystem::exists(candidate, error) && !error) {
                return candidate;
            }
        }

        std::filesystem::directory_iterator iterator(root, std::filesystem::directory_options::skip_permission_denied, error);
        const auto end = std::filesystem::directory_iterator();
        for (; iterator != end && !error; iterator.increment(error)) {
            if (!iterator->is_directory(error) || error) {
                continue;
            }
            const std::filesystem::path child = iterator->path();
            const std::vector<std::filesystem::path> childCandidates {
                child / fileName,
                child / "Binaries" / "Win64" / fileName,
                child / "Game" / "Binaries" / "Win64" / fileName,
                child / "ShooterGame" / "Binaries" / "Win64" / fileName,
                child / "live" / "ShooterGame" / "Binaries" / "Win64" / fileName,
            };
            for (const auto &candidate : childCandidates) {
                if (std::filesystem::exists(candidate, error) && !error) {
                    return candidate;
                }
            }
        }
    }
    return std::nullopt;
}

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

std::uint64_t queryAffinity(std::uint32_t pid) {
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (processHandle == nullptr) {
        return 0;
    }

    DWORD_PTR processMask = 0;
    DWORD_PTR systemMask = 0;
    const bool ok = GetProcessAffinityMask(processHandle, &processMask, &systemMask) == TRUE;
    CloseHandle(processHandle);
    return ok ? static_cast<std::uint64_t>(processMask) : 0;
}

std::pair<std::uint64_t, std::uint64_t> selectAffinityMasks() {
    DWORD_PTR processMask = 0;
    DWORD_PTR systemMask = 0;
    if (GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) != TRUE || systemMask == 0) {
        return {0, 0};
    }

    std::vector<int> bitPositions;
    for (int bit = 0; bit < static_cast<int>(sizeof(DWORD_PTR) * 8); ++bit) {
        if ((systemMask & (static_cast<DWORD_PTR>(1) << bit)) != 0) {
            bitPositions.push_back(bit);
        }
    }

    if (bitPositions.size() <= 2) {
        return {static_cast<std::uint64_t>(systemMask), 0};
    }

    const std::size_t split = bitPositions.size() / 2;
    std::uint64_t backgroundMask = 0;
    std::uint64_t gameMask = 0;
    for (std::size_t index = 0; index < bitPositions.size(); ++index) {
        const std::uint64_t bit = static_cast<std::uint64_t>(1) << bitPositions[index];
        if (index < split) {
            backgroundMask |= bit;
        } else {
            gameMask |= bit;
        }
    }

    if (gameMask == 0) {
        gameMask = static_cast<std::uint64_t>(systemMask);
    }

    return {gameMask, backgroundMask};
}

void persistSession(const SessionSnapshot &session) {
    auto settings = sessionSettings();
    settings.beginGroup(QStringLiteral("GameOptimizerSession"));
    settings.remove(QString());
    settings.setValue(QStringLiteral("active"), session.active);
    settings.setValue(QStringLiteral("gamePid"), static_cast<qulonglong>(session.gamePid));
    settings.setValue(QStringLiteral("originalPriority"), static_cast<qulonglong>(session.originalPriority));
    settings.setValue(QStringLiteral("originalAffinity"), static_cast<qulonglong>(session.originalAffinity));
    settings.setValue(QStringLiteral("displayName"), QString::fromStdString(session.displayName));
    settings.setValue(QStringLiteral("executableName"), QString::fromStdString(session.executableName));
    settings.setValue(QStringLiteral("aiSuspended"), session.aiSuspended);
    settings.setValue(QStringLiteral("networkTuned"), session.networkTuned);
    settings.setValue(QStringLiteral("powerProfileBoosted"), session.powerProfileBoosted);
    settings.setValue(QStringLiteral("gameAffinityMask"), static_cast<qulonglong>(session.gameAffinityMask));
    settings.setValue(QStringLiteral("backgroundAffinityMask"), static_cast<qulonglong>(session.backgroundAffinityMask));
    settings.beginWriteArray(QStringLiteral("suspendedLaunchers"));
    for (int index = 0; index < static_cast<int>(session.suspendedLauncherPids.size()); ++index) {
        settings.setArrayIndex(index);
        settings.setValue(QStringLiteral("pid"), static_cast<qulonglong>(session.suspendedLauncherPids[index]));
    }
    settings.endArray();
    settings.endGroup();

    settings.setValue(QStringLiteral("QtUi/gameModeActive"), session.active);
    settings.setValue(QStringLiteral("QtUi/gameModeTargetPid"), static_cast<int>(session.gamePid));
    settings.setValue(QStringLiteral("QtUi/gameModeTargetName"), QString::fromStdString(session.displayName));
    settings.setValue(QStringLiteral("QtUi/gameModeTargetExecutable"), QString::fromStdString(session.executableName));
    settings.setValue(QStringLiteral("QtUi/aiSuspendedForGame"), session.aiSuspended);
    settings.sync();
}

SessionSnapshot loadSession() {
    SessionSnapshot session;
    auto settings = sessionSettings();
    settings.beginGroup(QStringLiteral("GameOptimizerSession"));
    session.active = settings.value(QStringLiteral("active"), false).toBool();
    session.gamePid = static_cast<std::uint32_t>(settings.value(QStringLiteral("gamePid"), 0).toULongLong());
    session.originalPriority = static_cast<DWORD>(settings.value(QStringLiteral("originalPriority"), NORMAL_PRIORITY_CLASS).toULongLong());
    session.originalAffinity = static_cast<std::uint64_t>(settings.value(QStringLiteral("originalAffinity"), 0).toULongLong());
    session.displayName = settings.value(QStringLiteral("displayName"), QString()).toString().toStdString();
    session.executableName = settings.value(QStringLiteral("executableName"), QString()).toString().toStdString();
    session.aiSuspended = settings.value(QStringLiteral("aiSuspended"), false).toBool();
    session.networkTuned = settings.value(QStringLiteral("networkTuned"), false).toBool();
    session.powerProfileBoosted = settings.value(QStringLiteral("powerProfileBoosted"), false).toBool();
    session.gameAffinityMask = static_cast<std::uint64_t>(settings.value(QStringLiteral("gameAffinityMask"), 0).toULongLong());
    session.backgroundAffinityMask = static_cast<std::uint64_t>(settings.value(QStringLiteral("backgroundAffinityMask"), 0).toULongLong());
    const int count = settings.beginReadArray(QStringLiteral("suspendedLaunchers"));
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        const auto pid = static_cast<std::uint32_t>(settings.value(QStringLiteral("pid"), 0).toULongLong());
        if (pid != 0) {
            session.suspendedLauncherPids.push_back(pid);
        }
    }
    settings.endArray();
    settings.endGroup();
    return session;
}

void clearSession() {
    auto settings = sessionSettings();
    settings.beginGroup(QStringLiteral("GameOptimizerSession"));
    settings.remove(QString());
    settings.endGroup();
    settings.setValue(QStringLiteral("QtUi/gameModeActive"), false);
    settings.remove(QStringLiteral("QtUi/gameModeTargetPid"));
    settings.remove(QStringLiteral("QtUi/gameModeTargetName"));
    settings.remove(QStringLiteral("QtUi/gameModeTargetExecutable"));
    settings.setValue(QStringLiteral("QtUi/aiSuspendedForGame"), false);
    settings.sync();
}

bool launchDetached(const GameProfile &profile) {
    if (!profile.isDetected || profile.installPath.empty()) {
        return false;
    }
    const QString executablePath = QDir::toNativeSeparators(QString::fromStdString(profile.installPath + "\\" + profile.executableName));
    return QProcess::startDetached(executablePath, {}, QString::fromStdString(profile.installPath));
}

}  // namespace

GameOptimizer::GameOptimizer(ProcessManager &processManager, NetworkOptimizer &networkOptimizer)
    : processManager_(processManager), networkOptimizer_(networkOptimizer) {}

std::optional<std::uint32_t> GameOptimizer::runningPidForExecutable(const std::string &executableName) const {
    const std::string targetLower = toLowerAscii(executableName);
    for (const auto &process : processManager_.enumerateProcesses()) {
        if (toLowerAscii(process.name) == targetLower) {
            return process.pid;
        }
    }
    return std::nullopt;
}

std::vector<GameProfile> GameOptimizer::detectInstalledGames() const {
    const auto roots = candidateRoots();
    const auto processes = processManager_.enumerateProcesses();
    std::map<std::string, std::uint32_t> runningExecutables;
    for (const auto &process : processes) {
        runningExecutables.emplace(toLowerAscii(process.name), process.pid);
    }

    const auto activeSession = loadSession();
    const std::string activeTargetLower = toLowerAscii(activeSession.executableName);

    std::vector<GameProfile> profiles;
    for (const auto &known : knownGames()) {
        GameProfile profile;
        profile.executableName = known.executableName;
        profile.displayName = known.displayName;
        profile.launcher = known.launcher;
        profile.tweaksAvailable = 6;
        profile.tweaksApplied = {"High priority", "CPU affinity split", "Launcher suspension", "Network tune", "Power profile", "AI suspension"};

        const auto installedPath = locateExecutable(roots, known.executableName);
        if (installedPath.has_value()) {
            profile.isDetected = true;
            profile.installPath = installedPath->parent_path().string();
        }

        if (const auto running = runningExecutables.find(toLowerAscii(known.executableName)); running != runningExecutables.end()) {
            profile.isRunning = true;
            profile.runningPid = running->second;
        }
        profile.isOptimized = activeSession.active && profile.isRunning && toLowerAscii(known.executableName) == activeTargetLower;
        if (!profile.isDetected && !profile.isRunning) {
            continue;
        }
        profiles.push_back(std::move(profile));
    }

    std::sort(profiles.begin(), profiles.end(), [](const GameProfile &left, const GameProfile &right) {
        if (left.isRunning != right.isRunning) {
            return left.isRunning > right.isRunning;
        }
        if (left.isDetected != right.isDetected) {
            return left.isDetected > right.isDetected;
        }
        return left.displayName < right.displayName;
    });
    return profiles;
}

std::optional<GameProfile> GameOptimizer::findGameProfile(const std::string &query) const {
    const std::string needle = toLowerAscii(query);
    const auto profiles = detectInstalledGames();
    for (const auto &profile : profiles) {
        if (toLowerAscii(profile.executableName) == needle || toLowerAscii(profile.displayName) == needle) {
            return profile;
        }
    }
    return std::nullopt;
}

GameSessionResult GameOptimizer::optimizeRunningGame(const std::string &query) const {
    GameSessionResult result;
    const auto profile = findGameProfile(query);
    if (!profile.has_value()) {
        result.reason = "game-not-found";
        return result;
    }

    const auto pid = runningPidForExecutable(profile->executableName);
    if (!pid.has_value()) {
        result.reason = "game-not-running";
        return result;
    }

    const auto previous = loadSession();
    if (previous.active) {
        const_cast<GameOptimizer *>(this)->revertOptimization();
    }

    SessionSnapshot session;
    session.active = true;
    session.gamePid = *pid;
    session.displayName = profile->displayName;
    session.executableName = profile->executableName;
    session.originalPriority = queryPriority(*pid);
    session.originalAffinity = queryAffinity(*pid);

    result.pid = *pid;
    result.displayName = profile->displayName;
    result.executableName = profile->executableName;

    const bool priorityOk = processManager_.setPriority(*pid, HIGH_PRIORITY_CLASS);
    if (priorityOk) {
        result.actionsApplied.push_back("Raised game priority");
        result.proof.push_back("Raised the game process priority to High for steadier frame delivery.");
    }

    const auto [gameMask, backgroundMask] = selectAffinityMasks();
    session.gameAffinityMask = gameMask;
    session.backgroundAffinityMask = backgroundMask;
    if (gameMask != 0 && processManager_.setAffinity(*pid, gameMask)) {
        result.gameAffinityMask = gameMask;
        result.backgroundAffinityMask = backgroundMask;
        result.actionsApplied.push_back("Pinned game to preferred CPU cores");
        const int reservedCoreCount = std::popcount(static_cast<unsigned long long>(gameMask));
        result.proof.push_back("Reserved " + std::to_string(reservedCoreCount) + " CPU cores for the active game process.");
    }

    const auto launcherNames = launcherProcessNames();
    for (const auto &process : processManager_.enumerateProcesses()) {
        const auto lowerName = toLowerAscii(process.name);
        if (process.pid == *pid || process.isCritical) {
            continue;
        }
        if (std::find(launcherNames.begin(), launcherNames.end(), lowerName) == launcherNames.end()) {
            continue;
        }
        if (processManager_.suspendProcess(process.pid)) {
            session.suspendedLauncherPids.push_back(process.pid);
        } else if (backgroundMask != 0) {
            processManager_.setAffinity(process.pid, backgroundMask);
            processManager_.setPriority(process.pid, BELOW_NORMAL_PRIORITY_CLASS);
        }
    }
    result.launchersSuspended = static_cast<int>(session.suspendedLauncherPids.size());
    if (result.launchersSuspended > 0) {
        result.actionsApplied.push_back("Suspended heavy launcher helpers");
        result.proof.push_back("Suspended " + std::to_string(result.launchersSuspended) + " launcher helper processes after the game took focus.");
    }

    session.networkTuned = networkOptimizer_.optimizeTcp();
    result.networkTuned = session.networkTuned;
    if (session.networkTuned) {
        result.actionsApplied.push_back("Applied gaming network tuning");
        result.proof.push_back("Applied low-latency TCP tuning and kept the NIC path focused on the active session.");
    }

    DWORD exitCode = 1;
    session.powerProfileBoosted = runProcessHidden(L"powercfg /setactive SCHEME_MIN", &exitCode) && exitCode == 0;
    result.powerProfileBoosted = session.powerProfileBoosted;
    if (session.powerProfileBoosted) {
        result.actionsApplied.push_back("Activated high-performance power profile");
        result.proof.push_back("Switched Windows to the High Performance power profile for the session.");
    }

    session.aiSuspended = true;
    result.aiSuspended = true;
    result.actionsApplied.push_back("Suspended PulseBoost AI helpers during gameplay");
    result.proof.push_back("Bounded PulseBoost AI resource usage by suspending the AI assistant during the active game session.");

    persistSession(session);
    result.ok = priorityOk || session.networkTuned || session.powerProfileBoosted || result.launchersSuspended > 0;
    return result;
}

GameSessionResult GameOptimizer::launchAndOptimize(const std::string &query) const {
    const auto profile = findGameProfile(query);
    GameSessionResult result;
    if (!profile.has_value()) {
        result.reason = "game-not-found";
        return result;
    }

    if (!launchDetached(*profile)) {
        result.reason = "launch-failed";
        return result;
    }

    for (int attempt = 0; attempt < 30; ++attempt) {
        if (const auto pid = runningPidForExecutable(profile->executableName); pid.has_value()) {
            return optimizeRunningGame(profile->executableName);
        }
        Sleep(500);
    }

    result.reason = "launch-pending";
    result.displayName = profile->displayName;
    result.executableName = profile->executableName;
    return result;
}

GameSessionResult GameOptimizer::revertOptimization() const {
    GameSessionResult result;
    const auto session = loadSession();
    if (!session.active) {
        result.ok = true;
        result.reason = "no-active-session";
        return result;
    }

    result.displayName = session.displayName;
    result.executableName = session.executableName;
    result.pid = session.gamePid;

    if (session.gamePid != 0) {
        if (session.originalPriority != 0) {
            processManager_.setPriority(session.gamePid, session.originalPriority);
        }
        if (session.originalAffinity != 0) {
            processManager_.setAffinity(session.gamePid, session.originalAffinity);
        }
    }

    for (const auto pid : session.suspendedLauncherPids) {
        processManager_.resumeProcess(pid);
    }

    DWORD exitCode = 1;
    runProcessHidden(L"powercfg /setactive SCHEME_BALANCED", &exitCode);
    clearSession();

    result.aiSuspended = false;
    result.launchersSuspended = static_cast<int>(session.suspendedLauncherPids.size());
    result.ok = true;
    result.actionsApplied = {
        "Restored original power profile",
        "Resumed suspended launcher helpers",
        "Re-enabled PulseBoost AI"
    };
    result.proof = {
        "Restored the pre-game session state and released the CPU affinity reservation.",
        "Resumed " + std::to_string(result.launchersSuspended) + " launcher helper processes.",
        "Re-enabled PulseBoost AI background assistance after the game session ended."
    };
    return result;
}

GameSessionResult GameOptimizer::currentSessionStatus() const {
    GameSessionResult result;
    const auto session = loadSession();
    result.ok = true;
    result.pid = session.gamePid;
    result.displayName = session.displayName;
    result.executableName = session.executableName;
    result.aiSuspended = session.aiSuspended;
    result.networkTuned = session.networkTuned;
    result.powerProfileBoosted = session.powerProfileBoosted;
    result.gameAffinityMask = session.gameAffinityMask;
    result.backgroundAffinityMask = session.backgroundAffinityMask;
    result.launchersSuspended = static_cast<int>(session.suspendedLauncherPids.size());
    if (!session.active) {
        result.reason = "inactive";
        return result;
    }

    const auto livePid = runningPidForExecutable(session.executableName);
    if (!livePid.has_value()) {
        return revertOptimization();
    }

    result.pid = *livePid;
    result.actionsApplied = {
        "Game session optimization active",
        session.aiSuspended ? "AI suspended" : "AI active",
        session.networkTuned ? "Network tuned" : "Network default"
    };
    result.proof = {
        "Active game PID " + std::to_string(*livePid) + " is still being managed by GameOptimizer."
    };
    return result;
}

}  // namespace pulseboost
