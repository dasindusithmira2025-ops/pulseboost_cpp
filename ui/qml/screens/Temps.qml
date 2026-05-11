import QtGraphicalEffects 1.15
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
    contentHeight: contentColumn.implicitHeight + Style.s32

    property var thermal: SystemCtrl.thermalOverview
    property var history: SystemCtrl.thermalSeries

    function tempColor(value) {
        if (value >= 85) return Style.red
        if (value >= 60) return Style.amber
        return Style.green
    }
    
    function tempGlow(value) {
        if (value >= 85) return Style.redGlow
        if (value >= 60) return Style.amberGlow
        return Style.greenGlow
    }

    function tempSeries(key) {
        var values = []
        for (var i = 0; i < history.length; i += 1) {
            values.push(Number(history[i] && history[i][key] !== undefined ? history[i][key] : 0))
        }
        return values
    }

    readonly property var gauges: [
        { "title": "Processor Array", "value": Number(thermal.cpuTempC || 0), "zone": thermal.cpuZone || "Liquid Cool", "key": "cpuTemp", "accent": tempColor(Number(thermal.cpuTempC || 0)), "glow": tempGlow(Number(thermal.cpuTempC || 0)) },
        { "title": "Graphics Core", "value": Number(thermal.gpuTempC || 0), "zone": thermal.gpuZone || "Ambient", "key": "gpuTemp", "accent": tempColor(Number(thermal.gpuTempC || 0)), "glow": tempGlow(Number(thermal.gpuTempC || 0)) },
        { "title": "Storage Bays", "value": Number(thermal.driveTempC || 0), "zone": thermal.driveZone || "Stable", "key": "driveTemp", "accent": tempColor(Number(thermal.driveTempC || 0)), "glow": tempGlow(Number(thermal.driveTempC || 0)) }
    ]

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
            borderColor: thermal.cpuThrottling ? Style.red : Style.borderGlass
            fillColor: thermal.cpuThrottling ? Style.redGlow : Style.bg1

            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s6

                    Text {
                        text: "Core Thermodynamics"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f24
                        font.weight: Style.w700
                    }

                    Text {
                        text: "Thermal threshold rating: " + Number(thermal.thermalScore || 0).toFixed(0) + "/100  |  Peak junction: " + Number(thermal.hottestTempC || 0).toFixed(0) + " °C"
                        color: Style.text1
                        font.family: Style.fontMono
                        font.pixelSize: Style.f13
                    }

                    Text {
                        text: thermal.cpuThrottling ? "CRITICAL: System hardware is aggressively thermal throttling downclocks to protect silicon lifespans." : "Thermal envelopes are stable. Operating under maximum thermal junction constraints."
                        color: thermal.cpuThrottling ? Style.red : Style.text2
                        font.family: Style.fontBody
                        font.pixelSize: Style.f13
                        wrapMode: Text.WordWrap
                    }
                }

                GlassCard {
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 80
                    fillColor: Style.bg2
                    borderColor: thermal.cpuThrottling ? Style.red : Style.border1
                    contentMargin: 0

                    Column {
                        anchors.centerIn: parent
                        spacing: Style.s4

                        Text {
                            text: "Active Exhaust"
                            color: Style.text2
                            font.family: Style.fontMono
                            font.pixelSize: Style.f11
                            font.letterSpacing: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: Number(thermal.fanSpeedRpm || 0).toFixed(0) + " RPM"
                            color: Style.cyan
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f28
                            font.weight: Style.w700
                            anchors.horizontalCenter: parent.horizontalCenter
                            
                            layer.enabled: true
                            layer.effect: DropShadow { color: Style.cyanGlow; radius: 10; spread: 0.2; samples: 21 }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s20

            Repeater {
                model: root.gauges
                delegate: GlassPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 300

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.cardPad
                        spacing: Style.s16

                        Text {
                            text: modelData.title
                            color: Style.text0
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f20
                            font.weight: Style.w700
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 160

                            Canvas {
                                id: ring
                                anchors.centerIn: parent
                                width: 160
                                height: 160
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.reset()
                                    var cx = width / 2
                                    var cy = height / 2
                                    var radius = 64
                                    
                                    // Track gradient
                                    ctx.lineWidth = 14
                                    ctx.strokeStyle = Style.bg3
                                    ctx.lineCap = "round"
                                    ctx.beginPath()
                                    ctx.arc(cx, cy, radius, Math.PI * 0.75, Math.PI * 2.25, false)
                                    ctx.stroke()
                                    
                                    // Value line
                                    ctx.strokeStyle = modelData.accent
                                    ctx.beginPath()
                                    var endAngle = (Math.PI * 0.75) + (Math.PI * 1.5 * Math.min(1, modelData.value / 100.0))
                                    ctx.arc(cx, cy, radius, Math.PI * 0.75, endAngle, false)
                                    ctx.stroke()
                                }

                                Timer { interval: 1000; running: true; repeat: true; onTriggered: ring.requestPaint() }
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 0

                                Text {
                                    text: Number(modelData.value).toFixed(0) + "°"
                                    color: modelData.accent
                                    font.family: Style.fontDisplay
                                    font.pixelSize: Style.f40
                                    font.weight: Style.w700
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    
                                    layer.enabled: true
                                    layer.effect: DropShadow { color: modelData.glow; radius: 15; spread: 0.1; samples: 25 }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            radius: Style.r999
                            color: modelData.glow
                            border.color: modelData.accent
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: modelData.zone.toUpperCase() + " OPERATION"
                                color: modelData.accent
                                font.family: Style.fontMono
                                font.pixelSize: Style.f11
                                font.weight: Style.w700
                                font.letterSpacing: 1
                            }
                        }
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 280

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s16

                Text {
                    text: "Component Heat Dissipation Signatures"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Style.s20

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Style.s8
                        RowLayout {
                            spacing: Style.s8
                            Rectangle { width: 10; height: 10; radius: 5; color: Style.amber }
                            Text { text: "Processor Array"; color: Style.text1; font.family: Style.fontMono; font.pixelSize: Style.f12; font.weight: Style.w600 }
                        }
                        LineChart { Layout.fillWidth: true; Layout.fillHeight: true; data: root.tempSeries("cpuTemp"); lineColor: Style.amber; minValue: 20; maxValue: 100; revealProgress: 1 }
                    }
                    
                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Style.borderGlass }
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Style.s8
                        RowLayout {
                            spacing: Style.s8
                            Rectangle { width: 10; height: 10; radius: 5; color: Style.magenta }
                            Text { text: "Graphics Core"; color: Style.text1; font.family: Style.fontMono; font.pixelSize: Style.f12; font.weight: Style.w600 }
                        }
                        LineChart { Layout.fillWidth: true; Layout.fillHeight: true; data: root.tempSeries("gpuTemp"); lineColor: Style.magenta; minValue: 20; maxValue: 100; revealProgress: 1 }
                    }
                    
                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Style.borderGlass }
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Style.s8
                        RowLayout {
                            spacing: Style.s8
                            Rectangle { width: 10; height: 10; radius: 5; color: Style.cyan }
                            Text { text: "Storage Bays"; color: Style.text1; font.family: Style.fontMono; font.pixelSize: Style.f12; font.weight: Style.w600 }
                        }
                        LineChart { Layout.fillWidth: true; Layout.fillHeight: true; data: root.tempSeries("driveTemp"); lineColor: Style.cyan; minValue: 20; maxValue: 100; revealProgress: 1 }
                    }
                }
            }
        }
    }
}
