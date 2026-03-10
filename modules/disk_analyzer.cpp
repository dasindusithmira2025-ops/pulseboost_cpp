#include "PulseBoostAI/core/disk_analyzer.hpp"

#include <iostream>

namespace pulseboost::modules::disk_analyzer {

void run() {
    DiskAnalyzer analyzer;
    const auto summary = analyzer.analyzeSystemDrive();
    std::cout << "Disk usage: " << summary.usedPercent << "%\n";
}

}  // namespace pulseboost::modules::disk_analyzer
