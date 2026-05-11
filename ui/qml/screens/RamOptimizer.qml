import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/charts"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.s32

    property var memory: SystemCtrl.memoryOverview
    property var series: SystemCtrl.chartSeries

    function ramSeries() {
        var points = []
        for (var i = 0; i < series.length; i += 1) {
            points.push(series[i].ram)
        }
        return points
    }

    function segmentWidth(value) {
        var total = Math.max(1, memory.totalMb || 1)
        return Math.max(12, (value / total) * (breakdownRow.width - Style.s24))
    }

    ColumnLayout {
        id: contentColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 120

            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: "Recoverable Memory Payload"
                        color: Style.text2
                        font.family: Style.fontMono
                        font.pixelSize: Style.f11
                        font.letterSpacing: 1
                    }
                    RowLayout {
                        spacing: Style.s8
                        Text {
                            text: Number(SystemCtrl.recoverableRamMb).toFixed(0)
                            color: Style.cyan
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f40
                            font.weight: Style.w700
                            
                            layer.enabled: true
                            layer.effect: DropShadow { color: Style.cyanGlow; radius: 15; samples: 25; spread: 0.1 }
                        }
                        Text {
                            text: "MB"
                            color: Style.cyan
                            font.family: Style.fontMono
                            font.pixelSize: Style.f14
                            Layout.alignment: Qt.AlignBottom
                            Layout.bottomMargin: 6
                        }
                    }
                    Text {
                        text: "PulseBoost estimates this volume can be securely reclaimed from dormant background working sets."
                        color: Style.text1
                        font.family: Style.fontBody
                        font.pixelSize: Style.f13
                        wrapMode: Text.WordWrap
                        Layout.maximumWidth: parent.width * 0.8
                    }
                }

                GlowButton {
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 52
                    label: "Deep Clean Memory"
                    icon: Icons.glyph("bolt")
                    glowColor: Style.cyan
                    onClicked: SystemCtrl.optimizeRam()
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1180 ? 2 : 1
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 240

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s16

                    Text {
                        text: "Memory Footprint Breakdown"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }

                    Row {
                        id: breakdownRow
                        Layout.fillWidth: true
                        spacing: Style.s8
                        Layout.topMargin: Style.s8
                        Layout.bottomMargin: Style.s8

                        Rectangle { width: root.segmentWidth(memory.systemMb || 0); height: 32; radius: Style.r8; color: Style.red }
                        Rectangle { width: root.segmentWidth(memory.appsMb || 0); height: 32; radius: Style.r8; color: Style.cyan; opacity: 0.9 }
                        Rectangle { width: root.segmentWidth(memory.cachedMb || 0); height: 32; radius: Style.r8; color: Style.violet; opacity: 0.8 }
                        Rectangle { width: root.segmentWidth(memory.freeMb || 0); height: 32; radius: Style.r8; color: Style.green }
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: Style.s12
                        columnSpacing: Style.s20
                        Layout.fillWidth: true

                        Row { spacing: 8; Rectangle { width: 12; height: 12; radius: 6; color: Style.red; anchors.verticalCenter: parent.verticalCenter } Text { text: "System Reserved"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; anchors.verticalCenter: parent.verticalCenter } }
                        Text { text: Number(memory.systemMb || 0).toFixed(0) + " MB"; color: Style.red; font.family: Style.fontMono; font.pixelSize: Style.f13; font.weight: Style.w600; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }

                        Row { spacing: 8; Rectangle { width: 12; height: 12; radius: 6; color: Style.cyan; anchors.verticalCenter: parent.verticalCenter } Text { text: "Active Applications"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; anchors.verticalCenter: parent.verticalCenter } }
                        Text { text: Number(memory.appsMb || 0).toFixed(0) + " MB"; color: Style.cyan; font.family: Style.fontMono; font.pixelSize: Style.f13; font.weight: Style.w600; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }

                        Row { spacing: 8; Rectangle { width: 12; height: 12; radius: 6; color: Style.violet; anchors.verticalCenter: parent.verticalCenter } Text { text: "Cached Pages"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; anchors.verticalCenter: parent.verticalCenter } }
                        Text { text: Number(memory.cachedMb || 0).toFixed(0) + " MB"; color: Style.violet; font.family: Style.fontMono; font.pixelSize: Style.f13; font.weight: Style.w600; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }

                        Row { spacing: 8; Rectangle { width: 12; height: 12; radius: 6; color: Style.green; anchors.verticalCenter: parent.verticalCenter } Text { text: "Free & Available"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; anchors.verticalCenter: parent.verticalCenter } }
                        Text { text: Number(memory.freeMb || 0).toFixed(0) + " MB"; color: Style.green; font.family: Style.fontMono; font.pixelSize: Style.f13; font.weight: Style.w600; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }
                    }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 240

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s16

                    Text {
                        text: "Algorithm Aggressiveness"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }

                    Text {
                        text: "Define the hard threshold for predictive memory reclaiming. Higher values will prioritize foreground task headroom over background caching."
                        color: Style.text2
                        font.family: Style.fontBody
                        font.pixelSize: Style.f13
                        wrapMode: Text.WordWrap
                    }
                    
                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Style.s16
                        Text {
                            text: "Target Thrash %"
                            color: Style.text1
                            font.family: Style.fontMono
                            font.pixelSize: Style.f12
                        }
                        Slider {
                            id: saverSlider
                            Layout.fillWidth: true
                            from: 50
                            to: 95
                            value: 75
                        }
                        Text {
                            text: Number(saverSlider.value).toFixed(0) + "%"
                            color: Style.text0
                            font.family: Style.fontMono
                            font.pixelSize: Style.f14
                            font.weight: Style.w700
                        }
                    }

                    GlowButton {
                        Layout.preferredWidth: 200
                        label: "Commit Saver Profile"
                        variant: "outlined"
                        glowColor: Style.violet
                        onClicked: SystemCtrl.optimizeRam()
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 320

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s16

                Text {
                    text: "Physical Memory Allocation History"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }

                LineChart {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    data: root.ramSeries()
                    lineColor: Style.cyan
                    maxValue: 100
                    minValue: 0
                    revealProgress: 1
                }
            }
        }
    }
}
