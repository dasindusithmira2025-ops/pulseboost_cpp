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

    property var snapshots: SystemCtrl.systemSnapshots
    property var recent: SystemCtrl.recentActions
    property var series: SystemCtrl.chartSeries

    function healthSeries() {
        var values = []
        if (!series) return values
        for (var i = 0; i < series.length; i += 1) values.push(Number(series[i].health))
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
            Layout.preferredHeight: 152
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s24
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8
                    Text { text: "Backup"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "Snapshots, restore points, history, and export in one native recovery surface."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        spacing: Style.s10
                        GlowButton { label: "Take Snapshot"; glowColor: Style.cyan; onClicked: SystemCtrl.takeSystemSnapshot() }
                        GlowButton { label: "Restore Point"; variant: "outlined"; glowColor: Style.green; onClicked: SystemCtrl.createRestorePoint() }
                        GlowButton { label: "Export CSV"; variant: "outlined"; glowColor: Style.amber; onClicked: SystemCtrl.exportChartSeriesCsv() }
                    }
                }
                Column {
                    spacing: Style.s6
                    Text { text: "Snapshots"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                    Text { text: Number(root.snapshots.length).toFixed(0); color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f40; font.weight: Style.w700 }
                    Text { text: "Recent actions " + Number(root.recent.length).toFixed(0); color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12 }
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
                Layout.preferredHeight: 320
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s10
                    SectionHeader { Layout.fillWidth: true; title: "Snapshot Library"; subtitle: "Restore startup and baseline state from a known point." }
                    Repeater {
                        model: root.snapshots
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: Style.r10
                            color: Style.bg1
                            border.color: Style.border1
                            border.width: 1
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Style.s10
                                anchors.rightMargin: Style.s10
                                spacing: Style.s10
                                Text { text: modelData.id; color: Style.text0; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: Number(modelData.healthScore).toFixed(0); color: Style.healthColor(modelData.healthScore); font.family: Style.fontMono; font.pixelSize: Style.f12; Layout.preferredWidth: 40 }
                                GlowButton { label: "Restore"; variant: "outlined"; glowColor: Style.amber; onClicked: SystemCtrl.restoreSystemSnapshot(modelData.id) }
                            }
                        }
                    }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s12
                    SectionHeader { Layout.fillWidth: true; title: "Health Trend"; subtitle: "Recent system health snapshots logged by the native monitor." }
                    LineChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        data: root.healthSeries()
                        lineColor: Style.healthColor(SystemCtrl.healthScore)
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
                SectionHeader { Layout.fillWidth: true; title: "Recovery History"; subtitle: "The latest native actions affecting system state." }
                Repeater {
                    model: Math.min(6, root.recent.length)
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        radius: Style.r8
                        color: Style.bg1
                        border.color: Style.border1
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s10
                            anchors.rightMargin: Style.s10
                            spacing: Style.s10
                            Text { text: root.recent[index].timeLabel || "--:--"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 60 }
                            Text { text: root.recent[index].action; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600; Layout.preferredWidth: 160; elide: Text.ElideRight }
                            Text { text: root.recent[index].details; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                    }
                }
            }
        }
    }
}
