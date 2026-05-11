import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.pagePad * 2

    property var games: []
    property var status: SystemCtrl.gameModeStatus
    property var advisors: SystemCtrl.advisorItems

    function refreshGames() {
        games = SystemCtrl.detectedGames
    }

    Component.onCompleted: refreshGames()

    Connections {
        target: SystemCtrl
        function onGameModeChanged() { root.refreshGames() }
    }

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s24
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8
                    Text { text: "Games"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: status.active ? ("Active session: " + status.detectedGame) : "Detect, optimize, launch, and safely revert game sessions."; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f14; font.weight: Style.w600 }
                    Text { text: status.active ? "PulseBoost has applied session tuning and can restore the previous state when you finish." : "Use the library below to optimize a detected title or launch it with native session tuning."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        spacing: Style.s10
                        GlowButton { label: status.active ? "Restore Session" : "Optimize Detected"; glowColor: status.active ? Style.red : Style.cyan; onClicked: status.active ? SystemCtrl.revertGameOptimization() : SystemCtrl.optimizeDetectedGame("") }
                        GlowButton { label: "Refresh Games"; variant: "outlined"; glowColor: Style.amber; onClicked: root.refreshGames() }
                    }
                }
                GridLayout {
                    Layout.preferredWidth: 340
                    columns: 2
                    columnSpacing: Style.s12
                    rowSpacing: Style.s12
                    Repeater {
                        model: [
                            { label: "Latency", value: Number(status.latencyAfter >= 0 ? status.latencyAfter : SystemCtrl.networkOverview.latency).toFixed(0) + " ms", tone: Style.cNet },
                            { label: "RAM Freed", value: Number(status.ramFreedMb || 0).toFixed(0) + " MB", tone: Style.cRam },
                            { label: "AI Suspended", value: status.aiSuspended ? "Yes" : "No", tone: status.aiSuspended ? Style.green : Style.text1 },
                            { label: "Launchers Paused", value: status.launchersSuspended ? "Yes" : "No", tone: status.launchersSuspended ? Style.green : Style.text1 }
                        ]
                        delegate: GlassPanel {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 64
                            fillColor: Style.bg1
                            borderColor: Style.border1
                            Column {
                                anchors.fill: parent
                                anchors.margins: Style.s10
                                spacing: 2
                                Text { text: modelData.label; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                                Text { text: modelData.value; color: modelData.tone; font.family: Style.fontDisplay; font.pixelSize: Style.f20; font.weight: Style.w700 }
                            }
                        }
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(220, 92 + games.length * 92)
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s12
                SectionHeader { Layout.fillWidth: true; title: "Detected Games"; subtitle: "Optimize a running title or launch it with session tuning." }
                Repeater {
                    model: root.games
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        radius: Style.r12
                        color: Style.bg1
                        border.color: modelData.isRunning ? Style.cyan : Style.border1
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s12
                            anchors.rightMargin: Style.s12
                            spacing: Style.s12
                            Rectangle {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                radius: Style.r10
                                color: modelData.isRunning ? Style.cyanGlow : Style.bg3
                                Text { anchors.centerIn: parent; text: String(modelData.displayName).charAt(0); color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f20 }
                            }
                            Column {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: modelData.displayName; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f14; font.weight: Style.w600 }
                                Text { text: modelData.launcher + "  |  " + modelData.tweaksAvailable + " tweaks available"; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12 }
                                Text { text: modelData.isRunning ? "Running now" : (modelData.installPath || "Detected library entry"); color: modelData.isRunning ? Style.cyan : Style.text3; font.family: Style.fontMono; font.pixelSize: Style.f10; elide: Text.ElideRight; width: parent.width }
                            }
                            GlowButton { label: "Optimize"; glowColor: Style.cyan; onClicked: SystemCtrl.optimizeDetectedGame(modelData.executableName) }
                            GlowButton { label: "Launch & Optimize"; variant: "outlined"; glowColor: Style.amber; onClicked: SystemCtrl.launchOptimizedGame(modelData.executableName) }
                        }
                    }
                }
                Text {
                    visible: root.games.length === 0
                    text: "No supported games were detected yet. Launch a game once or install it in a standard library location, then refresh this page."
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f12
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s8
                SectionHeader { Layout.fillWidth: true; title: "Gaming Recommendations"; subtitle: "Recommendations currently relevant to active game performance." }
                Repeater {
                    model: Math.min(3, advisors.length)
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
                            Text { text: advisors[index].title; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: advisors[index].impact; color: advisors[index].impact === "high" ? Style.red : (advisors[index].impact === "medium" ? Style.amber : Style.cyan); font.family: Style.fontMono; font.pixelSize: Style.f11 }
                        }
                    }
                }
                Text {
                    visible: advisors.length === 0
                    text: "No gaming-specific advisor findings are currently active."
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f12
                }
            }
        }
    }
}
