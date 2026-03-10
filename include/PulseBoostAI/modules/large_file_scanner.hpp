#pragma once

#include <filesystem>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class LargeFileScanner {
public:
    std::vector<FileEntry> scan(const std::filesystem::path &root,
                                std::uintmax_t minimumSizeBytes = 512ULL * 1024ULL * 1024ULL,
                                std::size_t limit = 50) const;
};

}  // namespace pulseboost
