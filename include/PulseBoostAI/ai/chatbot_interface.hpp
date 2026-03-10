#pragma once

#include <string>

#include "PulseBoostAI/ai/ai_diagnostics_engine.hpp"
#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/modules/developer_mode.hpp"
#include "PulseBoostAI/modules/game_mode.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"

namespace pulseboost {

class ChatbotInterface {
public:
    ChatbotInterface(SystemScanner &scanner,
                     AiDiagnosticsEngine &diagnosticsEngine,
                     JunkCleaner &junkCleaner,
                     StartupOptimizer &startupOptimizer,
                     GameMode &gameMode,
                     DeveloperMode &developerMode,
                     OptimizationHistory &optimizationHistory);

    ChatResponse handleMessage(const std::string &message);

private:
    SystemScanner &scanner_;
    AiDiagnosticsEngine &diagnosticsEngine_;
    JunkCleaner &junkCleaner_;
    StartupOptimizer &startupOptimizer_;
    GameMode &gameMode_;
    DeveloperMode &developerMode_;
    OptimizationHistory &optimizationHistory_;
};

}  // namespace pulseboost
