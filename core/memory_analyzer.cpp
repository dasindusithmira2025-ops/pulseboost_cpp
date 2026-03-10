#include "PulseBoostAI/core/memory_analyzer.hpp"

#include <Windows.h>

#include <algorithm>

namespace pulseboost {

MemoryAnalyzer::MemoryAnalyzer(ProcessManager &processManager) : processManager_(processManager) {}

MemorySummary MemoryAnalyzer::analyze() {
    MEMORYSTATUSEX status {};
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);

    auto processes = processManager_.enumerateProcesses();
    std::sort(processes.begin(), processes.end(), [](const ProcessInfo &left, const ProcessInfo &right) {
        return left.memoryMb > right.memoryMb;
    });
    if (processes.size() > 10) {
        processes.resize(10);
    }

    MemorySummary summary;
    summary.usedMb = static_cast<double>(status.ullTotalPhys - status.ullAvailPhys) / (1024.0 * 1024.0);
    summary.totalMb = static_cast<double>(status.ullTotalPhys) / (1024.0 * 1024.0);
    summary.usedPercent = static_cast<double>(status.dwMemoryLoad);
    summary.topConsumers = std::move(processes);
    return summary;
}

}  // namespace pulseboost
