#include "PulseBoostAI/data/telemetry_logger.hpp"

#include <fstream>
#include <sstream>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

TelemetryLogger::TelemetryLogger(std::filesystem::path outputDirectory)
    : outputDirectory_(std::move(outputDirectory)), telemetryFile_(outputDirectory_ / "telemetry.csv") {
    std::error_code error;
    std::filesystem::create_directories(outputDirectory_, error);
    if (!std::filesystem::exists(telemetryFile_)) {
        std::ofstream output(telemetryFile_);
        output << "timestamp,cpu,ram,disk,network,health\n";
    }
}

void TelemetryLogger::append(const SystemSnapshot &snapshot) {
    std::ofstream output(telemetryFile_, std::ios::app);
    output << currentTimestampUtc() << ',' << snapshot.cpuUsagePercent << ',' << snapshot.ramUsagePercent << ','
           << snapshot.diskUsagePercent << ',' << snapshot.networkMbps << ',' << snapshot.healthScore << '\n';
}

std::vector<SystemSnapshot> TelemetryLogger::loadRecent(std::size_t limit) const {
    std::vector<SystemSnapshot> snapshots;
    std::ifstream input(telemetryFile_);
    std::string line;
    std::getline(input, line);
    while (std::getline(input, line)) {
        std::stringstream stream(line);
        std::string token;
        SystemSnapshot snapshot;
        try {
            std::getline(stream, token, ',');
            std::getline(stream, token, ',');
            snapshot.cpuUsagePercent = std::stod(token);
            std::getline(stream, token, ',');
            snapshot.ramUsagePercent = std::stod(token);
            std::getline(stream, token, ',');
            snapshot.diskUsagePercent = std::stod(token);
            std::getline(stream, token, ',');
            snapshot.networkMbps = std::stod(token);
            std::getline(stream, token, ',');
            snapshot.healthScore = std::stoi(token);
            snapshots.push_back(snapshot);
        } catch (...) {
            // Skip malformed rows so a single bad sample does not break history loading.
        }
    }

    if (snapshots.size() > limit) {
        snapshots.erase(snapshots.begin(), snapshots.end() - static_cast<std::ptrdiff_t>(limit));
    }
    return snapshots;
}

}  // namespace pulseboost
