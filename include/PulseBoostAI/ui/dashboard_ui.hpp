#pragma once

#include <QMainWindow>

#include "PulseBoostAI/ai/chatbot_interface.hpp"
#include "PulseBoostAI/core/system_scanner.hpp"
#include "PulseBoostAI/data/telemetry_logger.hpp"

class QLabel;
class QPushButton;
class QTableWidget;
class QTimer;

namespace pulseboost {

class PerformanceGraph;
class DiskVisualizer;
class ChatbotWindow;

class DashboardWindow : public QMainWindow {
public:
    DashboardWindow(SystemScanner &scanner, TelemetryLogger &telemetryLogger, ChatbotInterface &chatbot, QWidget *parent = nullptr);

private:
    void refresh();
    void updateOverview(const SystemSnapshot &snapshot);
    void updateProcesses(const SystemSnapshot &snapshot);

    SystemScanner &scanner_;
    TelemetryLogger &telemetryLogger_;
    QLabel *healthScoreLabel_ = nullptr;
    QLabel *summaryLabel_ = nullptr;
    QLabel *cpuLabel_ = nullptr;
    QLabel *ramLabel_ = nullptr;
    QLabel *diskLabel_ = nullptr;
    QLabel *networkLabel_ = nullptr;
    QTableWidget *processTable_ = nullptr;
    PerformanceGraph *cpuGraph_ = nullptr;
    PerformanceGraph *ramGraph_ = nullptr;
    PerformanceGraph *diskGraph_ = nullptr;
    DiskVisualizer *diskVisualizer_ = nullptr;
    ChatbotWindow *chatbotWindow_ = nullptr;
    QTimer *refreshTimer_ = nullptr;
};

}  // namespace pulseboost
