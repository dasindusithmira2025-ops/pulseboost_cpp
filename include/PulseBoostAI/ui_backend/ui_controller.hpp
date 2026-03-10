#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/data/telemetry_cache.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"
#include "PulseBoostAI/modules/game_mode.hpp"
#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/modules/safety_guard.hpp"

namespace pulseboost {

/// Singleton object exposed as "SystemCtrl" to QML.
/// Bridges the threaded TelemetryEngine with the QML UI.
class UiController : public QObject {
    Q_OBJECT

    Q_PROPERTY(double   cpuUsage        READ cpuUsage        NOTIFY metricsChanged)
    Q_PROPERTY(double   ramUsage        READ ramUsage        NOTIFY metricsChanged)
    Q_PROPERTY(double   diskUsage       READ diskUsage       NOTIFY metricsChanged)
    Q_PROPERTY(double   gpuUsage        READ gpuUsage        NOTIFY metricsChanged)
    Q_PROPERTY(double   networkMbps     READ networkMbps     NOTIFY metricsChanged)
    Q_PROPERTY(int      healthScore     READ healthScore     NOTIFY metricsChanged)
    Q_PROPERTY(QString  summary         READ summary         NOTIFY metricsChanged)
    Q_PROPERTY(int      startupCount    READ startupCount    NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList chartSeries READ chartSeries NOTIFY chartSeriesChanged)
    Q_PROPERTY(QVariantList  processList      READ processList     NOTIFY processListChanged)
    Q_PROPERTY(QVariantList  storageCategories READ storageCategories NOTIFY storageChanged)
    Q_PROPERTY(QVariantList  recentActions READ recentActions NOTIFY actionsChanged)
    Q_PROPERTY(QString  healthLabel READ healthLabel NOTIFY metricsChanged)

public:
    explicit UiController(JunkCleaner       &junkCleaner,
                          GameMode          &gameMode,
                          ProcessManager    &processManager,
                          SafetyGuard       &safetyGuard,
                          TelemetryCache    &telemetryCache,
                          OptimizationHistory &history,
                          QObject           *parent = nullptr);

    // Property accessors
    double       cpuUsage()         const { return snapshot_.cpuUsagePercent; }
    double       ramUsage()         const { return snapshot_.ramUsagePercent; }
    double       diskUsage()        const { return snapshot_.diskUsagePercent; }
    double       gpuUsage()         const { return snapshot_.gpuUsagePercent; }
    double       networkMbps()      const { return snapshot_.networkMbps; }
    int          healthScore()      const { return snapshot_.healthScore; }
    QString      summary()          const { return QString::fromStdString(snapshot_.summary); }
    int          startupCount()     const { return snapshot_.startupPrograms; }
    QString      healthLabel()      const;
    QVariantList chartSeries()      const;
    QVariantList processList()      const;
    QVariantList storageCategories() const;
    QVariantList recentActions()    const;

    // Called by main.cpp to push new telemetry snapshots
    void onSnapshotReady(const SystemSnapshot &snapshot);

    // QML-callable actions
    Q_INVOKABLE void runClean();
    Q_INVOKABLE void runOptimize();
    Q_INVOKABLE void enableGameMode();
    Q_INVOKABLE void killProcess(int pid);
    Q_INVOKABLE void createRestorePoint();
    Q_INVOKABLE QVariantList sortedProcesses(const QString &sortKey, bool descending) const;
    Q_INVOKABLE QVariantList storageTreemap(int width, int height) const;

signals:
    void metricsChanged();
    void chartSeriesChanged();
    void processListChanged();
    void storageChanged();
    void actionsChanged();
    void actionFeedback(QString message, bool success);

private:
    void appendAction(const std::string &action, const std::string &details, bool success);
    QVariantMap storageNodeToMap(const StorageCategory &category,
                                 double x,
                                 double y,
                                 double width,
                                 double height,
                                 double fraction) const;

    SystemSnapshot  snapshot_;
    JunkCleaner    &junkCleaner_;
    GameMode       &gameMode_;
    ProcessManager &processManager_;
    SafetyGuard    &safetyGuard_;
    TelemetryCache &telemetryCache_;
    OptimizationHistory &history_;
};

}  // namespace pulseboost
