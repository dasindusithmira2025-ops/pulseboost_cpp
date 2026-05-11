#pragma once

#include <string>
#include <vector>

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/modules/tweak_engine.hpp"

namespace pulseboost {

struct AdvisorItem {
    std::string id;
    std::string category;
    std::string title;
    std::string description;
    std::string impact;
    std::string status;
    bool actionable = false;
    std::string actionLabel;
    std::string actionId;
};

class SystemAdvisor {
public:
    std::vector<AdvisorItem> analyze(const SystemSnapshot &snapshot,
                                     const std::vector<TweakDefinition> &tweaks) const;
};

}  // namespace pulseboost
