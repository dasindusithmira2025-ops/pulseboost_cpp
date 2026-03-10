#pragma once

#include <Windows.h>
#include <string>

namespace pulseboost {

/// Safety operations to run before any destructive optimization.
class SafetyGuard {
public:
    /// Create a Windows System Restore point via WMI.
    /// Returns true on success.
    bool createRestorePoint(const std::wstring &description);

    /// Export a registry key to a .reg backup file.
    bool backupRegistryKey(HKEY root, const std::wstring &subKey, const std::wstring &dstPath);

    /// Append an action record to logs/safety.log.
    void logAction(const std::string &module, const std::string &detail, bool success);
};

}  // namespace pulseboost
