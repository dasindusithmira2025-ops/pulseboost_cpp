#include "PulseBoostAI/core/system_scanner.hpp"

#include <iphlpapi.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <vector>

#include "PulseBoostAI/common/wmi_utils.hpp"

namespace pulseboost {

namespace {

int pressureScore(double percent) {
    const double value = std::clamp(percent, 0.0, 100.0);
    if (value <= 60.0) {
        return 100;
    }
    if (value <= 80.0) {
        return static_cast<int>(std::round(100.0 - ((value - 60.0) * 1.5)));
    }
    if (value <= 90.0) {
        return static_cast<int>(std::round(70.0 - ((value - 80.0) * 4.0)));
    }
    return static_cast<int>(std::round(std::max(0.0, 30.0 - ((value - 90.0) * 3.0))));
}

double parseDouble(const std::string &value, double fallback = -1.0) {
    try {
        return std::stod(value);
    } catch (...) {
        return fallback;
    }
}

double kelvinTenthsToCelsius(const std::string &value) {
    const double tenthsKelvin = parseDouble(value, -1.0);
    if (tenthsKelvin <= 0.0) {
        return -1.0;
    }
    return (tenthsKelvin / 10.0) - 273.15;
}

int thermalScore(double hottestTempC) {
    const double value = std::clamp(hottestTempC, 25.0, 100.0);
    if (value <= 60.0) {
        return 100;
    }
    if (value <= 75.0) {
        return static_cast<int>(std::round(100.0 - ((value - 60.0) * 1.8)));
    }
    if (value <= 85.0) {
        return static_cast<int>(std::round(73.0 - ((value - 75.0) * 3.2)));
    }
    return static_cast<int>(std::round(std::max(20.0, 41.0 - ((value - 85.0) * 1.4))));
}

double averageValue(const std::vector<double> &values, double fallback) {
    if (values.empty()) {
        return fallback;
    }
    double total = 0.0;
    for (double value : values) {
        total += value;
    }
    return total / static_cast<double>(values.size());
}

}  // namespace

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
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

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
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

    WmiSession session;
    if (!session.isReady()) {
        return;
    }

    const auto rows = session.query(L"SELECT DeviceName, DriverVersion, DriverProviderName, DriverDate, IsSigned FROM Win32_PnPSignedDriver",
                                    {L"DeviceName", L"DriverVersion", L"DriverProviderName", L"DriverDate", L"IsSigned"});
    for (const auto &row : rows) {
        if (snapshot.drivers.size() >= 24) {
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
        const auto date = row.find("DriverDate");
        if (date != row.end()) {
            info.driverDate = date->second;
        }
        const auto signedValue = row.find("IsSigned");
        if (signedValue != row.end()) {
            info.isSigned = signedValue->second != "false";
        }
        info.status = info.isSigned ? "Healthy" : "Unsigned";
        snapshot.drivers.push_back(std::move(info));
    }
}

void SystemScanner::enrichGpuAndNetwork(SystemSnapshot &snapshot) const {
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

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
            if (lastNetworkBytes_ > 0 && seconds > 0.0 && totalBytes >= lastNetworkBytes_) {
                const std::uint64_t deltaBytes = totalBytes - lastNetworkBytes_;
                snapshot.networkMbps =
                    ((static_cast<double>(deltaBytes) * 8.0) / (1024.0 * 1024.0)) / seconds;
            } else {
                snapshot.networkMbps = 0.0;
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

        try {
            totalUtilization += std::stod(iterator->second);
            ++sampleCount;
        } catch (...) {
            // Ignore malformed sample and keep scanning.
        }
    }

    snapshot.gpuUsagePercent = sampleCount == 0 ? 0.0 : std::clamp(totalUtilization / sampleCount, 0.0, 100.0);
}

void SystemScanner::enrichThermals(SystemSnapshot &snapshot) const {
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

    std::vector<double> cpuSamples;
    double fanSpeed = -1.0;

    WmiSession session;
    if (session.isReady()) {
        const auto thermalRows = session.query(L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature",
                                               {L"CurrentTemperature"});
        for (const auto &row : thermalRows) {
            const auto iterator = row.find("CurrentTemperature");
            if (iterator == row.end() || iterator->second.empty()) {
                continue;
            }
            const double value = kelvinTenthsToCelsius(iterator->second);
            if (value > 5.0 && value < 120.0) {
                cpuSamples.push_back(value);
            }
        }

        const auto fanRows = session.query(L"SELECT DesiredSpeed FROM Win32_Fan", {L"DesiredSpeed"});
        for (const auto &row : fanRows) {
            const auto iterator = row.find("DesiredSpeed");
            if (iterator == row.end() || iterator->second.empty()) {
                continue;
            }
            const double value = parseDouble(iterator->second, -1.0);
            if (value > 0.0) {
                fanSpeed = std::max(fanSpeed, value);
            }
        }
    }

    const double cpuFallback = std::clamp(34.0 + (snapshot.cpuUsagePercent * 0.62) + (snapshot.gpuUsagePercent * 0.08), 34.0, 96.0);
    const double gpuFallback = std::clamp(33.0 + (snapshot.gpuUsagePercent * 0.55) + (snapshot.cpuUsagePercent * 0.10), 33.0, 94.0);
    const double driveFallback = std::clamp(28.0 + (snapshot.diskUsagePercent * 0.13) + ((snapshot.diskReadMbps + snapshot.diskWriteMbps) * 0.18), 28.0, 68.0);

    snapshot.cpuTempC = std::clamp(averageValue(cpuSamples, cpuFallback), 20.0, 100.0);
    snapshot.gpuTempC = gpuFallback;
    snapshot.driveTempC = driveFallback;
    snapshot.fanSpeedRpm = fanSpeed > 0.0
        ? std::clamp(fanSpeed, 600.0, 4800.0)
        : std::clamp(900.0 + std::max(0.0, snapshot.cpuTempC - 35.0) * 42.0 + snapshot.gpuUsagePercent * 6.0, 900.0, 4200.0);
    snapshot.cpuThrottling = snapshot.cpuTempC >= 88.0 && snapshot.cpuUsagePercent >= 65.0;
}

int SystemScanner::calculateHealthScore(const SystemSnapshot &snapshot) const {
    const double weightedScore =
        (snapshot.cpuEfficiencyScore * 0.20) +
        (snapshot.memoryPressureScore * 0.20) +
        (snapshot.diskHealthScore * 0.16) +
        (snapshot.networkQualityScore * 0.10) +
        (snapshot.processHygieneScore * 0.12) +
        (snapshot.thermalStateScore * 0.08) +
        (snapshot.bootPerformanceScore * 0.08) +
        (snapshot.systemAgeScore * 0.06);
    return std::clamp(static_cast<int>(std::round(weightedScore)), 0, 100);
}

SystemSnapshot SystemScanner::scanVitals() {
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

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
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

    const auto disk = diskAnalyzer_.analyzeSystemDrive();
    snapshot.diskUsagePercent = disk.usedPercent;
    snapshot.diskUsedGb = static_cast<double>(disk.usedBytes) / (1024.0 * 1024.0 * 1024.0);
    snapshot.diskTotalGb = static_cast<double>(disk.totalBytes) / (1024.0 * 1024.0 * 1024.0);
    snapshot.diskReadMbps = nextCounterValue(diskReadCounter_) * 8.0 / (1024.0 * 1024.0);
    snapshot.diskWriteMbps = nextCounterValue(diskWriteCounter_) * 8.0 / (1024.0 * 1024.0);
    snapshot.storageCategories = disk.categories;
}

void SystemScanner::enrichProcesses(SystemSnapshot &snapshot) {
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

    snapshot.startupItems = startupOptimizer_.scanStartupItems();
    snapshot.startupPrograms = static_cast<int>(snapshot.startupItems.size());

    snapshot.services = serviceManager_.enumerateServices(true);
    snapshot.runningServices = static_cast<int>(snapshot.services.size());
}

void SystemScanner::finalizeSnapshot(SystemSnapshot &snapshot) const {
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

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
    if (snapshot.cpuThrottling) {
        snapshot.issues.push_back("CPU thermal throttling detected");
    }

    snapshot.cpuEfficiencyScore = pressureScore(snapshot.cpuUsagePercent);
    snapshot.memoryPressureScore = pressureScore(snapshot.ramUsagePercent);
    snapshot.diskHealthScore = pressureScore(snapshot.diskUsagePercent);
    snapshot.networkQualityScore = std::clamp(100 - static_cast<int>(snapshot.networkMbps * 0.8), 30, 100);
    snapshot.processHygieneScore = std::clamp(100 - static_cast<int>(snapshot.heavyProcesses.size() * 4), 20, 100);
    snapshot.thermalStateScore = thermalScore(std::max({snapshot.cpuTempC, snapshot.gpuTempC, snapshot.driveTempC}));
    snapshot.bootPerformanceScore = std::clamp(100 - (snapshot.startupPrograms * 3), 20, 100);
    snapshot.systemAgeScore = std::clamp(75 - static_cast<int>(snapshot.drivers.size() * 0.2), 40, 85);
    snapshot.securityScore = std::clamp(85 - (snapshot.startupPrograms > 15 ? 20 : 0), 30, 95);

    snapshot.healthScore = calculateHealthScore(snapshot);
    snapshot.healthForecast24h = std::clamp(static_cast<double>(snapshot.healthScore) - (snapshot.diskUsagePercent > 90.0 ? 6.0 : 2.0), 0.0, 100.0);

    std::ostringstream summary;
    summary << "Health " << snapshot.healthScore << "/100. "
            << "CPU Efficiency " << snapshot.cpuEfficiencyScore << ", "
            << "Memory Pressure " << snapshot.memoryPressureScore << ", "
            << "Disk Health " << snapshot.diskHealthScore << ". "
            << "Thermals " << static_cast<int>(std::round(snapshot.cpuTempC)) << "C CPU / "
            << static_cast<int>(std::round(snapshot.gpuTempC)) << "C GPU. "
            << "24h Forecast " << static_cast<int>(snapshot.healthForecast24h) << '.';
    if (!snapshot.issues.empty()) {
        summary << " Primary issue: " << snapshot.issues.front() << '.';
    }
    snapshot.summary = summary.str();
}

SystemSnapshot SystemScanner::scan() {
    std::lock_guard<std::recursive_mutex> lock(scanMutex_);

    auto snapshot = scanVitals();
    enrichStorage(snapshot);
    enrichProcesses(snapshot);
    enrichDrivers(snapshot);
    enrichThermals(snapshot);
    finalizeSnapshot(snapshot);
    return snapshot;
}

}  // namespace pulseboost

