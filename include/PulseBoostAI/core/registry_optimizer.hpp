#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace pulseboost {

class RegistryOptimizer {
public:
    bool createRestorePoint(const std::string &description) const;
    std::optional<std::filesystem::path> backupRunKeys(const std::filesystem::path &outputDirectory) const;
    bool deleteRunValue(const std::wstring &subKey, const std::wstring &valueName, bool currentUser) const;
    bool setRunValue(const std::wstring &subKey,
                     const std::wstring &valueName,
                     const std::wstring &valueData,
                     bool currentUser) const;
};

}  // namespace pulseboost
