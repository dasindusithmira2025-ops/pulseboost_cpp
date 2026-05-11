#pragma once

#include <string>

namespace pulseboost {

/// Configures unhandled exception filters to generate crash artifacts.
class CrashReporter {
public:
    /// Initializes the global crash handlers. Call early in main().
    static void initialize(const std::string &dumpDirectory);

    /// Manually triggers a crash for testing purposes.
    static void triggerCrash();

    static bool hasPendingCrashReport();
    static std::string pendingCrashReportPath();
    static bool clearPendingCrashReport();
};

}  // namespace pulseboost
