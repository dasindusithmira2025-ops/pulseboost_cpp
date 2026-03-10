#include "PulseBoostAI/ui/dashboard_ui.hpp"

#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractItemView>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/ui/chatbot_window.hpp"
#include "PulseBoostAI/ui/disk_visualizer.hpp"
#include "PulseBoostAI/ui/performance_graph.hpp"

namespace pulseboost {

DashboardWindow::DashboardWindow(SystemScanner &scanner, TelemetryLogger &telemetryLogger, ChatbotInterface &chatbot, QWidget *parent)
    : QMainWindow(parent), scanner_(scanner), telemetryLogger_(telemetryLogger) {
    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(central);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    auto *leftPane = new QWidget(splitter);
    auto *rightPane = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(leftPane);
    auto *rightLayout = new QVBoxLayout(rightPane);
    auto *buttonRow = new QHBoxLayout();

    auto *overviewWidget = new QWidget(leftPane);
    auto *overviewLayout = new QGridLayout(overviewWidget);
    healthScoreLabel_ = new QLabel("Health: --", overviewWidget);
    summaryLabel_ = new QLabel("Waiting for telemetry...", overviewWidget);
    summaryLabel_->setWordWrap(true);
    cpuLabel_ = new QLabel("CPU: --", overviewWidget);
    ramLabel_ = new QLabel("RAM: --", overviewWidget);
    diskLabel_ = new QLabel("Disk: --", overviewWidget);
    networkLabel_ = new QLabel("Network: --", overviewWidget);

    overviewLayout->addWidget(healthScoreLabel_, 0, 0);
    overviewLayout->addWidget(cpuLabel_, 0, 1);
    overviewLayout->addWidget(ramLabel_, 1, 0);
    overviewLayout->addWidget(diskLabel_, 1, 1);
    overviewLayout->addWidget(networkLabel_, 2, 0);
    overviewLayout->addWidget(summaryLabel_, 3, 0, 1, 2);

    cpuGraph_ = new PerformanceGraph("CPU Usage", QColor("#d35400"), leftPane);
    ramGraph_ = new PerformanceGraph("RAM Usage", QColor("#16a085"), leftPane);
    diskGraph_ = new PerformanceGraph("Disk Usage", QColor("#2980b9"), leftPane);
    diskVisualizer_ = new DiskVisualizer(leftPane);

    processTable_ = new QTableWidget(leftPane);
    processTable_->setColumnCount(3);
    processTable_->setHorizontalHeaderLabels({"Process", "CPU %", "RAM MB"});
    processTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    processTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    processTable_->setSelectionMode(QAbstractItemView::NoSelection);

    leftLayout->addWidget(overviewWidget);
    leftLayout->addWidget(cpuGraph_);
    leftLayout->addWidget(ramGraph_);
    leftLayout->addWidget(diskGraph_);
    leftLayout->addWidget(diskVisualizer_);
    leftLayout->addWidget(processTable_, 1);

    auto *analyzeButton = new QPushButton("Analyze Performance", rightPane);
    auto *cleanButton = new QPushButton("Clean Junk", rightPane);
    auto *optimizeButton = new QPushButton("Optimize System", rightPane);
    auto *gameButton = new QPushButton("Boost Gaming", rightPane);
    buttonRow->addWidget(analyzeButton);
    buttonRow->addWidget(cleanButton);
    buttonRow->addWidget(optimizeButton);
    buttonRow->addWidget(gameButton);

    chatbotWindow_ = new ChatbotWindow(chatbot, rightPane);
    rightLayout->addLayout(buttonRow);
    rightLayout->addWidget(chatbotWindow_);

    connect(analyzeButton, &QPushButton::clicked, this, [this]() { chatbotWindow_->sendPresetCommand("analyze performance"); });
    connect(cleanButton, &QPushButton::clicked, this, [this]() { chatbotWindow_->sendPresetCommand("clean my pc"); });
    connect(optimizeButton, &QPushButton::clicked, this, [this]() { chatbotWindow_->sendPresetCommand("optimize system"); });
    connect(gameButton, &QPushButton::clicked, this, [this]() { chatbotWindow_->sendPresetCommand("boost gaming"); });

    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter);
    setCentralWidget(central);
    setWindowTitle("PulseBoost AI");

    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &DashboardWindow::refresh);
    refreshTimer_->start(1000);
    refresh();
}

void DashboardWindow::refresh() {
    const auto snapshot = scanner_.scan();
    telemetryLogger_.append(snapshot);
    updateOverview(snapshot);
    updateProcesses(snapshot);
    cpuGraph_->pushSample(snapshot.cpuUsagePercent);
    ramGraph_->pushSample(snapshot.ramUsagePercent);
    diskGraph_->pushSample(snapshot.diskUsagePercent);
    diskVisualizer_->setCategories(snapshot.storageCategories);
}

void DashboardWindow::updateOverview(const SystemSnapshot &snapshot) {
    healthScoreLabel_->setText(QString("Health: %1/100").arg(snapshot.healthScore));
    summaryLabel_->setText(QString::fromStdString(snapshot.summary));
    cpuLabel_->setText(QString("CPU: %1%").arg(snapshot.cpuUsagePercent, 0, 'f', 1));
    ramLabel_->setText(QString("RAM: %1% (%2 / %3 MB)")
                           .arg(snapshot.ramUsagePercent, 0, 'f', 1)
                           .arg(snapshot.ramUsedMb, 0, 'f', 0)
                           .arg(snapshot.ramTotalMb, 0, 'f', 0));
    diskLabel_->setText(QString("Disk: %1% (%2 / %3 GB)")
                            .arg(snapshot.diskUsagePercent, 0, 'f', 1)
                            .arg(snapshot.diskUsedGb, 0, 'f', 1)
                            .arg(snapshot.diskTotalGb, 0, 'f', 1));
    networkLabel_->setText(QString("Network: %1 Mbps, GPU: %2%")
                               .arg(snapshot.networkMbps, 0, 'f', 2)
                               .arg(snapshot.gpuUsagePercent, 0, 'f', 1));
}

void DashboardWindow::updateProcesses(const SystemSnapshot &snapshot) {
    processTable_->setRowCount(static_cast<int>(snapshot.heavyProcesses.size()));
    for (int row = 0; row < static_cast<int>(snapshot.heavyProcesses.size()); ++row) {
        const auto &process = snapshot.heavyProcesses[static_cast<std::size_t>(row)];
        processTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(process.name)));
        processTable_->setItem(row, 1, new QTableWidgetItem(QString::number(process.cpuPercent, 'f', 1)));
        processTable_->setItem(row, 2, new QTableWidgetItem(QString::number(process.memoryMb, 'f', 0)));
    }
}

}  // namespace pulseboost
