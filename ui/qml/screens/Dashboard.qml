import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/cards"
import "../components/charts"
import "../components/controls"
import "../components/feedback"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.pagePad * 2
    readonly property real availableWidth: Math.max(0, root.width - Style.pagePad * 2)

    property var series: SystemCtrl.chartSeries
    property var cpuHistory: chartOf("cpu")
    property var ramHistory: chartOf("ram")
    property var diskHistory: chartOf("disk")
    property var netHistory: chartOf("network")
    property var healthHistory: chartOf("health")
    property real healthDelta: healthHistory.length > 10 ? healthHistory[healthHistory.length - 1] - healthHistory[Math.max(0, healthHistory.length - 10)] : 0
    property real savedTodayMb: SystemCtrl.savedTodayMb

    function chartOf(key) {
        if (!series || series.length === 0) return []
        return series.map(function(point) { return Number(point[key]) })
    }

    function systemMood(score) {
        if (score >= 80) return "Optimal"
        if (score >= 55) return "Stressed"
        return "Critical"
    }

    function systemMoodColor(score) {
        if (score >= 80) return Style.success
        if (score >= 55) return Style.warning
        return Style.danger
    }

    function badgeColor(tone) {
        if (tone === "success") return Style.greenGlow
        if (tone === "warning") return Style.amberGlow
        if (tone === "error") return Style.redGlow
        return Style.cyanGlow
    }

    function badgeTextColor(tone) {
        if (tone === "success") return Style.success
        if (tone === "warning") return Style.warning
        if (tone === "error") return Style.danger
        return Style.info
    }

    Column {
        id: contentColumn
        width: root.width
        spacing: Style.s20

        GridLayout {
            id: topGrid
            x: Style.pagePad
            y: Style.pagePad
            width: root.availableWidth
            columns: root.availableWidth > 1180 ? 3 : 1
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            // Main Health Score Card
            GlassPanel {
                Layout.preferredWidth: topGrid.columns > 1 ? root.availableWidth * 0.28 : root.availableWidth
                Layout.preferredHeight: 280
                
                Column {
                    anchors.centerIn: parent
                    spacing: Style.s12

                    HealthRingLarge {
                        anchors.horizontalCenter: parent.horizontalCenter
                        score: SystemCtrl.healthScore
                        cpuScore: SystemCtrl.cpuScore
                        ramScore: SystemCtrl.memoryScore
                        diskScore: SystemCtrl.diskScore
                        trendDelta: root.healthDelta
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "STATUS: " + root.systemMood(SystemCtrl.healthScore).toUpperCase()
                        color: root.systemMoodColor(SystemCtrl.healthScore)
                        font.family: Style.fontMono
                        font.pixelSize: Style.f14
                        font.weight: Style.w700
                        font.letterSpacing: 2
                    }
                }
            }

            // Key Metrics
            ColumnLayout {
                Layout.columnSpan: topGrid.columns > 1 ? 2 : 1
                Layout.fillWidth: true
                spacing: Style.s20

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.availableWidth > 1300 ? 4 : 2
                    columnSpacing: Style.s20
                    rowSpacing: Style.s20

                    MetricCard {
                        Layout.fillWidth: true
                        label: "CPU Load"
                        value: SystemCtrl.cpuUsage
                        unit: "%"
                        detail: "Top pressure in foreground tasks"
                        accentColor: Style.violet
                        sparklineData: root.cpuHistory
                    }
                    MetricCard {
                        Layout.fillWidth: true
                        label: "Memory"
                        value: SystemCtrl.ramUsage
                        unit: "%"
                        detail: "Working set pressure"
                        accentColor: Style.cyan
                        sparklineData: root.ramHistory
                    }
                    MetricCard {
                        Layout.fillWidth: true
                        label: "Storage"
                        value: SystemCtrl.diskUsage
                        unit: "%"
                        detail: "Primary volume occupancy"
                        accentColor: Style.green
                        sparklineData: root.diskHistory
                    }
                    MetricCard {
                        Layout.fillWidth: true
                        label: "Network"
                        value: SystemCtrl.networkMbps
                        unit: " Mb/s"
                        detail: "Live throughput"
                        accentColor: Style.amber
                        sparklineData: root.netHistory
                    }
                }

                GlassPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Style.s8

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Live Telemetry Feed"
                                color: Style.text0
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f20
                                font.weight: Style.w700
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: "AI Forecast 24h: " + Number(SystemCtrl.forecastHealth24h).toFixed(0)
                                color: Style.healthColor(SystemCtrl.forecastHealth24h)
                                font.family: Style.fontMono
                                font.pixelSize: Style.f12
                            }
                        }
                        
                        LineChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            data: root.cpuHistory
                            lineColor: Style.violet
                        }
                    }
                }
            }
        }

        // Feature Action Banner
        GlassPanel {
            x: Style.pagePad
            width: root.availableWidth
            height: 64

            RowLayout {
                anchors.fill: parent
                spacing: Style.s16

                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    radius: Style.r12
                    color: Style.cyanGlow
                    border.color: Style.cyan
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: Icons.glyph("bolt")
                        color: Style.cyan
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                    }
                }

                Column {
                    spacing: 2
                    Text {
                        text: "PulseBoost saved you " + (root.savedTodayMb >= 1024 ? Number(root.savedTodayMb / 1024).toFixed(2) + " GB" : Number(root.savedTodayMb).toFixed(0) + " MB") + " today"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f18
                        font.weight: Style.w700
                    }
                    Text {
                        text: !UiPrefs.scheduleEnabled ? "Scheduling is currently off." : ((UiPrefs.scheduleMode === "weekly" ? "Next scheduled maintenance: weekly at " : "Next scheduled maintenance: daily at ") + ((((Number(UiPrefs.scheduleHour) + 11) % 12) + 1) + ":00 " + (Number(UiPrefs.scheduleHour) >= 12 ? "PM" : "AM")))
                        color: Style.text2
                        font.family: Style.fontBody
                        font.pixelSize: Style.f12
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }

        // Action Buttons Row
        RowLayout {
            x: Style.pagePad
            width: root.availableWidth
            height: Style.s48
            spacing: Style.s16

            GlowButton { Layout.alignment: Qt.AlignVCenter; label: "Deep Clean"; icon: Icons.glyph("storage"); glowColor: Style.cyan; onClicked: SystemCtrl.runClean() }
            GlowButton { Layout.alignment: Qt.AlignVCenter; label: "Game Mode"; icon: Icons.glyph("startup"); glowColor: Style.amber; onClicked: SystemCtrl.enableGameMode() }
            GlowButton { Layout.alignment: Qt.AlignVCenter; label: "Turbo Opt"; icon: Icons.glyph("bolt"); glowColor: Style.violet; onClicked: SystemCtrl.runOptimize() }
            GlowButton { Layout.alignment: Qt.AlignVCenter; label: "Free RAM"; icon: Icons.glyph("processes"); glowColor: Style.cyan; variant: "outlined"; onClicked: SystemCtrl.optimizeRam() }
            GlowButton { Layout.alignment: Qt.AlignVCenter; label: "Disk Scan"; icon: Icons.glyph("storage"); glowColor: Style.green; variant: "outlined"; onClicked: SystemCtrl.optimizeDisk() }
            Item { Layout.fillWidth: true }
        }

        GridLayout {
            x: Style.pagePad
            width: root.availableWidth
            columns: root.availableWidth > 1180 ? 2 : 1
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            // Top Processes
            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 320

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.s10

                    Text {
                        text: "Top Processes"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }

                    Repeater {
                        model: Math.min(5, SystemCtrl.processList.length)
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            radius: Style.r12
                            color: Style.glassCard
                            border.color: Style.borderGlass
                            border.width: 1

                            // Fill Bar indicating CPU usage
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: Math.max(8, parent.width * Math.min(1, Number(SystemCtrl.processList[index].cpuPercent) / 100))
                                color: Number(SystemCtrl.processList[index].cpuPercent) > 80 ? Style.redGlow
                                     : (Number(SystemCtrl.processList[index].cpuPercent) > 40 ? Style.amberGlow : Style.cyanGlow)
                                radius: parent.radius
                                Behavior on width { NumberAnimation { duration: Animations.normal; easing.type: Animations.easeStandard } }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Style.s16
                                anchors.rightMargin: Style.s16
                                spacing: Style.s12

                                Text {
                                    Layout.fillWidth: true
                                    text: SystemCtrl.processList[index].name
                                    color: Style.text0
                                    font.family: Style.fontBody
                                    font.pixelSize: Style.f13
                                    font.weight: Style.w600
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: Number(SystemCtrl.processList[index].cpuPercent).toFixed(1) + "% CPU"
                                    color: Style.usageColor(SystemCtrl.processList[index].cpuPercent)
                                    font.family: Style.fontMono
                                    font.pixelSize: Style.f12
                                }
                                Text {
                                    text: Number(SystemCtrl.processList[index].memoryMb).toFixed(0) + " MB"
                                    color: Style.text2
                                    font.family: Style.fontMono
                                    font.pixelSize: Style.f12
                                }
                            }
                        }
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: SystemCtrl.processList.length === 0
                        title: "No process data yet"
                        subtitle: "Process telemetry updates every few seconds."
                    }
                }
            }

            // Recent Actions
            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 320

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.s10

                    Text {
                        text: "Recent Agent Actions"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }

                    Repeater {
                        model: (SystemCtrl.recentActions || []).slice(0, 5)
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            radius: Style.r12
                            color: Style.glassCard
                            border.color: Style.borderGlass
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Style.s12
                                anchors.rightMargin: Style.s12
                                spacing: Style.s12

                                Rectangle {
                                    Layout.preferredWidth: 80
                                    Layout.preferredHeight: 28
                                    radius: Style.r999
                                    color: root.badgeColor(modelData.tone)
                                    border.color: root.badgeTextColor(modelData.tone)
                                    border.width: 1
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.type
                                        color: root.badgeTextColor(modelData.tone)
                                        font.family: Style.fontMono
                                        font.pixelSize: Style.f10
                                        font.weight: Style.w600
                                    }
                                }

                                Column {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        width: parent.width
                                        text: modelData.action
                                        color: Style.text0
                                        font.family: Style.fontBody
                                        font.pixelSize: Style.f13
                                        font.weight: Style.w600
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        width: parent.width
                                        text: modelData.details
                                        color: Style.text2
                                        font.family: Style.fontBody
                                        font.pixelSize: Style.f11
                                        elide: Text.ElideRight
                                    }
                                }

                                Text {
                                    text: modelData.timeLabel
                                    color: Style.text3
                                    font.family: Style.fontMono
                                    font.pixelSize: Style.f10
                                }
                            }
                        }
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: SystemCtrl.recentActions.length === 0
                        title: "No actions logged"
                        subtitle: "Optimization history will appear here."
                    }
                }
            }
        }
    }
}
