#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <deque>
#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

/// Thread-safe ring buffer storing recent SystemSnapshot samples.
class TelemetryCache {
public:
    explicit TelemetryCache(int maxEntries = 300);

    void push(const SystemSnapshot &snap);
    std::vector<SystemSnapshot> history(int n = 60) const;
    SystemSnapshot latest() const;
    QVariantList chartSeries(int n = 60) const;

private:
    mutable QMutex        mutex_;
    std::deque<SystemSnapshot> buffer_;
    int                   maxEntries_;
};

}  // namespace pulseboost
