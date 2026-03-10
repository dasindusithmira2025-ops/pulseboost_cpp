#include "PulseBoostAI/core/service_manager.hpp"

#include <iostream>

namespace pulseboost::modules::service_manager {

void run() {
    ServiceManager manager;
    const auto services = manager.enumerateServices(true);
    std::cout << "Running services: " << services.size() << '\n';
}

}  // namespace pulseboost::modules::service_manager
