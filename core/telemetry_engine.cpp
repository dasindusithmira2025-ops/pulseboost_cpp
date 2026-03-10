#include "PulseBoostAI/core/telemetry_engine.hpp"

#include <QTimer>

namespace pulseboost {

namespace {

SystemSnapshot mergeSnapshot(const SystemSnapshot &base, const SystemSnapshot &delta) {
    SystemSnapshot merged = base;

    merged.capturedAt = delta.capturedAt;
    merged.cpuUsagePercent = delta.cpuUsagePercent;
    merged.ramUsagePercent = delta.ramUsagePercent;
    merged.ramUsedMb = delta.ramUsedMb;
    merged.ramTotalMb = delta.ramTotalMb;
    merged.gpuUsagePercent = delta.gpuUsagePercent;
    merged.networkMbps = delta.networkMbps;

    if (!delta.heavyProcesses.empty()) {
        merged.heavyProcesses = delta.heavyProcesses;
    }
    if (!delta.startupItems.empty() || delta.startupPrograms > 0) {
        merged.startupItems = delta.startupItems;
        merged.startupPrograms = delta.startupPrograms;
    }
    if (!delta.services.empty() || delta.runningServices > 0) {
        merged.services = delta.services;
        merged.runningServices = delta.runningServices;
    }
    if (!delta.storageCategories.empty() || delta.diskTotalGb > 0.0) {
        merged.diskUsagePercent = delta.diskUsagePercent;
        merged.diskUsedGb = delta.diskUsedGb;
        merged.diskTotalGb = delta.diskTotalGb;
        merged.diskReadMbps = delta.diskReadMbps;
        merged.diskWriteMbps = delta.diskWriteMbps;
        merged.storageCategories = delta.storageCategories;
    }
    if (!delta.drivers.empty()) {
        merged.drivers = delta.drivers;
    }

    merged.issues = delta.issues;
    merged.healthScore = delta.healthScore;
    merged.summary = delta.summary;
    return merged;
}

}  // namespace

TelemetryWorker::TelemetryWorker(SystemScanner &scanner, QObject *parent)
    : QObject(parent), scanner_(scanner) {}

void TelemetryWorker::start() {
    cpuRamTimer_ = new QTimer(this);
    diskTimer_   = new QTimer(this);
    processTimer_= new QTimer(this);

    connect(cpuRamTimer_,  &QTimer::timeout, this, &TelemetryWorker::onCpuRamTick);
    connect(diskTimer_,    &QTimer::timeout, this, &TelemetryWorker::onDiskTick);
    connect(processTimer_, &QTimer::timeout, this, &TelemetryWorker::onProcessTick);

    cpuRamTimer_->start(1000);
    diskTimer_->start(3000);
    processTimer_->start(5000);

    latestSnapshot_ = scanner_.scan();
    emit snapshotReady(latestSnapshot_);
}

void TelemetryWorker::stop() {
    if (cpuRamTimer_)  cpuRamTimer_->stop();
    if (diskTimer_)    diskTimer_->stop();
    if (processTimer_) processTimer_->stop();
}

void TelemetryWorker::onCpuRamTick() {
    auto delta = scanner_.scanVitals();
    scanner_.finalizeSnapshot(delta);
    latestSnapshot_ = mergeSnapshot(latestSnapshot_, delta);
    scanner_.finalizeSnapshot(latestSnapshot_);
    emit snapshotReady(latestSnapshot_);
}

void TelemetryWorker::onDiskTick() {
    SystemSnapshot delta;
    scanner_.enrichStorage(delta);
    scanner_.finalizeSnapshot(delta);
    latestSnapshot_ = mergeSnapshot(latestSnapshot_, delta);
    scanner_.finalizeSnapshot(latestSnapshot_);
    emit snapshotReady(latestSnapshot_);
}

void TelemetryWorker::onProcessTick() {
    SystemSnapshot delta;
    delta.capturedAt = std::chrono::system_clock::now();
    delta.heavyProcesses = scanner_.scanVitals().heavyProcesses;
    scanner_.enrichProcesses(delta);
    scanner_.finalizeSnapshot(delta);
    latestSnapshot_ = mergeSnapshot(latestSnapshot_, delta);
    scanner_.finalizeSnapshot(latestSnapshot_);
    emit snapshotReady(latestSnapshot_);
}

// ---------------------------------------------------------------------------
// TelemetryEngine
// ---------------------------------------------------------------------------

TelemetryEngine::TelemetryEngine(SystemScanner &scanner, QObject *parent)
    : QObject(parent) {
    worker_ = new TelemetryWorker(scanner);
    worker_->moveToThread(&workerThread_);

    connect(&workerThread_, &QThread::started, worker_, &TelemetryWorker::start);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

    // Forward signal from worker to this engine (queued across thread boundary)
    connect(worker_, &TelemetryWorker::snapshotReady,
            this,    &TelemetryEngine::snapshotReady,
            Qt::QueuedConnection);
}

TelemetryEngine::~TelemetryEngine() {
    stop();
}

void TelemetryEngine::start() {
    workerThread_.start();
}

void TelemetryEngine::stop() {
    if (workerThread_.isRunning()) {
        QMetaObject::invokeMethod(worker_, "stop", Qt::QueuedConnection);
        workerThread_.quit();
        workerThread_.wait(3000);
    }
}

}  // namespace pulseboost
