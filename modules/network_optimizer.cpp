#include "PulseBoostAI/modules/network_optimizer.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <memory>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace {

struct WinsockScope {
    WinsockScope() {
        static const bool initialized = [] {
            WSADATA data {};
            return WSAStartup(MAKEWORD(2, 2), &data) == 0;
        }();
        ok = initialized;
    }

    bool ok = false;
};

std::string adapterTypeName(unsigned long ifType) {
    switch (ifType) {
        case IF_TYPE_ETHERNET_CSMACD:
            return "Ethernet";
        case IF_TYPE_IEEE80211:
            return "Wi-Fi";
        case IF_TYPE_TUNNEL:
            return "VPN/Tunnel";
        default:
            return "Other";
    }
}

std::string probeStatus(int latencyMs) {
    if (latencyMs < 0) {
        return "unreachable";
    }
    if (latencyMs <= 45) {
        return "ok";
    }
    if (latencyMs <= 90) {
        return "slow";
    }
    return "slow";
}

bool resolveIpv4(const std::string &host, IPAddr &address) {
    WinsockScope winsock;
    if (!winsock.ok) {
        return false;
    }

    IN_ADDR directAddress {};
    if (InetPtonA(AF_INET, host.c_str(), &directAddress) == 1) {
        address = directAddress.S_un.S_addr;
        return true;
    }

    addrinfo hints {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *result = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0 || result == nullptr) {
        return false;
    }

    const auto *sockaddr = reinterpret_cast<sockaddr_in *>(result->ai_addr);
    address = sockaddr->sin_addr.S_un.S_addr;
    freeaddrinfo(result);
    return true;
}

int probeLatency(const std::string &host) {
    IPAddr address = 0;
    if (!resolveIpv4(host, address)) {
        return -1;
    }

    HANDLE icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE) {
        return -1;
    }

    constexpr char payload[] = "PulseBoost";
    constexpr DWORD timeoutMs = 1200;
    std::vector<unsigned char> reply(sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 32);
    const DWORD replies = IcmpSendEcho(
        icmp,
        address,
        const_cast<char *>(payload),
        sizeof(payload),
        nullptr,
        reply.data(),
        static_cast<DWORD>(reply.size()),
        timeoutMs);

    int latency = -1;
    if (replies > 0) {
        const auto *echo = reinterpret_cast<ICMP_ECHO_REPLY *>(reply.data());
        latency = static_cast<int>(echo->RoundTripTime);
    }

    IcmpCloseHandle(icmp);
    return latency;
}

}  // namespace

namespace pulseboost {

std::vector<std::string> NetworkOptimizer::analyze() const {
    return {
        "Check for high background upload activity from sync clients before applying latency tweaks.",
        "Prefer wired Ethernet for competitive gaming sessions.",
        "Flush DNS only when name resolution is misbehaving, not as a routine optimization.",
    };
}

bool NetworkOptimizer::flushDns() const {
    DWORD exitCode = 1;
    return runProcessHidden(L"ipconfig /flushdns", &exitCode) && exitCode == 0;
}

bool NetworkOptimizer::backupNetworkSettings() const {
    std::filesystem::path root = "data/network_backups";
    if (const char *appData = std::getenv("APPDATA"); appData != nullptr && *appData != '\0') {
        root = std::filesystem::path(appData) / "PulseBoostAI" / "network_backups";
    }
    std::error_code error;
    std::filesystem::create_directories(root, error);
    if (error) {
        return false;
    }

    const auto backupPath = root / "latest-network-backup.json";
    std::ofstream output(backupPath, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << "{"
           << "\"createdBy\":\"PulseBoostAI\","
           << "\"note\":\"Use revertNetworkSettings to restore PulseBoost-managed netsh defaults.\","
           << "\"revertCommands\":["
           << "\"netsh int tcp set global autotuninglevel=normal\","
           << "\"netsh int tcp set global rss=default\","
           << "\"netsh int tcp set global netdma=default\""
           << "]"
           << "}\n";
    return output.good();
}

bool NetworkOptimizer::optimizeTcp(bool advancedMode) const {
    if (!advancedMode || !backupNetworkSettings()) {
        return false;
    }

    DWORD e1 = 1, e2 = 1, e3 = 1;
    const bool ok1 = runProcessHidden(L"netsh int tcp set global autotuninglevel=normal", &e1) && e1 == 0;
    const bool ok2 = runProcessHidden(L"netsh int tcp set global rss=enabled", &e2) && e2 == 0;
    const bool ok3 = runProcessHidden(L"netsh int tcp set global netdma=enabled", &e3) && e3 == 0;
    return ok1 && ok2 && ok3;
}

bool NetworkOptimizer::resetAdapter(bool advancedMode) const {
    if (!advancedMode || !backupNetworkSettings()) {
        return false;
    }

    DWORD exitCode = 1;
    return runProcessHidden(L"netsh winsock reset", &exitCode) && exitCode == 0;
}

bool NetworkOptimizer::revertNetworkSettings(bool advancedMode) const {
    if (!advancedMode) {
        return false;
    }

    DWORD e1 = 1, e2 = 1, e3 = 1;
    const bool ok1 = runProcessHidden(L"netsh int tcp set global autotuninglevel=normal", &e1) && e1 == 0;
    const bool ok2 = runProcessHidden(L"netsh int tcp set global rss=default", &e2) && e2 == 0;
    const bool ok3 = runProcessHidden(L"netsh int tcp set global netdma=default", &e3) && e3 == 0;
    return ok1 && ok2 && ok3;
}

int NetworkOptimizer::measureLatency(const std::string &host) const {
    return probeLatency(host);
}

NetworkDiagnostics NetworkOptimizer::diagnostics() const {
    NetworkDiagnostics result;

    ULONG bufferSize = 16 * 1024;
    std::vector<unsigned char> buffer(bufferSize);
    auto *addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());
    ULONG status = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufferSize);
    if (status == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(bufferSize);
        addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());
        status = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufferSize);
    }

    if (status == NO_ERROR) {
        for (auto *adapter = addresses; adapter != nullptr; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                continue;
            }

            result.connectionType = adapterTypeName(adapter->IfType);
            result.adapterName = fromWide(adapter->FriendlyName != nullptr ? adapter->FriendlyName : L"");
            result.adapterDescription = fromWide(adapter->Description != nullptr ? adapter->Description : L"");
            result.dnsSuffix = fromWide(adapter->DnsSuffix != nullptr ? adapter->DnsSuffix : L"");

            for (auto *dns = adapter->FirstDnsServerAddress; dns != nullptr; dns = dns->Next) {
                char host[NI_MAXHOST] = {};
                const sockaddr *sockaddr = dns->Address.lpSockaddr;
                if (sockaddr == nullptr) {
                    continue;
                }
                const int nameInfo = getnameinfo(
                    sockaddr,
                    static_cast<socklen_t>(dns->Address.iSockaddrLength),
                    host,
                    static_cast<DWORD>(sizeof(host)),
                    nullptr,
                    0,
                    NI_NUMERICHOST);
                if (nameInfo == 0) {
                    result.dnsServers.emplace_back(host);
                }
            }
            break;
        }
    }

    if (result.adapterName.empty()) {
        result.connectionType = "Disconnected";
    }

    const std::vector<std::pair<std::string, std::string>> endpoints = {
        {"Cloudflare", "1.1.1.1"},
        {"Steam", "store.steampowered.com"},
        {"Xbox Live", "xboxlive.com"},
        {"PSN", "playstation.com"},
        {"Discord", "discord.com"},
    };

    for (const auto &endpoint : endpoints) {
        const int latency = probeLatency(endpoint.second);
        result.probes.push_back(NetworkProbeResult {
            .name = endpoint.first,
            .host = endpoint.second,
            .latencyMs = latency,
            .status = probeStatus(latency),
        });
    }

    return result;
}

}  // namespace pulseboost
