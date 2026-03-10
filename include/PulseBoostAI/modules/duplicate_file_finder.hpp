#pragma once

#include <filesystem>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class DuplicateFileFinder {
public:
    std::vector<DuplicateGroup> scan(const std::filesystem::path &root, std::uintmax_t minimumSizeBytes = 5 * 1024 * 1024) const;

private:
    static std::string hashFile(const std::filesystem::path &path);
};

}  // namespace pulseboost
