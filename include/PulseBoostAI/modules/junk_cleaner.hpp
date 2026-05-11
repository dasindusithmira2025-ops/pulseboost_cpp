#pragma once

#include <filesystem>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

enum class CleanupMode {
    Quarantine,
    Recycle,
    PermanentDelete,
};

struct CleanupOptions {
    bool dryRun = false;
    CleanupMode mode = CleanupMode::Quarantine;
    std::filesystem::path quarantineRoot;
    std::vector<std::filesystem::path> overrideTargets;
};

class JunkCleaner {
public:
    CleanupResult cleanSafeTargets(const CleanupOptions &options = {}) const;
    std::uint64_t estimateRecoverableBytes() const;
    std::vector<std::filesystem::path> candidateTargets(const CleanupOptions &options = {}) const;

    static std::filesystem::path defaultQuarantineRoot();
};

}  // namespace pulseboost
