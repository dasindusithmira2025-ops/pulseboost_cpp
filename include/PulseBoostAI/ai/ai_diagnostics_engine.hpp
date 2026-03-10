#pragma once

#include <string>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class AiDiagnosticsEngine {
public:
    std::string answerQuestion(const std::string &question, const SystemSnapshot &snapshot) const;
    std::string summarizeSystem(const SystemSnapshot &snapshot) const;
};

}  // namespace pulseboost
