#include "PulseBoostAI/modules/developer_mode.hpp"

#include <algorithm>
#include <cctype>

namespace pulseboost {

namespace {

std::string toLower(std::string value) {
    for (char &character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

}  // namespace

DeveloperMode::DeveloperMode(ProcessManager &processManager, ServiceManager &serviceManager)
    : processManager_(processManager), serviceManager_(serviceManager) {}

std::vector<std::string> DeveloperMode::optimizeFor(const std::string &profile) const {
    std::vector<std::string> actions;
    const std::string lower = toLower(profile);

    if (lower.find("unreal") != std::string::npos) {
        actions.push_back("Prioritize UnrealEditor.exe and ShaderCompileWorker.exe when present.");
        actions.push_back("Keep at least 20% free space on the system SSD for DerivedDataCache and builds.");
    }
    if (lower.find("docker") != std::string::npos) {
        actions.push_back("Review Docker Desktop startup impact and stop idle containers before heavy builds.");
    }
    if (lower.find("android") != std::string::npos) {
        actions.push_back("Prioritize studio64.exe and Gradle workers during active compilation.");
    }
    if (lower.find("node") != std::string::npos || lower.find("build") != std::string::npos) {
        actions.push_back("Boost node.exe priority only during foreground build bursts to avoid desktop lag.");
    }
    if (actions.empty()) {
        actions.push_back("Developer mode is profile-driven. Mention Unreal, Docker, Android Studio, or Node workloads.");
    }

    for (const auto &process : processManager_.enumerateProcesses()) {
        const std::string name = toLower(process.name);
        if (name == "unrealeditor.exe" || name == "shadercompileworker.exe" || name == "studio64.exe" || name == "node.exe") {
            processManager_.setPriority(process.pid, ABOVE_NORMAL_PRIORITY_CLASS);
        }
    }

    return actions;
}

}  // namespace pulseboost
