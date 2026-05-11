#pragma once

#include <QObject>
#include <QHash>
#include <QSet>

class QTcpServer;
class QTcpSocket;
class QTimer;

namespace pulseboost {

class ProcessManager;
class ServiceManager;
class RegistryOptimizer;
class StartupOptimizer;
class MemoryAnalyzer;
class DiskAnalyzer;
class SystemScanner;
class TelemetryLogger;
class OptimizationHistory;
class JunkCleaner;
class DuplicateFileFinder;
class LargeFileScanner;
class GameMode;
class NetworkOptimizer;
class SafetyGuard;
class DefragTrigger;
class TweakEngine;
class PulseBench;
class SystemAdvisor;
class GameOptimizer;
class LicenseManager;
class AutoUpdater;

class BackendDaemon : public QObject {
    Q_OBJECT
public:
    BackendDaemon(ProcessManager &processManager,
                  ServiceManager &serviceManager,
                  RegistryOptimizer &registryOptimizer,
                  StartupOptimizer &startupOptimizer,
                  MemoryAnalyzer &memoryAnalyzer,
                  DiskAnalyzer &diskAnalyzer,
                  SystemScanner &scanner,
                  TelemetryLogger &telemetryLogger,
                  OptimizationHistory &optimizationHistory,
                  JunkCleaner &junkCleaner,
                  DuplicateFileFinder &duplicateFileFinder,
                  LargeFileScanner &largeFileScanner,
                  GameMode &gameMode,
                  NetworkOptimizer &networkOptimizer,
                  SafetyGuard &safetyGuard,
                  DefragTrigger &defragTrigger,
                  TweakEngine &tweakEngine,
                  PulseBench &pulseBench,
                  SystemAdvisor &systemAdvisor,
                  GameOptimizer &gameOptimizer,
                  LicenseManager &licenseManager,
                  AutoUpdater &autoUpdater,
                  QObject *parent = nullptr);

    bool listen(quint16 port = 47321);

private:
    void handleNewConnection();
    void handleReadyRead(QTcpSocket *socket);
    void handleSocketDisconnected(QTcpSocket *socket);
    void sendSnapshotEvent();

    ProcessManager &processManager_;
    ServiceManager &serviceManager_;
    RegistryOptimizer &registryOptimizer_;
    StartupOptimizer &startupOptimizer_;
    MemoryAnalyzer &memoryAnalyzer_;
    DiskAnalyzer &diskAnalyzer_;
    SystemScanner &scanner_;
    TelemetryLogger &telemetryLogger_;
    OptimizationHistory &optimizationHistory_;
    JunkCleaner &junkCleaner_;
    DuplicateFileFinder &duplicateFileFinder_;
    LargeFileScanner &largeFileScanner_;
    GameMode &gameMode_;
    NetworkOptimizer &networkOptimizer_;
    SafetyGuard &safetyGuard_;
    DefragTrigger &defragTrigger_;
    TweakEngine &tweakEngine_;
    PulseBench &pulseBench_;
    SystemAdvisor &systemAdvisor_;
    GameOptimizer &gameOptimizer_;
    LicenseManager &licenseManager_;
    AutoUpdater &autoUpdater_;

    QTcpServer *server_ = nullptr;
    QTimer *snapshotTimer_ = nullptr;
    QSet<QTcpSocket *> snapshotSubscribers_;
    QHash<QTcpSocket *, QByteArray> buffers_;
};

}  // namespace pulseboost
