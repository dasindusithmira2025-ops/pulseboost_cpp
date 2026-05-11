#include "PulseBoostAI/modules/junk_cleaner.hpp"

#include <Windows.h>

#include <filesystem>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

std::uint64_t directoryBytes(const std::filesystem::path &root) {
    std::uint64_t bytes = 0;
    std::error_code error;
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error) {
            error.clear();
            continue;
        }
        if (entry.is_regular_file(error)) {
            bytes += entry.file_size(error);
        } else {
            error.clear();
        }
    }
    return bytes;
}

}  // namespace

std::vector<std::filesystem::path> JunkCleaner::candidateTargets() const {
    std::vector<std::filesystem::path> targets;
    for (const auto &rawPath :
         {L"%TEMP%", L"%LOCALAPPDATA%\\Temp", L"%WINDIR%\\Temp", L"%LOCALAPPDATA%\\Microsoft\\Windows\\INetCache",
          L"%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Cache",
          L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\Cache"}) {
        const auto expanded = expandedPath(rawPath);
        if (expanded.has_value() && std::filesystem::exists(*expanded)) {
            targets.push_back(*expanded);
        }
    }
    return targets;
}

std::uint64_t JunkCleaner::estimateRecoverableBytes() const {
    std::uint64_t totalBytes = 0;
    for (const auto &target : candidateTargets()) {
        std::error_code error;
        for (const auto &entry :
             std::filesystem::directory_iterator(target, std::filesystem::directory_options::skip_permission_denied, error)) {
            if (error) {
                error.clear();
                continue;
            }

            if (entry.is_regular_file(error)) {
                totalBytes += entry.file_size(error);
            } else if (entry.is_directory(error)) {
                totalBytes += directoryBytes(entry.path());
            }
            error.clear();
        }
    }
    return totalBytes;
}

CleanupResult JunkCleaner::cleanSafeTargets() const {
    CleanupResult result;
    std::error_code error;

    for (const auto &target : candidateTargets()) {
        for (const auto &entry :
             std::filesystem::directory_iterator(target, std::filesystem::directory_options::skip_permission_denied, error)) {
            if (error) {
                error.clear();
                continue;
            }

            std::uint64_t bytes = 0;
            if (entry.is_regular_file(error)) {
                bytes = entry.file_size(error);
                std::filesystem::remove(entry.path(), error);
            } else if (entry.is_directory(error)) {
                bytes = directoryBytes(entry.path());
                const auto removedCount = std::filesystem::remove_all(entry.path(), error);
                if (!error) {
                    result.filesRemoved += static_cast<int>(removedCount);
                }
            }

            if (!error) {
                result.bytesRecovered += bytes;
                if (entry.is_regular_file()) {
                    ++result.filesRemoved;
                }
            } else {
                error.clear();
            }
        }
        result.actions.push_back("Cleaned " + target.string());
    }

    return result;
}

}  // namespace pulseboost

namespace pulseboost::modules::junk_cleaner {

void run() {
    JunkCleaner cleaner;
    const auto result = cleaner.cleanSafeTargets();
    (void)result;
}

}  // namespace pulseboost::modules::junk_cleaner
