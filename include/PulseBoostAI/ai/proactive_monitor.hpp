#pragma once

#include <QObject>
#include <QTimer>

#include <vector>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class ProactiveMonitor : public QObject {
    Q_OBJECT
public:
    explicit ProactiveMonitor(QObject *parent = nullptr);

    void feedSnapshot(const SystemSnapshot &snapshot);

signals:
    void aiAlertGenerated(QString alertMessage);

private:
    QString buildLocalAlert(const SystemSnapshot &latest) const;

    QTimer cooldownTimer_;
    std::vector<SystemSnapshot> buffer_;
};

}  // namespace pulseboost

