#include "PulseBoostAI/ai/system_prediction.hpp"

#include <numeric>

namespace pulseboost {

std::vector<std::string> SystemPrediction::predict(const std::vector<SystemSnapshot> &history) const {
    std::vector<std::string> predictions;
    if (history.size() < 5) {
        predictions.push_back("Not enough telemetry yet for stable prediction.");
        return predictions;
    }

    auto averageMetric = [&history](auto projection) {
        double total = 0.0;
        for (const auto &snapshot : history) {
            total += projection(snapshot);
        }
        return total / static_cast<double>(history.size());
    };

    const double averageRam = averageMetric([](const SystemSnapshot &snapshot) { return snapshot.ramUsagePercent; });
    const double averageDisk = averageMetric([](const SystemSnapshot &snapshot) { return snapshot.diskUsagePercent; });
    const double averageHealth = averageMetric([](const SystemSnapshot &snapshot) { return snapshot.healthScore; });

    if (averageRam > 75.0) {
        predictions.push_back("RAM pressure is trending high. Expect build/game slowdowns during multitasking.");
    }
    if (averageDisk > 80.0) {
        predictions.push_back("Disk capacity is trending toward saturation. Schedule storage cleanup soon.");
    }
    if (averageHealth < 70.0) {
        predictions.push_back("Overall health is trending downward. Review startup and background services.");
    }
    if (predictions.empty()) {
        predictions.push_back("No immediate degradation trend detected from recent telemetry.");
    }

    return predictions;
}

}  // namespace pulseboost
