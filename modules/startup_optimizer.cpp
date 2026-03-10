#include "PulseBoostAI/core/registry_optimizer.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"

#include <iostream>

namespace pulseboost::modules::startup_optimizer {

void run() {
    RegistryOptimizer registryOptimizer;
    StartupOptimizer optimizer(registryOptimizer);
    const auto items = optimizer.scanStartupItems();
    std::cout << "Startup items detected: " << items.size() << '\n';
}

}  // namespace pulseboost::modules::startup_optimizer
