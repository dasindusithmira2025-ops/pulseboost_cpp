import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/charts"
import "../components/controls"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.pagePad * 2

    property var pulse: SystemCtrl.pulseScore
    property var advisors: SystemCtrl.advisorItems
    property var benchmarkDelta: SystemCtrl.latestBenchmarkDelta
    property var series: SystemCtrl.chartSeries
    property var recent: SystemCtrl.recentActions

    function chartOf(key) {
        var values = []
        if (!series) return values
        for (var i = 0; i < series.length; i += 1) {
            values.push(Number(series[i][key] || 0))
        }
        return values
    }

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            fillColor: Style.bg2
            borderColor: Style.border1

            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s24

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8

                    Text {
                        text: "PC Health Score"
                        color: Style.text2
                        font.family: Style.fontMono
                        font.pixelSize: Style.f11
                        font.letterSpacing: 1
                    }
                    RowLayout {
                        spacing: Style.s10
                        Text {
                            text: Number(root.pulse.total || 0).toFixed(0)
                            color: Style.text0
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f56
                            font.weight: Style.w700
                        }
                        StatusPill {
                            text: String(root.pulse.grade || "C")
                            tone: (root.pulse.total || 0) >= 750 ? "success" : ((root.pulse.total || 0) >= 500 ? "warning" : "error")
                        }
                    }
                    Text {
                        text: "Startup items " + Number(SystemCtrl.startupCount).toFixed(0)
                             + "  |  Tweaks applied " + Number(root.pulse.tweaksApplied || 0).toFixed(0)
                             + "  |  Telemetry age " + Math.max(0, Number(SystemCtrl.telemetryAgeMs || 0)) + " ms"
                        color: Style.text1
                        font.family: Style.fontBody
                        font.pixelSize: Style.f13
                    }
                    Text {
                        text: SystemCtrl.summary
                        color: Style.text2
                        font.family: Style.fontBody
                        font.pixelSize: Style.f13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        spacing: Style.s12
                        GlowButton { label: "Run Safe Scan"; icon: Icons.glyph("bolt"); glowColor: Style.cyan; onClicked: SystemCtrl.refreshAll() }
                        GlowButton { label: "Open Dry-run Review"; icon: Icons.glyph("storage"); variant: "outlined"; glowColor: Style.amber; onClicked: SystemCtrl.refreshAll() }
                        GlowButton { label: "Backup Now"; icon: Icons.glyph("history"); variant: "outlined"; glowColor: Style.green; onClicked: SystemCtrl.takeSystemSnapshot() }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 320
                    spacing: Style.s10

                    GlassPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 76
                        fillColor: Style.bg1
                        borderColor: Style.border1
                        Column {
                            anchors.fill: parent
                            anchors.margins: Style.s14
                            spacing: Style.s4
                            Text { text: "Benchmark Delta"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                            Text {
                                text: root.benchmarkDelta.available ? ((Number(root.benchmarkDelta.percentChange).toFixed(1)) + "%") : "Run a benchmark"
                                color: root.benchmarkDelta.available ? Style.green : Style.text1
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f28
                                font.weight: Style.w700
                            }
                        }
                    }

                    GlassPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 104
                        fillColor: Style.bg1
                        borderColor: Style.border1
                        Column {
                            anchors.fill: parent
                            anchors.margins: Style.s14
                            spacing: Style.s4
                            Text { text: "Advisor Lead"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                            Text {
                                text: root.advisors.length > 0 ? root.advisors[0].title : "No urgent issues detected"
                                color: Style.text0
                                font.family: Style.fontBody
                                font.pixelSize: Style.f13
                                font.weight: Style.w600
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                text: root.advisors.length > 0 ? root.advisors[0].description : "PulseBoost is receiving live telemetry and waiting for the next meaningful optimization opportunity."
                                color: Style.text2
                                font.family: Style.fontBody
                                font.pixelSize: Style.f12
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1200 ? 4 : 2
            columnSpacing: Style.s16
            rowSpacing: Style.s16

            Repeater {
                model: [
                    { label: "CPU", value: Number(SystemCtrl.cpuUsage).toFixed(0) + "%", detail: "Score " + Number(SystemCtrl.cpuScore).toFixed(0), tone: Style.cCpu },
                    { label: "RAM", value: Number(SystemCtrl.ramUsage).toFixed(0) + "%", detail: Number(SystemCtrl.memoryOverview.freeMb).toFixed(0) + " MB free", tone: Style.cRam },
                    { label: "Network", value: Number(SystemCtrl.networkOverview.latency).toFixed(0) + " ms", detail: Number(SystemCtrl.networkMbps).toFixed(1) + " Mbps", tone: Style.cNet },
                    { label: "Thermals", value: Number(SystemCtrl.thermalOverview.hottestTempC).toFixed(0) + " C", detail: SystemCtrl.thermalOverview.overallZone, tone: Style.cTemp }
                ]
                delegate: GlassPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 108
                    fillColor: Style.bg2
                    borderColor: Style.border1
                    Column {
                        anchors.fill: parent
                        anchors.margins: Style.s14
                        spacing: Style.s4
                        Text { text: modelData.label; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                        Text { text: modelData.value; color: modelData.tone; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                        Text { text: String(modelData.detail || ""); color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12 }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1200 ? 2 : 1
            columnSpacing: Style.s16
            rowSpacing: Style.s16

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 260
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s12
                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Live Vitals"
                        subtitle: "CPU pressure over the last minute."
                    }
                    LineChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        data: root.chartOf("cpu")
                        lineColor: Style.cCpu
                    }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 260
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s10
                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Advisor Summary"
                        subtitle: "The highest-value actions PulseBoost sees right now."
                    }
                    Repeater {
                        model: Math.min(4, root.advisors.length)
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            radius: Style.r10
                            color: Style.bg1
                            border.color: Style.border1
                            border.width: 1
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Style.s12
                                anchors.rightMargin: Style.s12
                                spacing: Style.s12
                                Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: root.advisors[index].impact === "high" ? Style.red : (root.advisors[index].impact === "medium" ? Style.amber : Style.cyan) }
                                Column {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Text { text: root.advisors[index].title; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f13; font.weight: Style.w600; elide: Text.ElideRight; width: parent.width }
                                    Text { text: root.advisors[index].description; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f11; elide: Text.ElideRight; width: parent.width }
                                }
                            }
                        }
                    }
                    Text {
                        visible: root.advisors.length === 0
                        text: "No advisor issues are currently surfaced from the native analysis engine."
                        color: Style.text2
                        font.family: Style.fontBody
                        font.pixelSize: Style.f12
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 260
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s10
                SectionHeader {
                    Layout.fillWidth: true
                    title: "Recent Actions"
                    subtitle: "Real work completed on this PC."
                }
                Repeater {
                    model: Math.min(5, root.recent.length)
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        radius: Style.r10
                        color: Style.bg1
                        border.color: Style.border1
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s12
                            anchors.rightMargin: Style.s12
                            spacing: Style.s12
                            Text { text: root.recent[index].timeLabel || "--:--"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 64 }
                            Text { text: root.recent[index].action; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f13; font.weight: Style.w600; Layout.preferredWidth: 180; elide: Text.ElideRight }
                            Text { text: root.recent[index].details; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                    }
                }
                Text {
                    visible: root.recent.length === 0
                    text: "No completed actions have been recorded yet. Run a safe scan, prepare a dry-run, or create a backup to populate this feed."
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f12
                }
            }
        }
    }
}
