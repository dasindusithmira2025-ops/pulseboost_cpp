import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/foundation"

GlassPanel {
    id: root
    fillColor: Style.bg1
    borderColor: Style.border0
    panelRadius: Style.r20
    contentMargin: Style.s12

    property bool collapsed: false
    property string currentScreen: "home"
    signal screenSelected(string screenId)
    signal collapseToggled()

    readonly property var navItems: [
        { "id": "home", "label": "Overview", "icon": "dashboard" },
        { "id": "action-center", "label": "Action Center", "icon": "bolt" },
        { "id": "ai-advisor", "label": "AI Advisor", "icon": "chat" },
        { "id": "before-after", "label": "Before / After Proof", "icon": "history" },
        { "id": "audit-log", "label": "Audit Log", "icon": "history" },
        { "id": "restore-center", "label": "Restore Center", "icon": "history" },
        { "id": "processes", "label": "Processes", "icon": "settings" },
        { "id": "startup-apps", "label": "Startup Apps", "icon": "storage" },
        { "id": "storage-cleanup", "label": "Storage Cleanup", "icon": "storage" },
        { "id": "network-tools", "label": "Network Tools", "icon": "settings" },
        { "id": "settings", "label": "Settings", "icon": "settings" }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: Style.s12

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            spacing: Style.s12

            Rectangle {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                radius: Style.r10
                color: Style.cyan
                Text {
                    anchors.centerIn: parent
                    text: "PB"
                    color: Style.bg0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f18
                    font.weight: Style.w700
                }
            }

            Column {
                visible: !root.collapsed
                spacing: 0
                Text {
                    text: "PulseBoost"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f24
                    font.weight: Style.w700
                }
                Text {
                    text: "Windows-native safety suite"
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f11
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: root.collapsed ? 96 : 112
            fillColor: Style.bg2
            borderColor: Style.border1
            contentMargin: Style.s12

            Column {
                anchors.fill: parent
                spacing: Style.s6
                Text {
                    visible: !root.collapsed
                    text: "System State"
                    color: Style.text2
                    font.family: Style.fontMono
                    font.pixelSize: Style.f10
                    font.letterSpacing: 1
                }
                Text {
                    text: "Health " + Number(SystemCtrl.healthScore).toFixed(0)
                    color: Style.healthColor(SystemCtrl.healthScore)
                    font.family: Style.fontDisplay
                    font.pixelSize: root.collapsed ? Style.f20 : Style.f28
                    font.weight: Style.w700
                }
                Text {
                    visible: !root.collapsed
                    text: FeatureGate.tierLabel + "  |  CPU " + Number(SystemCtrl.cpuUsage).toFixed(0) + "%"
                    color: Style.text1
                    font.family: Style.fontBody
                    font.pixelSize: Style.f12
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Style.s6

            Repeater {
                model: root.navItems
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 46
                    radius: Style.r12
                    color: root.currentScreen === modelData.id ? Style.cyanGlow : "transparent"
                    border.width: 1
                    border.color: root.currentScreen === modelData.id ? Style.borderAccentSoft : "transparent"

                    Behavior on color { ColorAnimation { duration: Animations.fast } }
                    Behavior on border.color { ColorAnimation { duration: Animations.fast } }

                    // Glowing active line
                    Rectangle {
                        width: 4
                        height: 24
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: -1
                        radius: 2
                        color: Style.cyan
                        visible: root.currentScreen === modelData.id
                    }

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: Style.s14
                        spacing: Style.s12

                        Text {
                            text: Icons.glyph(modelData.icon)
                            color: root.currentScreen === modelData.id ? Style.cyan : Style.text2
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f18
                        }

                        Text {
                            visible: !root.collapsed
                            text: modelData.label
                            color: root.currentScreen === modelData.id ? Style.text0 : Style.text1
                            font.family: Style.fontBody
                            font.pixelSize: Style.f14
                            font.weight: root.currentScreen === modelData.id ? Style.w600 : Style.w500
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.screenSelected(modelData.id)
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 74
            fillColor: Style.bg2
            borderColor: Style.border1
            contentMargin: Style.s12

            Column {
                anchors.fill: parent
                spacing: Style.s4
                Text {
                    visible: !root.collapsed
                    text: "Assistant"
                    color: Style.text2
                    font.family: Style.fontMono
                    font.pixelSize: Style.f10
                }
                Text {
                    text: SystemCtrl.aiMode === "cloud" ? "Cloud advisory" : "Local advisory"
                    color: Style.text0
                    font.family: Style.fontBody
                    font.pixelSize: Style.f13
                    font.weight: Style.w600
                }
                Text {
                    visible: !root.collapsed
                    text: SystemCtrl.aiCloudConfigured ? "AI can advise only" : "Private local mode"
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f11
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            radius: Style.r10
            color: Style.bg2
            border.color: Style.border1
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: root.collapsed ? ">" : "Collapse"
                color: Style.text1
                font.family: Style.fontBody
                font.pixelSize: Style.f12
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.collapseToggled()
            }
        }
    }
}
