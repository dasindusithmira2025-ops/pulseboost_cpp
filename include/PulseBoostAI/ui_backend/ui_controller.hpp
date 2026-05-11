#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "PulseBoostAI/common/models.hpp"
#include "PulseBoostAI/core/disk_analyzer.hpp"
#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/data/telemetry_cache.hpp"
#include "PulseBoostAI/core/game_optimizer.hpp"
#include "PulseBoostAI/core/pulse_bench.hpp"
#include "PulseBoostAI/core/system_advisor.hpp"
#include "PulseBoostAI/modules/defrag_trigger.hpp"
#include "PulseBoostAI/modules/game_mode.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"
#include "PulseBoostAI/modules/network_optimizer.hpp"
#include "PulseBoostAI/modules/ram_optimizer.hpp"
#include "PulseBoostAI/modules/safety_guard.hpp"
#include "PulseBoostAI/modules/tweak_engine.hpp"

namespace pulseboost {

class UiController : public QObject {
    Q_OBJECT

    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY metricsChanged)
    Q_PROPERTY(double ramUsage READ ramUsage NOTIFY metricsChanged)
    Q_PROPERTY(double diskUsage READ diskUsage NOTIFY metricsChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY metricsChanged)
    Q_PROPERTY(double networkMbps READ networkMbps NOTIFY metricsChanged)
    Q_PROPERTY(int healthScore READ healthScore NOTIFY metricsChanged)
    Q_PROPERTY(int cpuScore READ cpuScore NOTIFY metricsChanged)
    Q_PROPERTY(int memoryScore READ memoryScore NOTIFY metricsChanged)
    Q_PROPERTY(int diskScore READ diskScore NOTIFY metricsChanged)
    Q_PROPERTY(int securityScore READ securityScore NOTIFY metricsChanged)
    Q_PROPERTY(double forecastHealth24h READ forecastHealth24h NOTIFY metricsChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY metricsChanged)
    Q_PROPERTY(int startupCount READ startupCount NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList chartSeries READ chartSeries NOTIFY chartSeriesChanged)
    Q_PROPERTY(QVariantList thermalSeries READ thermalSeries NOTIFY chartSeriesChanged)
    Q_PROPERTY(QVariantList processList READ processList NOTIFY processListChanged)
    Q_PROPERTY(QVariantList driverList READ driverList NOTIFY metricsChanged)
    Q_PROPERTY(QVariantMap driverSummary READ driverSummary NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList storageCategories READ storageCategories NOTIFY storageChanged)
    Q_PROPERTY(QVariantList recentActions READ recentActions NOTIFY actionsChanged)
    Q_PROPERTY(QString healthLabel READ healthLabel NOTIFY metricsChanged)
    Q_PROPERTY(double savedTodayMb READ savedTodayMb NOTIFY actionsChanged)
    Q_PROPERTY(QVariantMap networkOverview READ networkOverview NOTIFY metricsChanged)
    Q_PROPERTY(QVariantMap memoryOverview READ memoryOverview NOTIFY metricsChanged)
    Q_PROPERTY(QVariantMap thermalOverview READ thermalOverview NOTIFY metricsChanged)
    Q_PROPERTY(double recoverableRamMb READ recoverableRamMb NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList notifications READ notifications NOTIFY notificationCenterChanged)
    Q_PROPERTY(bool gameModeActive READ gameModeActive NOTIFY gameModeChanged)
    Q_PROPERTY(QVariantMap gameModeStatus READ gameModeStatus NOTIFY gameModeChanged)
    Q_PROPERTY(QVariantList systemSnapshots READ systemSnapshots NOTIFY snapshotsChanged)
    Q_PROPERTY(QVariantMap pulseScore READ pulseScore NOTIFY metricsChanged)
    Q_PROPERTY(QVariantMap latestBenchmarkDelta READ latestBenchmarkDelta NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList advisorItems READ advisorItems NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList optimizationPresets READ optimizationPresets CONSTANT)
    Q_PROPERTY(QVariantList detectedGames READ detectedGames NOTIFY gameModeChanged)
    Q_PROPERTY(QString aiMode READ aiMode NOTIFY uiStateChanged)
    Q_PROPERTY(bool aiCloudConfigured READ aiCloudConfigured NOTIFY uiStateChanged)
    Q_PROPERTY(bool uiDataReady READ uiDataReady NOTIFY uiStateChanged)
    Q_PROPERTY(QString uiErrorMessage READ uiErrorMessage NOTIFY uiStateChanged)
    Q_PROPERTY(int telemetryAgeMs READ telemetryAgeMs NOTIFY uiStateChanged)

public:
    explicit UiController(JunkCleaner &junkCleaner,
                          GameMode &gameMode,
                          ProcessManager &processManager,
                          DiskAnalyzer &diskAnalyzer,
                          StartupOptimizer &startupOptimizer,
                          SafetyGuard &safetyGuard,
                          TelemetryCache &telemetryCache,
                          OptimizationHistory &history,
                          NetworkOptimizer &networkOptimizer,
                          TweakEngine &tweakEngine,
                          GameOptimizer &gameOptimizer,
                          SystemAdvisor &systemAdvisor,
                          PulseBench &pulseBench,
                          QObject *parent = nullptr);

    double cpuUsage() const { return snapshot_.cpuUsagePercent; }
    double ramUsage() const { return snapshot_.ramUsagePercent; }
    double diskUsage() const { return snapshot_.diskUsagePercent; }
    double gpuUsage() const { return snapshot_.gpuUsagePercent; }
    double networkMbps() const { return snapshot_.networkMbps; }
    int healthScore() const { return snapshot_.healthScore; }
    int cpuScore() const { return snapshot_.cpuEfficiencyScore; }
    int memoryScore() const { return snapshot_.memoryPressureScore; }
    int diskScore() const { return snapshot_.diskHealthScore; }
    int securityScore() const { return snapshot_.securityScore; }
    double forecastHealth24h() const { return snapshot_.healthForecast24h; }
    QString summary() const { return QString::fromStdString(snapshot_.summary); }
    int startupCount() const { return snapshot_.startupPrograms; }
    bool gameModeActive() const { return gameMode_.isActive(); }

    QString healthLabel() const;
    QVariantList chartSeries() const;
    QVariantList thermalSeries() const;
    QVariantList processList() const;
    QVariantList driverList() const;
    QVariantMap driverSummary() const;
    QVariantList storageCategories() const;
    QVariantList recentActions() const;
    double savedTodayMb() const;
    QVariantMap networkOverview() const;
    QVariantMap memoryOverview() const;
    QVariantMap thermalOverview() const;
    double recoverableRamMb() const;
    QVariantList notifications() const;
    QVariantMap gameModeStatus() const;
    QVariantList systemSnapshots() const;
    QVariantMap pulseScore() const;
    QVariantMap latestBenchmarkDelta() const;
    QVariantList advisorItems() const;
    QVariantList optimizationPresets() const;
    QVariantList detectedGames() const;
    QString aiMode() const;
    bool aiCloudConfigured() const;
    bool uiDataReady() const { return uiDataReady_; }
    QString uiErrorMessage() const { return uiErrorMessage_; }
    int telemetryAgeMs() const;

    void onSnapshotReady(const SystemSnapshot &snapshot);

    Q_INVOKABLE void runClean();
    Q_INVOKABLE void runOptimize();
    Q_INVOKABLE void enableGameMode();
    Q_INVOKABLE void disableGameMode();
    Q_INVOKABLE void killProcess(int pid);
    Q_INVOKABLE bool suspendProcess(int pid);
    Q_INVOKABLE void createRestorePoint();
    Q_INVOKABLE QVariantList sortedProcesses(const QString &sortKey, bool descending) const;
    Q_INVOKABLE QVariantList storageTreemap(int width, int height) const;
    Q_INVOKABLE QVariantList findLargeFiles();
    Q_INVOKABLE bool flushDns();
    Q_INVOKABLE bool optimizeTcp();
    Q_INVOKABLE bool optimizeRam();
    Q_INVOKABLE bool optimizeDisk();
    Q_INVOKABLE int checkLatency() const;
    Q_INVOKABLE QVariantList fetchStartupItems() const;
    Q_INVOKABLE bool disableStartupItem(const QString &name, const QString &location, const QString &command);
    Q_INVOKABLE bool delayStartupItem(const QString &name, const QString &location, const QString &command, int delaySeconds);
    Q_INVOKABLE QString exportChartSeriesCsv();
    Q_INVOKABLE bool takeSystemSnapshot();
    Q_INVOKABLE bool restoreSystemSnapshot(const QString &snapshotId);
    Q_INVOKABLE QVariantList getScheduledOptimizations() const;
    Q_INVOKABLE bool setScheduledOptimization(const QString &taskId, bool enabled, const QString &type, int intervalHours);
    Q_INVOKABLE QVariantList listTweaks() const;
    Q_INVOKABLE bool applyTweak(const QString &id);
    Q_INVOKABLE bool revertTweak(const QString &id);
    Q_INVOKABLE bool applyOptimizationPreset(const QString &presetId);
    Q_INVOKABLE bool optimizeDetectedGame(const QString &query);
    Q_INVOKABLE bool launchOptimizedGame(const QString &query);
    Q_INVOKABLE bool revertGameOptimization();
    Q_INVOKABLE bool setAiPreferences(const QString &mode, const QString &apiKey);
    Q_INVOKABLE void refreshAll();

signals:
    void metricsChanged();
    void chartSeriesChanged();
    void processListChanged();
    void storageChanged();
    void actionsChanged();
    void notificationCenterChanged();
    void gameModeChanged();
    void snapshotsChanged();
    void actionFeedback(QString message, bool success);
    void uiStateChanged();

private:
    void appendAction(const std::string &action, const std::string &details, bool success);
    QVariantMap storageNodeToMap(const StorageCategory &category,
                                 double x,
                                 double y,
                                 double width,
                                 double height,
                                 double fraction) const;
    void refreshDetectedGame();
    void recomputeUiCaches();

    SystemSnapshot snapshot_;
    JunkCleaner &junkCleaner_;
    GameMode &gameMode_;
    ProcessManager &processManager_;
    DiskAnalyzer &diskAnalyzer_;
    StartupOptimizer &startupOptimizer_;
    SafetyGuard &safetyGuard_;
    TelemetryCache &telemetryCache_;
    OptimizationHistory &history_;
    NetworkOptimizer &networkOptimizer_;
    TweakEngine &tweakEngine_;
    GameOptimizer &gameOptimizer_;
    SystemAdvisor &systemAdvisor_;
    PulseBench &pulseBench_;
    QString detectedGameName_;
    int detectedGamePid_ = 0;
    double gameModeBoostEstimate_ = 0.0;
    double gameModeRamFreedMb_ = 0.0;
    int gameModeLatencyBefore_ = -1;
    int gameModeLatencyAfter_ = -1;
    bool uiDataReady_ = false;
    QString uiErrorMessage_;
    qint64 lastSnapshotEpochMs_ = 0;
    qint64 lastLatencyCheckMs_ = 0;
    int cachedLatencyMs_ = -1;
    QVariantList cachedRecentActions_;
    QVariantMap cachedNetworkOverview_;
    QVariantMap cachedMemoryOverview_;
    QVariantMap cachedThermalOverview_;
    QVariantList cachedNotifications_;
    QVariantMap cachedPulseScore_;
    QVariantMap cachedBenchmarkDelta_;
    QVariantList cachedAdvisorItems_;
    QVariantList cachedDetectedGames_;
    QVariantList cachedSystemSnapshots_;
    double cachedRecoverableRamMb_ = 0.0;
};

}  // namespace pulseboost

