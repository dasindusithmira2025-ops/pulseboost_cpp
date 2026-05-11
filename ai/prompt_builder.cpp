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

std::string PromptBuilder::buildDecisionPrompt(const SystemSnapshot &snapshot,
                                               const std::vector<SystemSnapshot> &history,
                                               const std::string &userMessage) const {
    std::ostringstream ss;
    ss << "You are PulseBoost AI, an expert Windows performance and diagnostics agent.\n";
    ss << "Decide what to do from telemetry and user intent, then respond ONLY as JSON.\n";
    ss << "Do not output markdown, explanations outside JSON, or code fences.\n\n";

    ss << "Telemetry:\n";
    ss << "cpu_percent=" << static_cast<int>(snapshot.cpuUsagePercent) << '\n';
    ss << "ram_percent=" << static_cast<int>(snapshot.ramUsagePercent) << '\n';
    ss << "disk_percent=" << static_cast<int>(snapshot.diskUsagePercent) << '\n';
    ss << "gpu_percent=" << static_cast<int>(snapshot.gpuUsagePercent) << '\n';
    ss << "network_mbps=" << static_cast<int>(snapshot.networkMbps) << '\n';
    ss << "startup_programs=" << snapshot.startupPrograms << '\n';
    ss << "running_services=" << snapshot.runningServices << '\n';

    if (!snapshot.issues.empty()) {
        ss << "issues=";
        for (std::size_t index = 0; index < snapshot.issues.size(); ++index) {
            if (index > 0) {
                ss << " | ";
            }
            ss << snapshot.issues[index];
        }
        ss << '\n';
    }

    if (!snapshot.heavyProcesses.empty()) {
        ss << "top_processes=";
        const auto count = std::min<std::size_t>(5, snapshot.heavyProcesses.size());
        for (std::size_t index = 0; index < count; ++index) {
            const auto &process = snapshot.heavyProcesses[index];
            if (index > 0) {
                ss << " | ";
            }
            ss << process.name << " cpu=" << static_cast<int>(process.cpuPercent)
               << " ram_mb=" << static_cast<int>(process.memoryMb);
        }
        ss << '\n';
    }

    if (!history.empty()) {
        ss << "recent_health_scores=";
        for (std::size_t index = 0; index < history.size(); ++index) {
            if (index > 0) {
                ss << ',';
            }
            ss << history[index].healthScore;
        }
        ss << '\n';
    }

    ss << "\nAllowed action names:\n";
    ss << "- create_restore_point\n";
    ss << "- clean_junk\n";
    ss << "- analyze_startup\n";
    ss << "- enable_game_mode\n";
    ss << "- optimize_developer_mode\n";
    ss << "- none\n\n";

    ss << "User message: " << userMessage << "\n\n";
    ss << "Return valid JSON with this exact shape:\n";
    ss << "{\n";
    ss << "  \"reply\": \"string\",\n";
    ss << "  \"plan\": [\n";
    ss << "    {\"title\": \"string\", \"details\": \"string\", \"execute\": true}\n";
    ss << "  ],\n";
    ss << "  \"actions\": [\"action_name\"]\n";
    ss << "}\n\n";
    ss << "Rules:\n";
    ss << "1) Keep reply concise, specific, and technical.\n";
    ss << "2) Include 2-5 plan steps.\n";
    ss << "3) Only request actions from allowed action names.\n";
    ss << "4) Use \"none\" if no safe action should run now.\n";
    return ss.str();
}

}  // namespace pulseboost
