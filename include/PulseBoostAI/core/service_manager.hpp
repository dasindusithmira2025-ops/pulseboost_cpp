#pragma once

#include <string>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class ServiceManager {
public:
    std::vector<ServiceInfo> enumerateServices(bool runningOnly = false) const;
    bool stopService(const std::string &serviceName) const;
    bool startService(const std::string &serviceName) const;
};

}  // namespace pulseboost
