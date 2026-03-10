#include "PulseBoostAI/ai/ai_diagnostics_engine.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

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

std::string AiDiagnosticsEngine::summarizeSystem(const SystemSnapshot &snapshot) const {
    std::ostringstream stream;
    stream << "Detected issues:\n";
    if (snapshot.issues.empty()) {
        stream << "- No critical issues detected.\n";
    } else {
        for (const auto &issue : snapshot.issues) {
            stream << "- " << issue << '\n';
        }
    }

    stream << "\nRecommended actions:\n";
    if (snapshot.ramUsagePercent > 80.0) {
        stream << "- Close or tune high-memory processes.\n";
    }
    if (snapshot.startupPrograms > 10) {
        stream << "- Disable or delay non-essential startup apps.\n";
    }
    if (snapshot.diskUsagePercent > 85.0) {
        stream << "- Run junk cleanup and move large files off the system drive.\n";
    }
    if (snapshot.cpuUsagePercent > 85.0) {
        stream << "- Use game/developer mode selectively and inspect CPU-heavy processes.\n";
    }
    if (snapshot.issues.empty() && snapshot.healthScore >= 80) {
        stream << "- Maintain current configuration and review telemetry trends.\n";
    }

    return stream.str();
}

std::string AiDiagnosticsEngine::answerQuestion(const std::string &question, const SystemSnapshot &snapshot) const {
    const std::string lower = toLower(question);
    std::ostringstream stream;

    stream << "PulseBoost AI diagnostic report\n\n";

    if (lower.find("slow") != std::string::npos || lower.find("performance") != std::string::npos) {
        stream << "Your system is slow because the current health score is " << snapshot.healthScore
               << "/100. CPU is at " << static_cast<int>(snapshot.cpuUsagePercent) << "% and RAM is at "
               << static_cast<int>(snapshot.ramUsagePercent) << "%.\n\n";
    } else if (lower.find("game") != std::string::npos || lower.find("lag") != std::string::npos) {
        stream << "Game lag analysis: CPU " << static_cast<int>(snapshot.cpuUsagePercent) << "%, RAM "
               << static_cast<int>(snapshot.ramUsagePercent) << "%, GPU " << static_cast<int>(snapshot.gpuUsagePercent)
               << "%, disk read " << static_cast<int>(snapshot.diskReadMbps) << " Mbps.\n\n";
    } else if (lower.find("disk") != std::string::npos || lower.find("storage") != std::string::npos ||
               lower.find("full") != std::string::npos) {
        stream << "Storage analysis: system drive is " << static_cast<int>(snapshot.diskUsagePercent)
               << "% full. Startup count is " << snapshot.startupPrograms << ".\n\n";
    } else {
        stream << "System overview: " << snapshot.summary << "\n\n";
    }

    stream << summarizeSystem(snapshot);

    if (!snapshot.heavyProcesses.empty()) {
        stream << "\nTop heavy processes:\n";
        for (std::size_t index = 0; index < std::min<std::size_t>(3, snapshot.heavyProcesses.size()); ++index) {
            const auto &process = snapshot.heavyProcesses[index];
            stream << "- " << process.name << ": " << static_cast<int>(process.memoryMb) << " MB, "
                   << static_cast<int>(process.cpuPercent) << "% CPU\n";
        }
    }

    return stream.str();
}

}  // namespace pulseboost
