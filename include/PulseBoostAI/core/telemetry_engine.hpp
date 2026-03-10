#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"

namespace pulseboost {

class TelemetryWorker : public QObject {
    Q_OBJECT
public:
    explicit TelemetryWorker(SystemScanner &scanner, QObject *parent = nullptr);

public slots:
    void start();
    void stop();

signals:
    void snapshotReady(pulseboost::SystemSnapshot snapshot);

private slots:
    void onCpuRamTick();
    void onDiskTick();
    void onProcessTick();

private:
    SystemScanner       &scanner_;
    QTimer              *cpuRamTimer_  = nullptr;
    QTimer              *diskTimer_    = nullptr;
    QTimer              *processTimer_ = nullptr;
    pulseboost::SystemSnapshot latestSnapshot_;
};

class TelemetryEngine : public QObject {
    Q_OBJECT
public:
    explicit TelemetryEngine(SystemScanner &scanner, QObject *parent = nullptr);
    ~TelemetryEngine();

    void start();
    void stop();

signals:
    void snapshotReady(pulseboost::SystemSnapshot snapshot);

private:
    QThread          workerThread_;
    TelemetryWorker *worker_ = nullptr;
};

}  // namespace pulseboost
