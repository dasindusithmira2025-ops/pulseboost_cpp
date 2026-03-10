#include "PulseBoostAI/core/process_manager.hpp"

#include <iostream>

namespace pulseboost::modules::process_optimizer {

void run() {
    ProcessManager processManager;
    const auto processes = processManager.enumerateProcesses();
    std::cout << "Processes enumerated: " << processes.size() << '\n';
}

}  // namespace pulseboost::modules::process_optimizer
