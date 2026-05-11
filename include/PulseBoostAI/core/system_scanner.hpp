#pragma once

#include <Windows.h>
#include <Pdh.h>

#include <chrono>
#include <mutex>

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/disk_analyzer.hpp"
#include "PulseBoostAI/core/memory_analyzer.hpp"
#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/core/service_manager.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"

namespace pulseboost {

class SystemScanner {
public:
    SystemScanner(ProcessManager &processManager,
                  MemoryAnalyzer &memoryAnalyzer,
                  DiskAnalyzer &diskAnalyzer,
                  StartupOptimizer &startupOptimizer,
                  ServiceManager &serviceManager);
    ~SystemScanner();

    SystemSnapshot scan();
    SystemSnapshot scanVitals();
    void enrichStorage(SystemSnapshot &snapshot);
    void enrichProcesses(SystemSnapshot &snapshot);
    void enrichDrivers(SystemSnapshot &snapshot) const;
    void enrichGpuAndNetwork(SystemSnapshot &snapshot) const;
    void enrichThermals(SystemSnapshot &snapshot) const;
    void finalizeSnapshot(SystemSnapshot &snapshot) const;

private:
    double nextCounterValue(PDH_HCOUNTER counter) const;
    int calculateHealthScore(const SystemSnapshot &snapshot) const;

    ProcessManager &processManager_;
    MemoryAnalyzer &memoryAnalyzer_;
    DiskAnalyzer &diskAnalyzer_;
    StartupOptimizer &startupOptimizer_;
    ServiceManager &serviceManager_;
    PDH_HQUERY query_ = nullptr;
    PDH_HCOUNTER cpuCounter_ = nullptr;
    PDH_HCOUNTER diskReadCounter_ = nullptr;
    PDH_HCOUNTER diskWriteCounter_ = nullptr;
    mutable std::uint64_t lastNetworkBytes_ = 0;
    mutable std::chrono::steady_clock::time_point lastNetworkSample_ = std::chrono::steady_clock::now();
    mutable std::recursive_mutex scanMutex_;
};

}  // namespace pulseboost
