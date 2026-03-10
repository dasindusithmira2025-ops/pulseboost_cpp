#include "PulseBoostAI/core/system_scanner.hpp"

#include <iphlpapi.h>

#include <algorithm>
#include <memory>
#include <sstream>

#include "PulseBoostAI/common/wmi_utils.hpp"

namespace pulseboost {

SystemScanner::SystemScanner(ProcessManager &processManager,
                             MemoryAnalyzer &memoryAnalyzer,
                             DiskAnalyzer &diskAnalyzer,
                             StartupOptimizer &startupOptimizer,
                             ServiceManager &serviceManager)
    : processManager_(processManager),
      memoryAnalyzer_(memoryAnalyzer),
      diskAnalyzer_(diskAnalyzer),
      startupOptimizer_(startupOptimizer),
      serviceManager_(serviceManager) {
    if (PdhOpenQueryW(nullptr, 0, &query_) == ERROR_SUCCESS) {
        PdhAddEnglishCounterW(query_, L"\\Processor(_Total)\\% Processor Time", 0, &cpuCounter_);
        PdhAddEnglishCounterW(query_, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &diskReadCounter_);
        PdhAddEnglishCounterW(query_, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &diskWriteCounter_);
        PdhCollectQueryData(query_);
    }
}

SystemScanner::~SystemScanner() {
    if (query_ != nullptr) {
        PdhCloseQuery(query_);
    }
}

double SystemScanner::nextCounterValue(PDH_HCOUNTER counter) const {
    if (query_ == nullptr || counter == nullptr) {
        return 0.0;
    }

    PdhCollectQueryData(query_);
    PDH_FMT_COUNTERVALUE value {};
    if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value) != ERROR_SUCCESS) {
        return 0.0;
    }
    return value.doubleValue;
}

void SystemScanner::enrichDrivers(SystemSnapshot &snapshot) const {
    WmiSession session;
    if (!session.isReady()) {
        return;
    }

    const auto rows = session.query(L"SELECT DeviceName, DriverVersion, DriverProviderName FROM Win32_PnPSignedDriver",
                                    {L"DeviceName", L"DriverVersion", L"DriverProviderName"});
    for (const auto &row : rows) {
        if (snapshot.drivers.size() >= 8) {
            break;
        }

        const auto device = row.find("DeviceName");
        const auto version = row.find("DriverVersion");
        if (device == row.end() || version == row.end() || device->second.empty() || version->second.empty()) {
            continue;
        }

        DriverInfo info;
        info.deviceName = device->second;
        info.driverVersion = version->second;
        const auto provider = row.find("DriverProviderName");
        if (provider != row.end()) {
            info.provider = provider->second;
        }
        snapshot.drivers.push_back(std::move(info));
    }
}

void SystemScanner::enrichGpuAndNetwork(SystemSnapshot &snapshot) const {
    ULONG size = 0;
    if (GetIfTable(nullptr, &size, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        auto buffer = std::make_unique<std::byte[]>(size);
        auto *table = reinterpret_cast<MIB_IFTABLE *>(buffer.get());
        if (GetIfTable(table, &size, FALSE) == NO_ERROR) {
            std::uint64_t totalBytes = 0;
            for (DWORD index = 0; index < table->dwNumEntries; ++index) {
                const auto &row = table->table[index];
                if (row.dwType == IF_TYPE_SOFTWARE_LOOPBACK || row.dwOperStatus != IF_OPER_STATUS_OPERATIONAL) {
                    continue;
                }
                totalBytes += row.dwInOctets + row.dwOutOctets;
            }

            const auto now = std::chrono::steady_clock::now();
            const double seconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastNetworkSample_).count() / 1000.0;
            if (lastNetworkBytes_ > 0 && seconds > 0.0) {
                snapshot.networkMbps =
                    ((static_cast<double>(totalBytes - lastNetworkBytes_) * 8.0) / (1024.0 * 1024.0)) / seconds;
            }
            lastNetworkBytes_ = totalBytes;
            lastNetworkSample_ = now;
        }
    }

    WmiSession session;
    if (!session.isReady()) {
        snapshot.gpuUsagePercent = 0.0;
        return;
    }

    const auto rows =
        session.query(L"SELECT UtilizationPercentage FROM Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine",
                      {L"UtilizationPercentage"});
    double totalUtilization = 0.0;
    int sampleCount = 0;
    for (const auto &row : rows) {
        const auto iterator = row.find("UtilizationPercentage");
        if (iterator == row.end() || iterator->second.empty()) {
            continue;
        }

        totalUtilization += std::stod(iterator->second);
        ++sampleCount;
    }

    snapshot.gpuUsagePercent = sampleCount == 0 ? 0.0 : std::clamp(totalUtilization / sampleCount, 0.0, 100.0);
}

int SystemScanner::calculateHealthScore(const SystemSnapshot &snapshot) const {
    int score = 100;
    score -= static_cast<int>(snapshot.cpuUsagePercent / 8.0);
    score -= static_cast<int>(snapshot.ramUsagePercent / 10.0);
    score -= static_cast<int>(snapshot.diskUsagePercent / 10.0);
    score -= std::min(snapshot.startupPrograms * 2, 20);
    score -= static_cast<int>(snapshot.heavyProcesses.size() * 2);
    return std::clamp(score, 0, 100);
}

SystemSnapshot SystemScanner::scanVitals() {
    SystemSnapshot snapshot;
    snapshot.capturedAt = std::chrono::system_clock::now();
    snapshot.cpuUsagePercent = std::clamp(nextCounterValue(cpuCounter_), 0.0, 100.0);

    const auto memory = memoryAnalyzer_.analyze();
    snapshot.ramUsagePercent = memory.usedPercent;
    snapshot.ramUsedMb = memory.usedMb;
    snapshot.ramTotalMb = memory.totalMb;
    snapshot.heavyProcesses = memory.topConsumers;
    enrichGpuAndNetwork(snapshot);
    return snapshot;
}

void SystemScanner::enrichStorage(SystemSnapshot &snapshot) {
    const auto disk = diskAnalyzer_.analyzeSystemDrive();
    snapshot.diskUsagePercent = disk.usedPercent;
    snapshot.diskUsedGb = static_cast<double>(disk.usedBytes) / (1024.0 * 1024.0 * 1024.0);
    snapshot.diskTotalGb = static_cast<double>(disk.totalBytes) / (1024.0 * 1024.0 * 1024.0);
    snapshot.diskReadMbps = nextCounterValue(diskReadCounter_) * 8.0 / (1024.0 * 1024.0);
    snapshot.diskWriteMbps = nextCounterValue(diskWriteCounter_) * 8.0 / (1024.0 * 1024.0);
    snapshot.storageCategories = disk.categories;
}

void SystemScanner::enrichProcesses(SystemSnapshot &snapshot) {
    snapshot.startupItems = startupOptimizer_.scanStartupItems();
    snapshot.startupPrograms = static_cast<int>(snapshot.startupItems.size());

    snapshot.services = serviceManager_.enumerateServices(true);
    snapshot.runningServices = static_cast<int>(snapshot.services.size());
}

void SystemScanner::finalizeSnapshot(SystemSnapshot &snapshot) const {
    snapshot.issues.clear();

    if (snapshot.cpuUsagePercent > 85.0) {
        snapshot.issues.push_back("CPU saturation detected");
    }
    if (snapshot.ramUsagePercent > 85.0) {
        snapshot.issues.push_back("Memory pressure is high");
    }
    if (snapshot.diskUsagePercent > 90.0) {
        snapshot.issues.push_back("System drive is critically full");
    }
    if (snapshot.startupPrograms > 12) {
        snapshot.issues.push_back("Too many startup applications");
    }
    if (!snapshot.heavyProcesses.empty() && snapshot.heavyProcesses.front().memoryMb > 2048.0) {
        snapshot.issues.push_back(snapshot.heavyProcesses.front().name + " is using more than 2 GB of RAM");
    }

    snapshot.healthScore = calculateHealthScore(snapshot);

    std::ostringstream summary;
    summary << "Health " << snapshot.healthScore << "/100. CPU " << static_cast<int>(snapshot.cpuUsagePercent)
            << "%, RAM " << static_cast<int>(snapshot.ramUsagePercent) << "%, Disk "
            << static_cast<int>(snapshot.diskUsagePercent) << "% used.";
    if (!snapshot.issues.empty()) {
        summary << " Primary issue: " << snapshot.issues.front() << '.';
    }
    snapshot.summary = summary.str();
}

SystemSnapshot SystemScanner::scan() {
    auto snapshot = scanVitals();
    enrichStorage(snapshot);
    enrichProcesses(snapshot);
    enrichDrivers(snapshot);
    finalizeSnapshot(snapshot);
    return snapshot;
}

}  // namespace pulseboost
