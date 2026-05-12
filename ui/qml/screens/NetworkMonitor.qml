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

    property var series: SystemCtrl.chartSeries
    property int latency: SystemCtrl.checkLatency()
    property var overview: SystemCtrl.networkOverview

    function netSeries() {
        if (!series || series.length === 0) return []
        return series.map(function(point) { return Number(point.network) })
    }

    function latencyColor(value) {
        if (value < 0) return Style.text2
        if (value <= 30) return Style.success
        if (value <= 80) return Style.warning
        return Style.danger
    }
    
    function latencyGlow(value) {
        if (value < 0) return Style.border1
        if (value <= 30) return Style.greenGlow
        if (value <= 80) return Style.amberGlow
        return Style.redGlow
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
            Layout.preferredHeight: 72
            contentMargin: Style.s16

            RowLayout {
                anchors.fill: parent
                spacing: Style.s16
                
                Text {
                    text: "Network Intelligence & Latency"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }
                
                Item { Layout.fillWidth: true }
                
                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: root.latencyColor(root.latency)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: root.latency < 0 ? "-- ms ping" : root.latency + " ms ping"
                        color: root.latencyColor(root.latency)
                        font.family: Style.fontMono
                        font.pixelSize: Style.f14
                        font.weight: Style.w600
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1180 ? 5 : (width > 800 ? 3 : 2)
            columnSpacing: Style.s16
            rowSpacing: Style.s16

            Repeater {
                model: [
                    { label: "Active Connections", value: String(root.overview.activeConnections || 0), accent: Style.amber },
                    { label: "Top Origin Process", value: String(root.overview.topProcess || "System"), accent: Style.violet },
                    { label: "DNS Resolution", value: String(root.overview.dnsServer || "Unavailable"), accent: Style.cyan },
                    { label: "Hardware Link", value: String(root.overview.connectionType || "Idle"), accent: Style.green },
                    { label: "Signal Fidelity", value: String(root.overview.signalQuality || 0) + "%", accent: Style.magenta }
                ]
                delegate: GlassCard {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 88
                    borderColor: Style.borderGlass
                    
                    Column {
                        anchors.fill: parent
                        anchors.margins: Style.s16
                        spacing: Style.s8

                        Text {
                            text: modelData.label.toUpperCase()
                            color: Style.text2
                            font.family: Style.fontMono
                            font.pixelSize: Style.f10
                            font.letterSpacing: 1
                        }
                        Text {
                            text: modelData.value
                            color: modelData.accent
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f20
                            font.weight: Style.w700
                            elide: Text.ElideRight
                            width: parent.width
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1100 ? 2 : 1
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 380

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.s4
                    spacing: Style.s16

                    Text {
                        text: "Bandwidth Pressure Map"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f18
                        font.weight: Style.w700
                    }
                    
                    LineChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        data: root.netSeries()
                        lineColor: Style.amber
                    }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 380

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.s4
                    spacing: Style.s16

                    Text {
                        text: "Edge Gateway Latency"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f18
                        font.weight: Style.w700
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Canvas {
                            anchors.fill: parent
                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()
                            Connections {
                                target: root
                                function onLatencyChanged() { parent.requestPaint() }
                            }
                            
                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                ctx.clearRect(0, 0, width, height)
                                const cx = width / 2
                                const cy = height * 0.75
                                const radius = Math.min(width, height) * 0.38
                                const start = Math.PI
                                const end = Math.PI * 2

                                // Background track
                                ctx.lineCap = "round"
                                ctx.lineWidth = 14
                                ctx.strokeStyle = Style.bg3
                                ctx.beginPath()
                                ctx.arc(cx, cy, radius, start, end)
                                ctx.stroke()

                                // Foreground Value
                                const value = Math.max(0, Math.min(200, root.latency))
                                const ratio = value / 200
                                ctx.lineWidth = 14
                                ctx.strokeStyle = root.latencyColor(root.latency)
                                ctx.beginPath()
                                ctx.arc(cx, cy, radius, start, start + (end - start) * ratio)
                                ctx.stroke()
                            }
                        }

                        Column {
                            anchors.centerIn: parent
                            anchors.verticalCenterOffset: 12
                            spacing: 2
                            
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: root.latency < 0 ? "--" : root.latency
                                color: Style.text0
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f56
                                font.weight: Style.w700
                                
                                // Glowing text logic
                                layer.enabled: true
                                layer.effect: DropShadow {
                                    color: root.latencyGlow(root.latency)
                                    radius: 12
                                    samples: 25
                                    spread: 0.1
                                }
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: root.latency < 0 ? "Unavailable" : "milliseconds"
                                color: Style.text2
                                font.family: Style.fontMono
                                font.pixelSize: Style.f12
                                font.letterSpacing: 2
                            }
                        }
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            contentMargin: Style.s16
            
            RowLayout {
                anchors.fill: parent
                spacing: Style.s16
                
                GlowButton {
                    Layout.preferredWidth: 180
                    label: "Run Diagnostics"
                    glowColor: Style.cyan
                    onClicked: {
                        root.latency = SystemCtrl.checkLatency()
                        root.overview = SystemCtrl.networkOverview
                    }
                }
                GlowButton {
                    Layout.preferredWidth: 220
                    label: "TCP Change: Manual Review"
                    glowColor: Style.violet
                    variant: "outlined"
                    enabled: false
                }
                GlowButton {
                    Layout.preferredWidth: 160
                    label: "Recheck Latency"
                    variant: "outlined"
                    glowColor: Style.amber
                    onClicked: {
                        root.latency = SystemCtrl.checkLatency()
                        root.overview = SystemCtrl.networkOverview
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }
    }
}
