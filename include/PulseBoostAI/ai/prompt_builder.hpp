#pragma once

#include <string>
#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

/// Builds a rich context prompt combining system telemetry and the user's message.
class PromptBuilder {
public:
    /// Returns a text prompt ready to send to an LLM.
    std::string build(const SystemSnapshot &snapshot, const std::string &userMessage) const;
    std::string buildAgentPrompt(const SystemSnapshot &snapshot,
                                 const std::vector<SystemSnapshot> &history,
                                 const std::vector<OptimizationPlanStep> &plan,
                                 const std::string &userMessage) const;
};

}  // namespace pulseboost
