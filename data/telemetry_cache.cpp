#include "PulseBoostAI/data/telemetry_cache.hpp"

#include <QVariantMap>

namespace pulseboost {

TelemetryCache::TelemetryCache(int maxEntries) : maxEntries_(maxEntries) {}

void TelemetryCache::push(const SystemSnapshot &snap) {
    QMutexLocker lock(&mutex_);
    buffer_.push_back(snap);
    while (static_cast<int>(buffer_.size()) > maxEntries_) {
        buffer_.pop_front();
    }
}

std::vector<SystemSnapshot> TelemetryCache::history(int n) const {
    QMutexLocker lock(&mutex_);
    const int count = std::min(n, static_cast<int>(buffer_.size()));
    return std::vector<SystemSnapshot>(buffer_.end() - count, buffer_.end());
}

SystemSnapshot TelemetryCache::latest() const {
    QMutexLocker lock(&mutex_);
    if (buffer_.empty()) return {};
    return buffer_.back();
}

QVariantList TelemetryCache::chartSeries(int n) const {
    QMutexLocker lock(&mutex_);
    QVariantList points;
    const int count = std::min(n, static_cast<int>(buffer_.size()));
    if (count == 0) {
        return points;
    }

    const auto start = buffer_.end() - count;
    int index = 0;
    for (auto iterator = start; iterator != buffer_.end(); ++iterator, ++index) {
        QVariantMap point;
        point["index"] = index;
        point["cpu"] = iterator->cpuUsagePercent;
        point["ram"] = iterator->ramUsagePercent;
        point["disk"] = iterator->diskUsagePercent;
        point["gpu"] = iterator->gpuUsagePercent;
        point["network"] = iterator->networkMbps;
        point["health"] = iterator->healthScore;
        points.push_back(point);
    }
    return points;
}

}  // namespace pulseboost
