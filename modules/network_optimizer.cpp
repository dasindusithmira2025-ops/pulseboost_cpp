#include "PulseBoostAI/modules/network_optimizer.hpp"

#include "PulseBoostAI/common/windows_utils.hpp"

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

}  // namespace pulseboost
