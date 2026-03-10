#pragma once

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/process_manager.hpp"

namespace pulseboost {

class MemoryAnalyzer {
public:
    explicit MemoryAnalyzer(ProcessManager &processManager);
    MemorySummary analyze();

private:
    ProcessManager &processManager_;
};

}  // namespace pulseboost
