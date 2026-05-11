#pragma once

#include <string>

namespace pulseboost {

/// Checks a remote JSON manifest for newer application versions
/// and handles background downloads.
class AutoUpdater {
public:
    AutoUpdater();
    ~AutoUpdater();

    /// Pings the update endpoint to check if a higher version exists.
    bool checkForUpdates(const std::string &currentVersion);

    /// Downloads the new installer to a temp location.
    bool downloadUpdate();

    /// Launches the downloaded installer and exits the app.
    bool installAndRestart();

    bool updateAvailable() const;
    std::string latestVersion() const;
    std::string downloadUrl() const;
    std::string expectedSha256() const;
    bool manifestTrusted() const;
    std::string lastError() const;
    bool hasDownloadedInstaller() const;

private:
    struct Impl;
    Impl* pImpl;
};

}  // namespace pulseboost
