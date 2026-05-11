#include "PulseBoostAI/core/system_advisor.hpp"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

namespace pulseboost {

namespace {

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::optional<std::uint32_t> readDwordValue(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD type = 0;
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LONG status = RegQueryValueExW(keyHandle,
                                         valueName.c_str(),
                                         nullptr,
                                         &type,
                                         reinterpret_cast<BYTE *>(&value),
                                         &size);
    RegCloseKey(keyHandle);
    if (status != ERROR_SUCCESS || type != REG_DWORD) {
        return std::nullopt;
    }
    return value;
}


std::optional<std::wstring> readStringValue(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD type = 0;
    DWORD bytes = 0;
    LONG status = RegQueryValueExW(keyHandle, valueName.c_str(), nullptr, &type, nullptr, &bytes);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        RegCloseKey(keyHandle);
        return std::nullopt;
    }

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    status = RegQueryValueExW(keyHandle,
                              valueName.c_str(),
                              nullptr,
                              &type,
                              reinterpret_cast<BYTE *>(value.data()),
                              &bytes);
    RegCloseKey(keyHandle);
    if (status != ERROR_SUCCESS) {
        return std::nullopt;
    }

    if (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return value;
}

const TweakDefinition *findTweak(const std::vector<TweakDefinition> &tweaks, const std::string &id) {
    const auto iterator = std::find_if(tweaks.begin(), tweaks.end(), [&](const TweakDefinition &tweak) {
        return tweak.id == id;
    });
    return iterator == tweaks.end() ? nullptr : &(*iterator);
}

AdvisorItem makeItem(const std::string &id,
                     const std::string &category,
                     const std::string &title,
                     const std::string &description,
                     const std::string &impact,
                     const std::string &status,
                     bool actionable,
                     const std::string &actionLabel = {},
                     const std::string &actionId = {}) {
    AdvisorItem item;
    item.id = id;
    item.category = category;
    item.title = title;
    item.description = description;
    item.impact = impact;
    item.status = status;
    item.actionable = actionable;
    item.actionLabel = actionLabel;
    item.actionId = actionId;
    return item;
}

}  // namespace

std::vector<AdvisorItem> SystemAdvisor::analyze(const SystemSnapshot &snapshot,
                                                const std::vector<TweakDefinition> &tweaks) const {
    std::vector<AdvisorItem> items;

    const bool vbsEnabled = readDwordValue(HKEY_LOCAL_MACHINE,
                                           L"SYSTEM\\CurrentControlSet\\Control\\DeviceGuard",
                                           L"EnableVirtualizationBasedSecurity").value_or(0) != 0;
    items.push_back(makeItem(
        "vbs_status",
        "security",
        "Virtualization-Based Security",
        vbsEnabled
            ? "VBS is enabled. That improves isolation but can reduce gaming and low-latency performance on some systems."
            : "VBS is disabled, which usually favors raw performance on gaming-first systems.",
        "high",
        vbsEnabled ? "warning" : "optimal",
        false));

    if (const auto *gpuScheduling = findTweak(tweaks, "gpu_enable_hardware_gpu_scheduling"); gpuScheduling != nullptr) {
        items.push_back(makeItem(
            "gpu_scheduling",
            "gpu",
            "Hardware GPU Scheduling",
            gpuScheduling->isApplied
                ? "Hardware GPU scheduling is already enabled."
                : "Hardware GPU scheduling is off. Enabling it can reduce queueing overhead on supported GPUs.",
            "high",
            gpuScheduling->isApplied ? "optimal" : "suboptimal",
            !gpuScheduling->isApplied,
            "Enable GPU Scheduling",
            gpuScheduling->id));
    }

    if (const auto *networkTweak = findTweak(tweaks, "network_disable_network_throttling"); networkTweak != nullptr) {
        items.push_back(makeItem(
            "network_throttling",
            "windows",
            "Windows Network Throttling",
            networkTweak->isApplied
                ? "Network throttling is already disabled for multimedia traffic."
                : "Windows network throttling is still active. Competitive games can benefit from disabling it.",
            "high",
            networkTweak->isApplied ? "optimal" : "suboptimal",
            !networkTweak->isApplied,
            "Disable Throttling",
            networkTweak->id));
    }

    if (const auto *powerTweak = findTweak(tweaks, "power_disable_power_throttling"); powerTweak != nullptr) {
        items.push_back(makeItem(
            "power_throttling",
            "windows",
            "Power Throttling",
            powerTweak->isApplied
                ? "Power throttling is already disabled."
                : "Windows can downclock bursts of foreground work. Disabling power throttling helps sustained responsiveness.",
            "medium",
            powerTweak->isApplied ? "optimal" : "suboptimal",
            !powerTweak->isApplied,
            "Disable Power Throttling",
            powerTweak->id));
    }

    const std::size_t enabledStartup = std::count_if(snapshot.startupItems.begin(), snapshot.startupItems.end(), [](const StartupItem &item) {
        return item.enabled;
    });
    items.push_back(makeItem(
        "startup_pressure",
        "windows",
        "Startup Load",
        enabledStartup > 12
            ? "This system has a heavy startup load. Disabling a few high-impact entries should improve boot-to-desktop time."
            : "Startup load is within a reasonable range.",
        enabledStartup > 12 ? "medium" : "low",
        enabledStartup > 12 ? "warning" : "optimal",
        false));

    items.push_back(makeItem(
        "thermal_headroom",
        "windows",
        "Thermal Headroom",
        snapshot.cpuTempC >= 85.0
            ? "CPU temperature is already in a throttling-risk zone. Performance tuning should wait until cooling improves."
            : "Thermal headroom looks acceptable for additional tuning.",
        snapshot.cpuTempC >= 85.0 ? "high" : "low",
        snapshot.cpuTempC >= 85.0 ? "warning" : "optimal",
        false));

    items.push_back(makeItem(
        "memory_pressure",
        "ram",
        "Memory Pressure",
        snapshot.ramUsagePercent >= 85.0
            ? "RAM pressure is high right now. A RAM cleanup and startup reduction will likely help immediately."
            : "Memory pressure is stable.",
        snapshot.ramUsagePercent >= 85.0 ? "high" : "low",
        snapshot.ramUsagePercent >= 85.0 ? "warning" : "optimal",
        false));

    return items;
}

}  // namespace pulseboost
