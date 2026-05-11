import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/charts"
import "../components/feedback"
import "../components/controls"
import "../components/foundation"

Item {
    id: root
    anchors.fill: parent
    property var series: SystemCtrl.chartSeries
    property real hoverRatio: -1
    property real revealProgress: 0
    property string lastExportPath: ""
    readonly property bool hasData: root.series && root.series.length > 1

    function values(key) {
        if (!series || series.length === 0) return []
        return series.map(function(point) { return Number(point[key]) })
    }

    function statValue(key, mode) {
        const valuesList = values(key)
        if (valuesList.length === 0) return 0
        let total = 0
        let min = valuesList[0]
        let max = valuesList[0]
        for (let i = 0; i < valuesList.length; i += 1) {
            const value = valuesList[i]
            total += value
            min = Math.min(min, value)
            max = Math.max(max, value)
        }
        if (mode === "min") return min
        if (mode === "max") return max
        return total / valuesList.length
    }

    function hoverValue(key) {
        const valuesList = values(key)
        if (valuesList.length === 0 || hoverRatio < 0) return "--"
        const index = Math.max(0, Math.min(valuesList.length - 1, Math.round(hoverRatio * (valuesList.length - 1))))
        return Number(valuesList[index]).toFixed(1)
    }

    NumberAnimation on revealProgress {
        from: 0
        to: 1
        duration: Style.xxslow
        easing.type: Easing.OutCubic
    }

    Component.onCompleted: revealProgress = 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.pagePad
        visible: root.hasData
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            contentMargin: Style.s16

            RowLayout {
                anchors.fill: parent
                spacing: Style.s16
                
                Text {
                    text: "Live Granular Telemetry"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }
                
                Item { Layout.fillWidth: true }
                
                Repeater {
                    model: ["60s", "5m", "30m", "2h"]
                    delegate: Rectangle {
                        required property var modelData
                        Layout.preferredHeight: Style.s32
                        Layout.preferredWidth: Style.s48
                        radius: Style.r999
                        color: modelData === "60s" ? Style.violetGlow : Style.bg3
                        border.color: modelData === "60s" ? Style.violet : Style.border1
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: modelData === "60s" ? Style.violet : Style.text1
                            font.family: Style.fontMono
                            font.pixelSize: Style.f11
                            font.weight: Style.w600
                        }
                    }
                }
                
                Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 24; color: Style.border1 }
                
                GlowButton {
                    label: "Export Raw CSV"
                    glowColor: Style.cyan
                    variant: "outlined"
                    onClicked: root.lastExportPath = SystemCtrl.exportChartSeriesCsv()
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: width > 1100 ? 2 : 1
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            Repeater {
                model: [
                    { label: "Processor Yield", key: "cpu", color: Style.cCpu, unit: "%" },
                    { label: "Memory Pages", key: "ram", color: Style.cRam, unit: "%" },
                    { label: "Drive Saturation", key: "disk", color: Style.cDisk, unit: "%" },
                    { label: "Network Bandwidth", key: "network", color: Style.cNet, unit: " Mb/s" }
                ]
                delegate: GlassPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentMargin: Style.s16

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Style.s12

                        RowLayout {
                            Layout.fillWidth: true
                            
                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: modelData.color
                                layer.enabled: true
                                layer.effect: DropShadow { color: modelData.color; radius: 8; spread: 0.2 }
                            }
                            
                            Text {
                                text: modelData.label
                                color: Style.text0
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f18
                                font.weight: Style.w700
                            }
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: root.hoverRatio >= 0 ? "Cursor value:" : "Current:"
                                color: Style.text2
                                font.family: Style.fontMono
                                font.pixelSize: Style.f11
                            }
                            Text {
                                text: root.hoverValue(modelData.key) + modelData.unit
                                color: modelData.color
                                font.family: Style.fontMono
                                font.pixelSize: Style.f18
                                font.weight: Style.w700
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            LineChart {
                                anchors.fill: parent
                                data: root.values(modelData.key)
                                lineColor: modelData.color
                                maxValue: 100
                                hoverRatio: root.hoverRatio
                                revealProgress: root.revealProgress
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onPositionChanged: root.hoverRatio = Math.max(0, Math.min(1, mouseX / Math.max(1, width)))
                                onExited: root.hoverRatio = -1
                            }
                            
                            Rectangle {
                                visible: root.hoverRatio >= 0
                                x: root.hoverRatio * parent.width
                                y: 0
                                width: 1
                                height: parent.height
                                color: Style.text1
                                opacity: 0.5
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: Style.borderGlass }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Style.s16

                            SummaryPill { label: "Nadir"; value: Number(root.statValue(modelData.key, "min")).toFixed(1) + modelData.unit; accent: modelData.color }
                            SummaryPill { label: "Average"; value: Number(root.statValue(modelData.key, "avg")).toFixed(1) + modelData.unit; accent: modelData.color }
                            SummaryPill { label: "Zenith"; value: Number(root.statValue(modelData.key, "max")).toFixed(1) + modelData.unit; accent: modelData.color }
                            Item { Layout.fillWidth: true }
                        }
                    }
                }
            }
        }

        GlassCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            visible: root.lastExportPath !== ""
            fillColor: Style.cyanGlow
            borderColor: Style.cyan

            Text {
                anchors.centerIn: parent
                text: "Telemetry traces successfully encoded to " + root.lastExportPath
                color: Style.cyan
                font.family: Style.fontMono
                font.pixelSize: Style.f12
                font.weight: Style.w600
                elide: Text.ElideMiddle
                width: parent.width - Style.s32
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    EmptyState {
        anchors.fill: parent
        visible: !root.hasData
        title: "Waiting for metric stream"
        subtitle: "C++ backend telemetry controller is indexing system traces."
    }

    component SummaryPill: RowLayout {
        property string label: ""
        property string value: ""
        property color accent: Style.violet
        spacing: Style.s6

        Text {
            text: label.toUpperCase()
            color: Style.text2
            font.family: Style.fontMono
            font.pixelSize: Style.f11
        }
        Text {
            text: value
            color: accent
            font.family: Style.fontMono
            font.pixelSize: Style.f13
            font.weight: Style.w700
        }
    }
}
