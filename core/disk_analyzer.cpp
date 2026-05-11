#include "PulseBoostAI/core/disk_analyzer.hpp"

#include <Windows.h>

#include <algorithm>
#include <deque>
#include <string>
#include <system_error>
#include <vector>

namespace pulseboost {

namespace {

std::filesystem::path systemDriveRoot() {
    wchar_t buffer[MAX_PATH] {};
    const DWORD written = GetEnvironmentVariableW(L"SystemDrive", buffer, MAX_PATH);
    if (written == 0) {
        return std::filesystem::path(L"C:\\");
    }
    return std::filesystem::path(std::wstring(buffer) + L"\\");
}

std::uint64_t safeDirectorySize(const std::filesystem::path &root, int maxDepth, std::size_t maxFiles) {
    std::uint64_t bytes = 0;
    std::size_t filesSeen = 0;
    std::deque<std::pair<std::filesystem::path, int>> queue;
    queue.emplace_back(root, 0);
    std::error_code error;

    while (!queue.empty() && filesSeen < maxFiles) {
        const auto [current, depth] = queue.front();
        queue.pop_front();

        for (const auto &entry :
             std::filesystem::directory_iterator(current, std::filesystem::directory_options::skip_permission_denied, error)) {
            if (error) {
                error.clear();
                continue;
            }

            if (entry.is_regular_file(error)) {
                bytes += entry.file_size(error);
                ++filesSeen;
                error.clear();
                continue;
            }

            if (depth < maxDepth && entry.is_directory(error) && !entry.is_symlink(error)) {
                queue.emplace_back(entry.path(), depth + 1);
            }
            error.clear();
        }
    }

    return bytes;
}

}  // namespace

DiskSummary DiskAnalyzer::analyzeSystemDrive(std::size_t maxLargeFiles) const {
    DiskSummary summary;

    const auto root = systemDriveRoot();
    ULARGE_INTEGER freeBytesAvailable {}, totalBytes {}, freeBytes {};
    if (GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &freeBytes)) {
        summary.totalBytes = totalBytes.QuadPart;
        summary.usedBytes = totalBytes.QuadPart - freeBytes.QuadPart;
        if (summary.totalBytes > 0) {
            summary.usedPercent =
                (static_cast<double>(summary.usedBytes) / static_cast<double>(summary.totalBytes)) * 100.0;
        }
    }

    const auto now = std::chrono::steady_clock::now();
    const bool shouldDeepScan = cachedCategories_.empty() || (now - lastDeepScan_) > std::chrono::minutes(5);
    if (shouldDeepScan) {
        cachedCategories_ = buildStorageMap(root);
        cachedLargeFiles_ = findLargestFiles(root, maxLargeFiles);
        lastDeepScan_ = now;
    }

    summary.categories = cachedCategories_;
    summary.largeFiles = cachedLargeFiles_;
    return summary;
}

std::vector<StorageCategory> DiskAnalyzer::buildStorageMap(const std::filesystem::path &root) const {
    std::vector<StorageCategory> measuredCategories;
    std::error_code error;

    for (const auto &entry :
         std::filesystem::directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error) {
            error.clear();
            continue;
        }
        if (!entry.is_directory(error) || entry.is_symlink(error)) {
            error.clear();
            continue;
        }

        auto name = entry.path().filename().string();
        if (name.empty()) {
            continue;
        }

        const std::uint64_t bytes = safeDirectorySize(entry.path(), 3, 22000);
        if (bytes == 0) {
            continue;
        }
        measuredCategories.push_back(StorageCategory {name, bytes, ""});
    }

    std::sort(measuredCategories.begin(),
              measuredCategories.end(),
              [](const StorageCategory &left, const StorageCategory &right) { return left.bytes > right.bytes; });

    constexpr std::size_t kMaxVisibleCategories = 6;
    std::vector<StorageCategory> categories;
    categories.reserve(kMaxVisibleCategories + 1);

    std::uint64_t knownBytes = 0;
    for (const auto &category : measuredCategories) {
        knownBytes += category.bytes;
    }

    std::uint64_t visibleBytes = 0;
    for (std::size_t index = 0; index < measuredCategories.size() && index < kMaxVisibleCategories; ++index) {
        categories.push_back(measuredCategories[index]);
        visibleBytes += measuredCategories[index].bytes;
    }

    ULARGE_INTEGER freeBytesAvailable {}, totalBytes {}, freeBytes {};
    std::uint64_t usedBytes = 0;
    if (GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &freeBytes)) {
        usedBytes = totalBytes.QuadPart - freeBytes.QuadPart;
    }

    const std::uint64_t unknownBytes = usedBytes > knownBytes ? usedBytes - knownBytes : 0;
    const std::uint64_t trimmedBytes = knownBytes > visibleBytes ? knownBytes - visibleBytes : 0;
    const std::uint64_t otherBytes = unknownBytes + trimmedBytes;
    if (otherBytes > 0) {
        categories.push_back(StorageCategory {"Other", otherBytes, ""});
    }

    return categories;
}

std::vector<FileEntry> DiskAnalyzer::findLargestFiles(const std::filesystem::path &root, std::size_t limit) const {
    std::vector<FileEntry> largestFiles;
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

        largestFiles.push_back(FileEntry {.path = entry.path().string(), .bytes = entry.file_size(error)});
    }

    std::sort(largestFiles.begin(), largestFiles.end(),
              [](const FileEntry &left, const FileEntry &right) { return left.bytes > right.bytes; });
    if (largestFiles.size() > limit) {
        largestFiles.resize(limit);
    }
    return largestFiles;
}

}  // namespace pulseboost
