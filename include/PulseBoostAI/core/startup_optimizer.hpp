#pragma once

#include <filesystem>
#include <vector>

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/registry_optimizer.hpp"

namespace pulseboost {

class StartupOptimizer {
public:
    explicit StartupOptimizer(RegistryOptimizer &registryOptimizer);

    std::vector<StartupItem> scanStartupItems() const;
    bool disableStartupItem(const StartupItem &item, const std::filesystem::path &backupDirectory) const;
    bool delayStartupItem(const StartupItem &item, int delaySeconds) const;

private:
    RegistryOptimizer &registryOptimizer_;
};

}  // namespace pulseboost
