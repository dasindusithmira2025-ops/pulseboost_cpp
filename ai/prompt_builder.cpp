#include "PulseBoostAI/ai/prompt_builder.hpp"

#include <algorithm>
#include <sstream>

namespace pulseboost {

std::string PromptBuilder::build(const SystemSnapshot &snapshot, const std::string &userMessage) const {
    return buildAgentPrompt(snapshot, {}, {}, userMessage);
}

std::string PromptBuilder::buildAgentPrompt(const SystemSnapshot &snapshot,
                                            const std::vector<SystemSnapshot> &history,
                                            const std::vector<OptimizationPlanStep> &plan,
                                            const std::string &userMessage) const {
    std::ostringstream ss;
    ss << "You are PulseBoost AI, a professional Windows performance and optimization assistant.\n"
       << "Diagnose issues from the telemetry, explain the likely causes, and provide clear actions.\n\n";

    ss << "User system stats:\n";
    ss << "CPU: " << static_cast<int>(snapshot.cpuUsagePercent) << "%\n";
    ss << "RAM: " << static_cast<int>(snapshot.ramUsagePercent) << "%\n";
    ss << "Disk: " << static_cast<int>(snapshot.diskUsagePercent) << "%\n";
    ss << "GPU: " << static_cast<int>(snapshot.gpuUsagePercent) << "%\n";
    ss << "Network: " << static_cast<int>(snapshot.networkMbps) << " Mbps\n";
    ss << "Startup apps: " << snapshot.startupPrograms << "\n";
    ss << "Running services: " << snapshot.runningServices << "\n";

    if (!snapshot.issues.empty()) {
        ss << "Detected issues:\n";
        for (const auto &issue : snapshot.issues) {
            ss << "- " << issue << '\n';
        }
    }

    if (!snapshot.heavyProcesses.empty()) {
        ss << "Heavy processes:\n";
        const auto count = std::min<std::size_t>(5, snapshot.heavyProcesses.size());
        for (std::size_t index = 0; index < count; ++index) {
            const auto &process = snapshot.heavyProcesses[index];
            ss << "- " << process.name << " | CPU " << static_cast<int>(process.cpuPercent)
               << "% | RAM " << static_cast<int>(process.memoryMb) << " MB\n";
        }
    }

    if (!history.empty()) {
        ss << "Recent telemetry samples: " << history.size() << "\n";
        ss << "Recent health scores:";
        for (const auto &sample : history) {
            ss << ' ' << sample.healthScore;
        }
        ss << '\n';
    }

    if (!plan.empty()) {
        ss << "\nPlanned actions:\n";
        int stepNumber = 1;
        for (const auto &step : plan) {
            ss << stepNumber++ << ". " << step.title << " - " << step.details;
            if (step.execute) {
                ss << " [execute]";
            }
            ss << '\n';
        }
    }

    ss << "\nUser question:\n" << userMessage << "\n\n";
    ss << "Instructions:\n";
    ss << "1. Explain the likely root cause using the telemetry.\n";
    ss << "2. If planned actions are present, include a PLAN section with the steps.\n";
    ss << "3. Keep the answer concise and actionable.\n";
    ss << "4. Mention the expected impact and one safety note when relevant.\n";
    return ss.str();
}

}  // namespace pulseboost
