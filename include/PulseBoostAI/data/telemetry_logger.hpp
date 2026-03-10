#pragma once

#include <filesystem>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class TelemetryLogger {
public:
    explicit TelemetryLogger(std::filesystem::path outputDirectory = "logs");

    void append(const SystemSnapshot &snapshot);
    std::vector<SystemSnapshot> loadRecent(std::size_t limit = 100) const;

private:
    std::filesystem::path outputDirectory_;
    std::filesystem::path telemetryFile_;
};

}  // namespace pulseboost
