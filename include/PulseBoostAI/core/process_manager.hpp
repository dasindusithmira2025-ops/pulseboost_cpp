#pragma once

#include <Windows.h>

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class ProcessManager {
public:
    std::vector<ProcessInfo> enumerateProcesses();
    bool setPriority(std::uint32_t pid, DWORD priorityClass) const;
    bool setAffinity(std::uint32_t pid, std::uint64_t affinityMask) const;
    bool suspendProcess(std::uint32_t pid) const;
    bool resumeProcess(std::uint32_t pid) const;

private:
    struct CpuSample {
        std::uint64_t processTime = 0;
        std::uint64_t wallClock = 0;
    };

    mutable std::mutex cpuSamplesMutex_;
    std::unordered_map<std::uint32_t, CpuSample> cpuSamples_;
};

}  // namespace pulseboost
