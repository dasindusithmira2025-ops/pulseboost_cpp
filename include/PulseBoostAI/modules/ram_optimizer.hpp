#pragma once

#include <cstdint>
#include <string>

#include "PulseBoostAI/core/process_manager.hpp"

namespace pulseboost {

struct RamOptimizationResult {
    std::uint64_t processesOptimized = 0;
    std::uint64_t processesSkipped = 0;
    bool advancedModeRequired = false;
    std::string message;
};

struct RamBreakdown {
    double totalMb = 0.0;
    double usedMb = 0.0;
    double freeMb = 0.0;
    double appsMb = 0.0;
    double systemMb = 0.0;
    double cachedMb = 0.0;
    double recoverableMb = 0.0;
};

class RamOptimizer {
public:
    explicit RamOptimizer(ProcessManager &processManager);

    RamOptimizationResult optimizeWorkingSets(double minMemoryMb = 256.0, bool advancedMode = false) const;
    RamOptimizationResult flushStandbyList(bool advancedMode = false) const;
    RamOptimizationResult enableRamSaverMode(bool advancedMode = false) const;
    RamBreakdown currentBreakdown() const;

private:
    static bool trimWorkingSet(HANDLE processHandle);

    ProcessManager &processManager_;
};

}  // namespace pulseboost
