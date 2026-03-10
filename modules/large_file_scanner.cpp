#include "PulseBoostAI/modules/large_file_scanner.hpp"

#include <algorithm>

namespace pulseboost {

std::vector<FileEntry> LargeFileScanner::scan(const std::filesystem::path &root,
                                              std::uintmax_t minimumSizeBytes,
                                              std::size_t limit) const {
    std::vector<FileEntry> files;
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
            files.push_back(FileEntry {.path = entry.path().string(), .bytes = static_cast<std::uint64_t>(size)});
        } else {
            error.clear();
        }
    }

    std::sort(files.begin(), files.end(), [](const FileEntry &left, const FileEntry &right) {
        return left.bytes > right.bytes;
    });
    if (files.size() > limit) {
        files.resize(limit);
    }
    return files;
}

}  // namespace pulseboost
