#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"

namespace pulseboost {

// ---------------------------------------------------------------------------
// Worker object that lives on a background QThread.
// Emits snapshotReady() via a queued connection – the UI thread is never
// touched by the Windows PDH / WMI calls.
// ---------------------------------------------------------------------------
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
    void doScan(bool includeDisk, bool includeProcesses);

    SystemScanner &scanner_;
    QTimer *cpuRamTimer_   = nullptr;   // 1 s
    QTimer *diskTimer_     = nullptr;   // 3 s
    QTimer *processTimer_  = nullptr;   // 5 s

    // Cached snapshot sections so partial updates still emit a full snapshot
    pulseboost::SystemSnapshot latestSnapshot_;
};

// ---------------------------------------------------------------------------
// Thin owner that creates the thread and the worker.
// ---------------------------------------------------------------------------
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
    QThread        workerThread_;
    TelemetryWorker *worker_ = nullptr;
};

}  // namespace pulseboost
