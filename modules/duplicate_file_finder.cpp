#include "PulseBoostAI/modules/duplicate_file_finder.hpp"

#include <fstream>
#include <unordered_map>

namespace pulseboost {

namespace {

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

}  // namespace

std::string DuplicateFileFinder::hashFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    std::uint64_t hash = kFnvOffset;
    char buffer[8192];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0) {
        for (std::streamsize index = 0; index < input.gcount(); ++index) {
            hash ^= static_cast<unsigned char>(buffer[index]);
            hash *= kFnvPrime;
        }
    }
    return std::to_string(hash);
}

std::vector<DuplicateGroup> DuplicateFileFinder::scan(const std::filesystem::path &root, std::uintmax_t minimumSizeBytes) const {
    std::unordered_map<std::uintmax_t, std::vector<std::filesystem::path>> sizeBuckets;
    std::error_code error;
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error) {
            error.clear();
            continue;
        }
        if (!entry.is_regular_file(error)) {
            continue;
        }

        const auto size = entry.file_size(error);
        if (!error && size >= minimumSizeBytes) {
            sizeBuckets[size].push_back(entry.path());
        } else {
            error.clear();
        }
    }

    std::vector<DuplicateGroup> groups;
    for (const auto &[size, files] : sizeBuckets) {
        if (files.size() < 2) {
            continue;
        }

        std::unordered_map<std::string, std::vector<std::filesystem::path>> hashBuckets;
        for (const auto &file : files) {
            const auto hash = hashFile(file);
            if (!hash.empty()) {
                hashBuckets[hash].push_back(file);
            }
        }

        for (const auto &[hash, duplicates] : hashBuckets) {
            if (duplicates.size() < 2) {
                continue;
            }

            DuplicateGroup group;
            group.reclaimableBytes = static_cast<std::uint64_t>((duplicates.size() - 1) * size);
            for (const auto &duplicate : duplicates) {
                group.files.push_back(FileEntry {.path = duplicate.string(), .bytes = static_cast<std::uint64_t>(size)});
            }
            groups.push_back(std::move(group));
        }
    }

    return groups;
}

}  // namespace pulseboost
