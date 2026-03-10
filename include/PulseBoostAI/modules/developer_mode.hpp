#pragma once

#include <string>
#include <vector>

#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/core/service_manager.hpp"

namespace pulseboost {

class DeveloperMode {
public:
    DeveloperMode(ProcessManager &processManager, ServiceManager &serviceManager);

    std::vector<std::string> optimizeFor(const std::string &profile) const;

private:
    ProcessManager &processManager_;
    ServiceManager &serviceManager_;
};

}  // namespace pulseboost
