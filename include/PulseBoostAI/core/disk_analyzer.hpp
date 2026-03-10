#pragma once

#include <chrono>
#include <filesystem>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class DiskAnalyzer {
public:
    DiskSummary analyzeSystemDrive(std::size_t maxLargeFiles = 20) const;

private:
    std::vector<StorageCategory> buildStorageMap(const std::filesystem::path &root) const;
    std::vector<FileEntry> findLargestFiles(const std::filesystem::path &root, std::size_t limit) const;

    mutable std::chrono::steady_clock::time_point lastDeepScan_ {};
    mutable std::vector<StorageCategory> cachedCategories_;
    mutable std::vector<FileEntry> cachedLargeFiles_;
};

}  // namespace pulseboost
