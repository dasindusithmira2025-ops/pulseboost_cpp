#pragma once

#include <filesystem>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class OptimizationHistory {
public:
    explicit OptimizationHistory(std::filesystem::path outputDirectory = "logs");

    void record(const ActionRecord &record);
    std::vector<ActionRecord> load() const;

private:
    std::filesystem::path historyFile_;
};

}  // namespace pulseboost
