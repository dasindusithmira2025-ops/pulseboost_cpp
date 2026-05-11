#include "PulseBoostAI/modules/junk_cleaner.hpp"

#include <Windows.h>
#include <shellapi.h>

#include <cstdlib>
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

std::filesystem::path uniqueQuarantinePath(const std::filesystem::path &root,
                                           const std::filesystem::path &source) {
    std::filesystem::path filename = source.filename();
    if (filename.empty()) {
        filename = "item";
    }

    std::filesystem::path candidate = root / filename;
    std::error_code error;
    int suffix = 1;
    while (std::filesystem::exists(candidate, error) && !error) {
        candidate = root / (filename.string() + "." + std::to_string(suffix++));
    }
    return candidate;
}

bool recyclePath(const std::filesystem::path &path) {
    std::wstring from = path.wstring();
    from.push_back(L'\0');
    from.push_back(L'\0');

    SHFILEOPSTRUCTW operation {};
    operation.wFunc = FO_DELETE;
    operation.pFrom = from.c_str();
    operation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    return SHFileOperationW(&operation) == 0 && !operation.fAnyOperationsAborted;
}

}  // namespace

std::filesystem::path JunkCleaner::defaultQuarantineRoot() {
    if (const char *appData = std::getenv("APPDATA"); appData != nullptr && *appData != '\0') {
        return std::filesystem::path(appData) / "PulseBoostAI" / "quarantine";
    }
    return std::filesystem::path("data") / "quarantine";
}

std::vector<std::filesystem::path> JunkCleaner::candidateTargets(const CleanupOptions &options) const {
    if (!options.overrideTargets.empty()) {
        return options.overrideTargets;
    }

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

CleanupResult JunkCleaner::cleanSafeTargets(const CleanupOptions &options) const {
    CleanupResult result;
    result.dryRun = options.dryRun;
    result.permanentDelete = options.mode == CleanupMode::PermanentDelete;

    std::error_code error;
    const auto quarantineRoot = options.quarantineRoot.empty() ? defaultQuarantineRoot() : options.quarantineRoot;
    if (!options.dryRun && options.mode == CleanupMode::Quarantine) {
        std::filesystem::create_directories(quarantineRoot, error);
        if (error) {
            ++result.failures;
            result.actions.push_back("Failed to create quarantine " + quarantineRoot.string());
            return result;
        }
    }

    for (const auto &target : candidateTargets(options)) {
        for (const auto &entry :
             std::filesystem::directory_iterator(target, std::filesystem::directory_options::skip_permission_denied, error)) {
            if (error) {
                error.clear();
                continue;
            }

            std::uint64_t bytes = 0;
            if (entry.is_regular_file(error)) {
                bytes = entry.file_size(error);
            } else if (entry.is_directory(error)) {
                bytes = directoryBytes(entry.path());
            }
            if (error) {
                ++result.failures;
                error.clear();
                continue;
            }

            ++result.filesScanned;
            if (options.dryRun) {
                result.bytesRecovered += bytes;
                result.actions.push_back("Would clean " + entry.path().string());
                continue;
            }

            bool ok = false;
            if (options.mode == CleanupMode::Quarantine) {
                const auto destination = uniqueQuarantinePath(quarantineRoot, entry.path());
                std::filesystem::rename(entry.path(), destination, error);
                ok = !error;
                if (ok) {
                    ++result.filesQuarantined;
                    ++result.filesRemoved;
                    result.actions.push_back("Quarantined " + entry.path().string() + " -> " + destination.string());
                }
            } else if (options.mode == CleanupMode::Recycle) {
                ok = recyclePath(entry.path());
                if (ok) {
                    ++result.filesRecycled;
                    ++result.filesRemoved;
                    result.actions.push_back("Recycled " + entry.path().string());
                }
            } else {
                if (entry.is_directory(error)) {
                    const auto removedCount = std::filesystem::remove_all(entry.path(), error);
                    ok = !error;
                    if (ok) {
                        result.filesRemoved += static_cast<int>(removedCount);
                    }
                } else {
                    ok = std::filesystem::remove(entry.path(), error);
                    if (ok) {
                        ++result.filesRemoved;
                    }
                }
                if (ok) {
                    result.actions.push_back("Permanently deleted " + entry.path().string());
                }
            }

            if (ok) {
                result.bytesRecovered += bytes;
            } else {
                ++result.failures;
                error.clear();
            }
        }
        result.actions.push_back((options.dryRun ? "Scanned " : "Processed ") + target.string());
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
