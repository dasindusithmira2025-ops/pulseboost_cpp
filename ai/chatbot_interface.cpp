#include "PulseBoostAI/ai/chatbot_interface.hpp"

#include <algorithm>
#include <cctype>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

std::string toLower(std::string value) {
    for (char &character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

}  // namespace

ChatbotInterface::ChatbotInterface(SystemScanner &scanner,
                                   AiDiagnosticsEngine &diagnosticsEngine,
                                   JunkCleaner &junkCleaner,
                                   StartupOptimizer &startupOptimizer,
                                   GameMode &gameMode,
                                   DeveloperMode &developerMode,
                                   OptimizationHistory &optimizationHistory)
    : scanner_(scanner),
      diagnosticsEngine_(diagnosticsEngine),
      junkCleaner_(junkCleaner),
      startupOptimizer_(startupOptimizer),
      gameMode_(gameMode),
      developerMode_(developerMode),
      optimizationHistory_(optimizationHistory) {}

ChatResponse ChatbotInterface::handleMessage(const std::string &message) {
    const std::string lower = toLower(message);
    const auto snapshot = scanner_.scan();

    if (lower.find("clean") != std::string::npos) {
        const auto result = junkCleaner_.cleanSafeTargets();
        optimizationHistory_.record(ActionRecord {.timestampUtc = currentTimestampUtc(),
                                                  .action = "safe-clean",
                                                  .details = "Recovered " + std::to_string(result.bytesRecovered) + " bytes",
                                                  .success = true});
        return ChatResponse {.actionExecuted = true,
                             .message = "Cleanup completed. Removed " + std::to_string(result.filesRemoved) +
                                            " files and recovered " + formatBytes(result.bytesRecovered) + "."};
    }

    if (lower.find("optimize system") != std::string::npos || lower.find("optimize my system") != std::string::npos) {
        const auto result = junkCleaner_.cleanSafeTargets();
        optimizationHistory_.record(ActionRecord {.timestampUtc = currentTimestampUtc(),
                                                  .action = "optimize-system",
                                                  .details = "Cleanup recovered " + std::to_string(result.bytesRecovered) + " bytes",
                                                  .success = true});
        return ChatResponse {.actionExecuted = true,
                             .message = "System optimization pass finished. Junk cleanup recovered " +
                                            formatBytes(result.bytesRecovered) + ".\n\n" +
                                            diagnosticsEngine_.summarizeSystem(snapshot)};
    }

    if (lower.find("analyze performance") != std::string::npos || lower.find("system scan") != std::string::npos) {
        optimizationHistory_.record(ActionRecord {.timestampUtc = currentTimestampUtc(),
                                                  .action = "analyze-performance",
                                                  .details = "Generated diagnostic report",
                                                  .success = true});
        return ChatResponse {.actionExecuted = false, .message = diagnosticsEngine_.answerQuestion(message, snapshot)};
    }

    if (lower.find("startup") != std::string::npos) {
        const auto items = startupOptimizer_.scanStartupItems();
        return ChatResponse {.actionExecuted = false,
                             .message = "Startup analysis found " + std::to_string(items.size()) +
                                            " entries. Review the highest-impact items in the dashboard before disabling them."};
    }

    if (lower.find("game mode") != std::string::npos || lower.find("boost gaming") != std::string::npos) {
        for (const auto &process : snapshot.heavyProcesses) {
            if (!process.isCritical && process.memoryMb > 512.0) {
                const bool enabled = gameMode_.enableForProcess(process.pid);
                optimizationHistory_.record(ActionRecord {.timestampUtc = currentTimestampUtc(),
                                                          .action = "boost-gaming",
                                                          .details = "Target PID " + std::to_string(process.pid),
                                                          .success = enabled});
                return ChatResponse {.actionExecuted = enabled,
                                     .message = enabled ? "Game boost enabled for " + process.name + "."
                                                        : "Game boost could not be enabled for " + process.name + "."};
            }
        }

        return ChatResponse {.actionExecuted = false,
                             .message = "No obvious game process is active right now. Launch a game first, then run boost gaming again."};
    }

    if (lower.find("developer") != std::string::npos || lower.find("docker") != std::string::npos ||
        lower.find("unreal") != std::string::npos || lower.find("android studio") != std::string::npos) {
        const auto actions = developerMode_.optimizeFor(message);
        std::string response = "Developer mode recommendations:\n";
        for (const auto &action : actions) {
            response += "- " + action + '\n';
        }
        return ChatResponse {.actionExecuted = false, .message = response};
    }

    return ChatResponse {.actionExecuted = false, .message = diagnosticsEngine_.answerQuestion(message, snapshot)};
}

}  // namespace pulseboost
