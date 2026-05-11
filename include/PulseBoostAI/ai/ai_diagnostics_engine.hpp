#pragma once

#include <string>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

struct LocalAgentDecision {
    std::string reply;
    std::vector<OptimizationPlanStep> plan;
    std::vector<std::string> actions;
    std::vector<std::string> predictions;
    int confidence = 0;
};

class AiDiagnosticsEngine {
public:
    LocalAgentDecision reason(const std::string &question,
                              const SystemSnapshot &snapshot,
                              const std::vector<SystemSnapshot> &history) const;

    std::string answerQuestion(const std::string &question, const SystemSnapshot &snapshot) const;
    std::string summarizeSystem(const SystemSnapshot &snapshot) const;
};

}  // namespace pulseboost
