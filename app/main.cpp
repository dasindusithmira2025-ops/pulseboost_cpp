#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include <QDateTime>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>

#include "PulseBoostAI/ai/agent_engine.hpp"
#include "PulseBoostAI/ai/chat_router.hpp"
#include "PulseBoostAI/ai/llm_client.hpp"
#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/core/disk_analyzer.hpp"
#include "PulseBoostAI/core/memory_analyzer.hpp"
#include "PulseBoostAI/core/process_manager.hpp"
#include "PulseBoostAI/core/registry_optimizer.hpp"
#include "PulseBoostAI/core/service_manager.hpp"
#include "PulseBoostAI/core/startup_optimizer.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"
#include "PulseBoostAI/core/telemetry_engine.hpp"
#include "PulseBoostAI/data/optimization_history.hpp"
#include "PulseBoostAI/data/telemetry_cache.hpp"
#include "PulseBoostAI/data/telemetry_logger.hpp"
#include "PulseBoostAI/modules/developer_mode.hpp"
#include "PulseBoostAI/modules/game_mode.hpp"
#include "PulseBoostAI/modules/junk_cleaner.hpp"
#include "PulseBoostAI/modules/safety_guard.hpp"
#include "PulseBoostAI/ui_backend/ui_controller.hpp"

namespace {

std::mutex g_guiLogMutex;

void appendGuiLog(const std::string &message) {
    std::lock_guard<std::mutex> lock(g_guiLogMutex);
    std::ofstream output("logs/gui_runtime.log", std::ios::app);
    output << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString() << ' ' << message << '\n';
}

}  // namespace

int main(int argc, char *argv[]) {
    if (argc > 1) {
        pulseboost::ProcessManager processManager;
        pulseboost::ServiceManager serviceManager;
        pulseboost::RegistryOptimizer registryOptimizer;
        pulseboost::StartupOptimizer startupOptimizer(registryOptimizer);
        pulseboost::MemoryAnalyzer memoryAnalyzer(processManager);
        pulseboost::DiskAnalyzer diskAnalyzer;
        pulseboost::SystemScanner scanner(processManager,
                                          memoryAnalyzer,
                                          diskAnalyzer,
                                          startupOptimizer,
                                          serviceManager);
        pulseboost::TelemetryLogger telemetryLogger;
        pulseboost::OptimizationHistory optimizationHistory;
        pulseboost::JunkCleaner junkCleaner;

        std::ofstream selfTestLog("logs/self_test.log", std::ios::app);
        const std::string command = argv[1];

        if (command == "--scan") {
            const auto snapshot = scanner.scan();
            telemetryLogger.append(snapshot);
            selfTestLog << "scan," << pulseboost::currentTimestampUtc() << ',' << snapshot.healthScore << '\n';
            return 0;
        }

        if (command == "--clean") {
            const auto result = junkCleaner.cleanSafeTargets();
            optimizationHistory.record(pulseboost::ActionRecord {
                .timestampUtc = pulseboost::currentTimestampUtc(),
                .action = "cli-clean",
                .details = "Recovered " + std::to_string(result.bytesRecovered) + " bytes",
                .success = true,
            });
            selfTestLog << "clean," << pulseboost::currentTimestampUtc() << ',' << result.filesRemoved << '\n';
            return 0;
        }

        if (command == "--self-test") {
            const auto snapshot = scanner.scan();
            telemetryLogger.append(snapshot);
            const auto disk = diskAnalyzer.analyzeSystemDrive();
            const auto startup = startupOptimizer.scanStartupItems();
            selfTestLog << "self-test," << pulseboost::currentTimestampUtc() << ',' << snapshot.healthScore << ','
                        << disk.usedPercent << ',' << startup.size() << '\n';
            return 0;
        }

        if (command == "--chat") {
            std::string message;
            for (int index = 2; index < argc; ++index) {
                if (!message.empty()) {
                    message += ' ';
                }
                message += argv[index];
            }

            const auto snapshot = scanner.scan();
            pulseboost::ChatRouter router;
            const auto decision = router.buildDecision(QString::fromStdString(message), snapshot);
            const auto reply = pulseboost::LlmClient::offlineFallback(QString::fromStdString(message),
                                                                      snapshot,
                                                                      decision.plan);
            std::cout << reply.toStdString() << '\n';
            selfTestLog << "chat," << pulseboost::currentTimestampUtc() << ',' << snapshot.healthScore << '\n';
            return 0;
        }
    }

    appendGuiLog("GUI bootstrap start");
    QQuickStyle::setStyle("Basic");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);

    QGuiApplication app(argc, argv);
    app.setApplicationName("PulseBoost AI");
    app.setOrganizationName("PulseBoost");
    app.setApplicationVersion("1.0.0");

    QSettings::setDefaultFormat(QSettings::IniFormat);

    pulseboost::ProcessManager processManager;
    pulseboost::ServiceManager serviceManager;
    pulseboost::RegistryOptimizer registryOptimizer;
    pulseboost::StartupOptimizer startupOptimizer(registryOptimizer);
    pulseboost::MemoryAnalyzer memoryAnalyzer(processManager);
    pulseboost::DiskAnalyzer diskAnalyzer;
    pulseboost::SystemScanner scanner(processManager,
                                      memoryAnalyzer,
                                      diskAnalyzer,
                                      startupOptimizer,
                                      serviceManager);
    pulseboost::TelemetryLogger telemetryLogger;
    pulseboost::OptimizationHistory optimizationHistory;
    pulseboost::JunkCleaner junkCleaner;
    pulseboost::GameMode gameMode(processManager, serviceManager);
    pulseboost::DeveloperMode developerMode(processManager, serviceManager);
    pulseboost::SafetyGuard safetyGuard;
    pulseboost::TelemetryCache telemetryCache(300);

    pulseboost::UiController uiController(junkCleaner,
                                          gameMode,
                                          processManager,
                                          safetyGuard,
                                          telemetryCache,
                                          optimizationHistory);

    pulseboost::AgentEngine agentEngine(scanner,
                                        telemetryCache,
                                        junkCleaner,
                                        startupOptimizer,
                                        gameMode,
                                        developerMode,
                                        optimizationHistory,
                                        safetyGuard);

    pulseboost::TelemetryEngine telemetryEngine(scanner);

    QObject::connect(&telemetryEngine,
                     &pulseboost::TelemetryEngine::snapshotReady,
                     &uiController,
                     [&](const pulseboost::SystemSnapshot &snap) {
                         telemetryCache.push(snap);
                         uiController.onSnapshotReady(snap);
                         telemetryLogger.append(snap);
                         agentEngine.updateSnapshot(snap);
                     });

    QQmlApplicationEngine engine;
    appendGuiLog("QQmlApplicationEngine created");

    QObject::connect(&engine, &QQmlEngine::warnings, &app, [](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) {
            appendGuiLog("QML warning: " + warning.toString().toStdString());
        }
    });

    engine.rootContext()->setContextProperty("SystemCtrl", &uiController);
    engine.rootContext()->setContextProperty("AgentEngine", &agentEngine);
    appendGuiLog("Context properties registered");

    const QUrl url(QStringLiteral("qrc:/ui/qml/MainWindow.qml"));
    QObject::connect(&engine,
                     &QQmlApplicationEngine::objectCreated,
                     &app,
                     [url](QObject *obj, const QUrl &objUrl) {
                         appendGuiLog(std::string("objectCreated: ")
                                      + (obj ? "success " : "failure ")
                                      + objUrl.toString().toStdString());
                         if (!obj && url == objUrl) {
                             QCoreApplication::exit(-1);
                         }
                     },
                     Qt::QueuedConnection);

    appendGuiLog("Loading QML: " + url.toString().toStdString());
    engine.load(url);
    appendGuiLog("Root object count: " + std::to_string(engine.rootObjects().size()));

    telemetryEngine.start();
    appendGuiLog("Telemetry engine started");

    const int exitCode = app.exec();
    appendGuiLog("Event loop exited with code " + std::to_string(exitCode));
    telemetryEngine.stop();
    return exitCode;
}
