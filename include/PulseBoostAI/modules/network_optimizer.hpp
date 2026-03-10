#pragma once

#include <string>
#include <vector>

namespace pulseboost {

class NetworkOptimizer {
public:
    std::vector<std::string> analyze() const;
    bool flushDns() const;
};

}  // namespace pulseboost
