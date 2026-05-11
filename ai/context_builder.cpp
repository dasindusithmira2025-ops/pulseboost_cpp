#include "PulseBoostAI/ai/context_builder.hpp"

namespace pulseboost {

QString ContextBuilder::buildSystemInstruction() {
    return QString(
        "You are PulseBoost AI, an elite embedded Windows diagnostic agent.\n"
        "Your role is to analyze live system telemetry, identify bottlenecks, and recommend or perform optimizations.\n"
        "Rules:\n"
        "1. Be precise, technical, and concise in your responses.\n"
        "2. Do not offer platitudes or generic advice; use the provided telemetry data.\n"
        "3. Focus on CPU, RAM, Disk, and Network health.\n"
        "4. Output format should be raw text formatted nicely, keeping it directly readable in the UI.\n"
    );
}

QString ContextBuilder::buildTelemetryContext(const SystemSnapshot &snapshot) {
    return QString(
        "--- LIVE SYSTEM TELEMETRY ---\n"
        "CPU Usage: %1%\n"
        "RAM Usage: %2% (%3 MB used)\n"
        "Disk Usage: %4%\n"
        "Network: %5 Mbps\n"
        "Health Score: %6/100\n"
        "-----------------------------\n"
    ).arg(QString::number(snapshot.cpuUsagePercent, 'f', 1))
     .arg(QString::number(snapshot.ramUsagePercent, 'f', 1))
     .arg(snapshot.ramUsedMb)
     .arg(QString::number(snapshot.diskUsagePercent, 'f', 1))
     .arg(QString::number(snapshot.networkMbps, 'f', 1))
     .arg(snapshot.healthScore);
}

QString ContextBuilder::buildUserPrompt(const QString &userQuery, const SystemSnapshot &snapshot) {
    QString telemetry = buildTelemetryContext(snapshot);
    return QString("%1\nUser Request: %2").arg(telemetry, userQuery);
}

} // namespace pulseboost
