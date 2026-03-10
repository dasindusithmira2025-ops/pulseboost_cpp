#include "PulseBoostAI/core/service_manager.hpp"

#include <Windows.h>

#include <algorithm>
#include <vector>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

std::string startModeName(DWORD startType) {
    switch (startType) {
    case SERVICE_AUTO_START:
        return "Automatic";
    case SERVICE_DEMAND_START:
        return "Manual";
    case SERVICE_DISABLED:
        return "Disabled";
    default:
        return "Unknown";
    }
}

std::string serviceStateName(DWORD state) {
    switch (state) {
    case SERVICE_RUNNING:
        return "Running";
    case SERVICE_STOPPED:
        return "Stopped";
    case SERVICE_PAUSED:
        return "Paused";
    default:
        return "Pending";
    }
}

}  // namespace

std::vector<ServiceInfo> ServiceManager::enumerateServices(bool runningOnly) const {
    std::vector<ServiceInfo> services;

    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (manager == nullptr) {
        return services;
    }

    DWORD bytesNeeded = 0;
    DWORD serviceCount = 0;
    DWORD resumeHandle = 0;
    EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, runningOnly ? SERVICE_ACTIVE : SERVICE_STATE_ALL,
                          nullptr, 0, &bytesNeeded, &serviceCount, &resumeHandle, nullptr);

    std::vector<BYTE> buffer(bytesNeeded);
    if (!EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, runningOnly ? SERVICE_ACTIVE : SERVICE_STATE_ALL,
                               buffer.data(), bytesNeeded, &bytesNeeded, &serviceCount, &resumeHandle, nullptr)) {
        CloseServiceHandle(manager);
        return services;
    }

    auto *entries = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW *>(buffer.data());
    for (DWORD index = 0; index < serviceCount; ++index) {
        const auto &entry = entries[index];
        SC_HANDLE serviceHandle = OpenServiceW(manager, entry.lpServiceName, SERVICE_QUERY_CONFIG);
        std::string startMode = "Unknown";
        if (serviceHandle != nullptr) {
            DWORD configBytesNeeded = 0;
            QueryServiceConfigW(serviceHandle, nullptr, 0, &configBytesNeeded);
            std::vector<BYTE> configBuffer(configBytesNeeded);
            auto *config = reinterpret_cast<QUERY_SERVICE_CONFIGW *>(configBuffer.data());
            if (QueryServiceConfigW(serviceHandle, config, configBytesNeeded, &configBytesNeeded)) {
                startMode = startModeName(config->dwStartType);
            }
            CloseServiceHandle(serviceHandle);
        }

        services.push_back(ServiceInfo {.name = fromWide(entry.lpServiceName),
                                        .displayName = fromWide(entry.lpDisplayName),
                                        .state = serviceStateName(entry.ServiceStatusProcess.dwCurrentState),
                                        .startMode = startMode,
                                        .canStop = (entry.ServiceStatusProcess.dwControlsAccepted & SERVICE_ACCEPT_STOP) != 0});
    }

    std::sort(services.begin(), services.end(), [](const ServiceInfo &left, const ServiceInfo &right) {
        return left.displayName < right.displayName;
    });

    CloseServiceHandle(manager);
    return services;
}

bool ServiceManager::stopService(const std::string &serviceName) const {
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        return false;
    }

    SC_HANDLE service = OpenServiceW(manager, toWide(serviceName).c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (service == nullptr) {
        CloseServiceHandle(manager);
        return false;
    }

    SERVICE_STATUS status {};
    const bool success = ControlService(service, SERVICE_CONTROL_STOP, &status) == TRUE;
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return success;
}

bool ServiceManager::startService(const std::string &serviceName) const {
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        return false;
    }

    SC_HANDLE service = OpenServiceW(manager, toWide(serviceName).c_str(), SERVICE_START);
    if (service == nullptr) {
        CloseServiceHandle(manager);
        return false;
    }

    const bool success = StartServiceW(service, 0, nullptr) == TRUE;
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return success;
}

}  // namespace pulseboost
