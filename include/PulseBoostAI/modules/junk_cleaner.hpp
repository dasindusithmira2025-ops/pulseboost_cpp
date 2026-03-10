#pragma once

#include <filesystem>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class JunkCleaner {
public:
    CleanupResult cleanSafeTargets() const;
    std::vector<std::filesystem::path> candidateTargets() const;
};

}  // namespace pulseboost
