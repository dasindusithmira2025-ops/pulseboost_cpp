#pragma once

#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class SystemPrediction {
public:
    std::vector<std::string> predict(const std::vector<SystemSnapshot> &history) const;
};

}  // namespace pulseboost
