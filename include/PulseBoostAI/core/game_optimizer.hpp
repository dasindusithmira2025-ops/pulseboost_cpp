#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pulseboost {

class ProcessManager;
class NetworkOptimizer;

struct GameProfile {
    std::string executableName;
    std::string displayName;
    std::string installPath;
    std::string launcher;
    bool isDetected = false;
    bool isOptimized = false;
    bool isRunning = false;
    std::uint32_t runningPid = 0;
    std::vector<std::string> tweaksApplied;
    int tweaksAvailable = 0;
};

struct GameSessionResult {
    bool ok = false;
    std::uint32_t pid = 0;
    std::string displayName;
    std::string executableName;
    bool aiSuspended = false;
    bool networkTuned = false;
    bool powerProfileBoosted = false;
    std::uint64_t gameAffinityMask = 0;
    std::uint64_t backgroundAffinityMask = 0;
    int launchersSuspended = 0;
    std::vector<std::string> actionsApplied;
    std::vector<std::string> proof;
    std::string reason;
};

class GameOptimizer {
public:
    GameOptimizer(ProcessManager &processManager, NetworkOptimizer &networkOptimizer);

    std::vector<GameProfile> detectInstalledGames() const;
    std::optional<GameProfile> findGameProfile(const std::string &query) const;
    std::optional<std::uint32_t> runningPidForExecutable(const std::string &executableName) const;

    GameSessionResult optimizeRunningGame(const std::string &query) const;
    GameSessionResult launchAndOptimize(const std::string &query) const;
    GameSessionResult revertOptimization() const;
    GameSessionResult currentSessionStatus() const;

private:
    ProcessManager &processManager_;
    NetworkOptimizer &networkOptimizer_;
};

}  // namespace pulseboost
