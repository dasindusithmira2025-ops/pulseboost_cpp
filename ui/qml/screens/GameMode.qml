import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/feedback"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.s32

    property var status: SystemCtrl.gameModeStatus

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
            fillColor: root.status.active ? Style.amberGlow : Style.bg1
            borderColor: root.status.active ? Style.amber : Style.borderGlass

            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20

                Rectangle {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    radius: Style.r16
                    color: root.status.active ? Style.amber : Style.bg3
                    border.color: root.status.active ? Style.amber : Style.border1

                    layer.enabled: root.status.active
                    layer.effect: DropShadow { color: Style.amber; radius: 12; samples: 25; spread: 0.1 }

                    Text {
                        anchors.centerIn: parent
                        text: Icons.glyph("game")
                        color: root.status.active ? Style.bg0 : Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f32
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: root.status.active ? "Gaming Mode Engaged" : "Gaming Mode Ready"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f24
                        font.weight: Style.w700
                    }

                    Text {
                        text: root.status.detectedGame + " | " + root.status.statusLabel
                        color: root.status.active ? Style.amber : Style.text1
                        font.family: Style.fontBody
                        font.pixelSize: Style.f14
                        font.weight: Style.w600
                    }

                    Text {
                        text: root.status.active
                              ? "PulseBoost is prioritizing your game, trimming background memory, and holding the system on a performance profile."
                              : "Launch a supported game to trigger automatic network latency reduction, memory cleanup, and process priority bumps."
                        color: Style.text2
                        font.family: Style.fontBody
                        font.pixelSize: Style.f13
                        wrapMode: Text.WordWrap
                    }
                }

                GlowButton {
                    Layout.preferredWidth: 200
                    label: root.status.active ? "Disable Overdrive" : "Force Overdrive"
                    icon: Icons.glyph("bolt")
                    glowColor: root.status.active ? Style.red : Style.amber
                    variant: root.status.active ? "outlined" : "solid"
                    onClicked: root.status.active ? SystemCtrl.disableGameMode() : SystemCtrl.enableGameMode()
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1180 ? 4 : 2
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            Repeater {
                model: [
                    {
                        label: "Hardware Boost",
                        value: Number(root.status.boostEstimate || 0).toFixed(0),
                        unit: "%",
                        detail: "Projected CPU priority headroom",
                        accent: Style.amber
                    },
                    {
                        label: "Network Latency",
                        value: root.status.latencyAfter >= 0 ? root.status.latencyAfter : SystemCtrl.networkOverview.latency,
                        unit: "ms",
                        detail: root.status.latencyBefore >= 0 ? ("Was " + root.status.latencyBefore + "ms") : "Current edge gateway latency",
                        accent: Style.cyan
                    },
                    {
                        label: "Memory Freed",
                        value: Number(root.status.ramFreedMb || 0).toFixed(0),
                        unit: "MB",
                        detail: "Trimmed from background caches",
                        accent: Style.green
                    },
                    {
                        label: "Framerate Delta",
                        value: Number(root.status.fpsBoostEstimate || 0).toFixed(0),
                        unit: "%",
                        detail: "Estimated application FPS lift",
                        accent: Style.violet
                    }
                ]

                delegate: GlassCard {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.s16
                        spacing: Style.s8

                        Text {
                            text: modelData.label.toUpperCase()
                            color: Style.text2
                            font.family: Style.fontMono
                            font.pixelSize: Style.f11
                            font.letterSpacing: 1
                        }

                        RowLayout {
                            spacing: Style.s6
                            Text {
                                text: modelData.value
                                color: modelData.accent
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f32
                                font.weight: Style.w700
                            }
                            Text {
                                text: modelData.unit
                                color: Style.text1
                                font.family: Style.fontMono
                                font.pixelSize: Style.f14
                                Layout.alignment: Qt.AlignBottom
                            }
                        }

                        Text {
                            text: modelData.detail
                            color: Style.text1
                            font.family: Style.fontBody
                            font.pixelSize: Style.f12
                            wrapMode: Text.WordWrap
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
                    text: "Game Optimizations Matrix"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }

                GridLayout {
                    columns: 2
                    rowSpacing: Style.s12
                    columnSpacing: Style.s20
                    Layout.fillWidth: true

                    Repeater {
                        model: [
                            "Elevates the detected game process to highest real-time CPU priority limits.",
                            "Reduces priority of selected background applications consuming CPU slices.",
                            "Flushes local DNS cache and executes low-latency TCP stack tuning.",
                            "Purges standby RAM lists to open memory pages for game assets.",
                            "Forces Windows power plan into Ultimate Performance configuration."
                        ]

                        delegate: Rectangle {
                            required property string modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            radius: Style.r8
                            color: Style.bg2
                            border.color: Style.border1
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: Style.s12
                                spacing: Style.s12

                                Rectangle {
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: 8
                                    radius: 4
                                    color: Style.amber
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData
                                    color: Style.text1
                                    font.family: Style.fontBody
                                    font.pixelSize: Style.f13
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                EmptyState {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    visible: !root.status.active && root.status.detectedPid <= 0
                    title: "Awaiting software telemetry..."
                    subtitle: "PulseBoost runs silently in the background waiting for a high-intensity fullscreen process."
                }
            }
        }
    }
}
