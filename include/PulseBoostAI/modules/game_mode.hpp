#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/core/service_manager.hpp"

namespace pulseboost {

class GameMode {
public:
    GameMode(ProcessManager &processManager, ServiceManager &serviceManager);

    bool enableForProcess(std::uint32_t gamePid);
    void disable();
    bool isActive() const;

private:
    ProcessManager &processManager_;
    ServiceManager &serviceManager_;
    bool active_ = false;
    std::unordered_map<std::uint32_t, DWORD> previousPriorities_;
};

}  // namespace pulseboost
