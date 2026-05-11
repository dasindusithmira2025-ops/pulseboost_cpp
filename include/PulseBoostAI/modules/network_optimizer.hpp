#pragma once

#include <string>
#include <vector>

namespace pulseboost {

struct NetworkProbeResult {
    std::string name;
    std::string host;
    int latencyMs = -1;
    std::string status = "unreachable";
};

struct NetworkDiagnostics {
    std::string connectionType = "Disconnected";
    std::string adapterName;
    std::string adapterDescription;
    std::string dnsSuffix;
    std::vector<std::string> dnsServers;
    std::vector<NetworkProbeResult> probes;
};

class NetworkOptimizer {
public:
    std::vector<std::string> analyze() const;

    bool flushDns() const;
    bool optimizeTcp(bool advancedMode = false) const;
    bool resetAdapter(bool advancedMode = false) const;
    bool backupNetworkSettings() const;
    bool revertNetworkSettings(bool advancedMode = false) const;

    int measureLatency(const std::string &host = "8.8.8.8") const;
    NetworkDiagnostics diagnostics() const;
};

}  // namespace pulseboost
