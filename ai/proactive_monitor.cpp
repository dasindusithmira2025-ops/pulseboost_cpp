#include "PulseBoostAI/ai/proactive_monitor.hpp"

namespace pulseboost {

ProactiveMonitor::ProactiveMonitor(QObject *parent) : QObject(parent) {
    cooldownTimer_.setInterval(300000);
    cooldownTimer_.setSingleShot(true);
}

void ProactiveMonitor::feedSnapshot(const SystemSnapshot &snapshot) {
    if (cooldownTimer_.isActive()) {
        return;
    }

    buffer_.push_back(snapshot);
    if (buffer_.size() > 10) {
        buffer_.erase(buffer_.begin());
    }

    if (buffer_.size() < 10) {
        return;
    }

    int highCpu = 0;
    int highRam = 0;
    int highDisk = 0;
    for (const auto &sample : buffer_) {
        if (sample.cpuUsagePercent > 85.0) {
            highCpu++;
        }
        if (sample.ramUsagePercent > 90.0) {
            highRam++;
        }
        if (sample.diskUsagePercent > 90.0) {
            highDisk++;
        }
    }

    if (highCpu >= 8 || highRam >= 8 || highDisk >= 8) {
        emit aiAlertGenerated(buildLocalAlert(buffer_.back()));
        cooldownTimer_.start();
    }
}

QString ProactiveMonitor::buildLocalAlert(const SystemSnapshot &latest) const {
    if (latest.diskUsagePercent >= 90.0) {
        return QString("PulseModel detected sustained storage pressure (%1% disk). Run Safe Cleanup now.")
            .arg(QString::number(latest.diskUsagePercent, 'f', 1));
    }
    if (latest.ramUsagePercent >= 90.0) {
        return QString("PulseModel detected sustained RAM pressure (%1% RAM). Run RAM optimization now.")
            .arg(QString::number(latest.ramUsagePercent, 'f', 1));
    }
    return QString("PulseModel detected sustained CPU pressure (%1% CPU). Consider process optimization.")
        .arg(QString::number(latest.cpuUsagePercent, 'f', 1));
}

}  // namespace pulseboost

