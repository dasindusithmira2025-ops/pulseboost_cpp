#include "PulseBoostAI/modules/ram_optimizer.hpp"

#include <Windows.h>
#include <psapi.h>

#include <algorithm>

namespace pulseboost {

RamOptimizer::RamOptimizer(ProcessManager &processManager) : processManager_(processManager) {}

bool RamOptimizer::trimWorkingSet(HANDLE processHandle) {
    if (processHandle == nullptr) {
        return false;
    }
    return EmptyWorkingSet(processHandle) == TRUE;
}

RamOptimizationResult RamOptimizer::optimizeWorkingSets(double minMemoryMb) const {
    RamOptimizationResult result;
    const auto processes = processManager_.enumerateProcesses();

    for (const auto &process : processes) {
        if (process.isCritical || process.memoryMb < minMemoryMb) {
            ++result.processesSkipped;
            continue;
        }

        HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_QUOTA, FALSE, process.pid);
        if (processHandle == nullptr) {
            ++result.processesSkipped;
            continue;
        }

        if (trimWorkingSet(processHandle)) {
            ++result.processesOptimized;
        } else {
            ++result.processesSkipped;
        }
        CloseHandle(processHandle);
    }

    return result;
}

RamOptimizationResult RamOptimizer::flushStandbyList() const {
    RamOptimizationResult result = optimizeWorkingSets(64.0);
    HANDLE currentProcess = GetCurrentProcess();
    if (!trimWorkingSet(currentProcess)) {
        ++result.processesSkipped;
    }
    return result;
}

RamOptimizationResult RamOptimizer::enableRamSaverMode() const {
    RamOptimizationResult result = optimizeWorkingSets(128.0);
    HANDLE currentProcess = GetCurrentProcess();
    if (!trimWorkingSet(currentProcess)) {
        ++result.processesSkipped;
    }
    return result;
}

RamBreakdown RamOptimizer::currentBreakdown() const {
    RamBreakdown breakdown;

    MEMORYSTATUSEX memoryStatus {};
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (!GlobalMemoryStatusEx(&memoryStatus)) {
        return breakdown;
    }

    PERFORMANCE_INFORMATION performanceInfo {};
    performanceInfo.cb = sizeof(performanceInfo);
    if (!GetPerformanceInfo(&performanceInfo, sizeof(performanceInfo))) {
        return breakdown;
    }

    const double megabyte = 1024.0 * 1024.0;
    const double pageToMb = static_cast<double>(performanceInfo.PageSize) / megabyte;

    breakdown.totalMb = static_cast<double>(memoryStatus.ullTotalPhys) / megabyte;
    breakdown.freeMb = static_cast<double>(memoryStatus.ullAvailPhys) / megabyte;
    breakdown.usedMb = std::max(0.0, breakdown.totalMb - breakdown.freeMb);
    breakdown.cachedMb = static_cast<double>(performanceInfo.SystemCache) * pageToMb;
    breakdown.systemMb = static_cast<double>(performanceInfo.KernelPaged + performanceInfo.KernelNonpaged) * pageToMb;

    const double estimatedApps = std::max(0.0, breakdown.usedMb - breakdown.cachedMb - breakdown.systemMb);
    breakdown.appsMb = std::min(estimatedApps, breakdown.usedMb);

    const double cacheRecovery = breakdown.cachedMb * 0.55;
    const double appRecovery = std::max(0.0, breakdown.appsMb - (breakdown.totalMb * 0.32));
    breakdown.recoverableMb = std::max(256.0, std::min(breakdown.usedMb, cacheRecovery + appRecovery));

    return breakdown;
}

}  // namespace pulseboost
