import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"

Rectangle {
    id: root
    color: Style.sidebarBackground
    border.color: Style.sidebarBorder
    border.width: 1

    property string currentScreen: "home"
    signal screenSelected(string screenId)

    readonly property var navItems: [
        { "id": "home", "label": "Overview", "icon": "dashboard" },
        { "id": "action-center", "label": "Action Center", "icon": "bolt" },
        { "id": "ai-advisor", "label": "AI Advisor", "icon": "sparkles" },
        { "id": "before-after", "label": "Before / After Proof", "icon": "chart" },
        { "id": "audit-log", "label": "Audit Log", "icon": "file" },
        { "id": "restore-center", "label": "Restore Center", "icon": "restore" },
        { "id": "processes", "label": "Processes", "icon": "activity" },
        { "id": "startup-apps", "label": "Startup Apps", "icon": "rocket" },
        { "id": "storage-cleanup", "label": "Storage Cleanup", "icon": "storage" },
        { "id": "network-tools", "label": "Network Tools", "icon": "wifi" },
        { "id": "settings", "label": "Settings", "icon": "settings" }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: navColumn.implicitHeight
            clip: true

            ColumnLayout {
                id: navColumn
                width: parent.width
                spacing: 4

                Repeater {
                    model: root.navItems
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 8
                        color: root.currentScreen === modelData.id ? Style.sidebarPrimary : (hover.hovered ? Style.sidebarAccent : "transparent")

                        Behavior on color { ColorAnimation { duration: Style.fast } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 12

                            Text {
                                text: Icons.glyph(modelData.icon)
                                color: root.currentScreen === modelData.id ? Style.sidebarPrimaryText : Style.sidebarMutedText
                                font.family: Style.fontDisplay
                                font.pixelSize: 20
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                color: root.currentScreen === modelData.id ? Style.sidebarPrimaryText : Style.sidebarText
                                font.family: Style.fontBody
                                font.pixelSize: 14
                                font.weight: Style.w500
                                elide: Text.ElideRight
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        HoverHandler { id: hover }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.screenSelected(modelData.id)
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            radius: 8
            color: Style.sidebarAccent

            Column {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                Text {
                    text: "Version"
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: 12
                }

                Text {
                    text: FeatureGate.tierLabel.length > 0 ? ("2.5.0 " + FeatureGate.tierLabel) : "2.5.0 Pro"
                    color: Style.sidebarText
                    font.family: Style.fontBody
                    font.pixelSize: 14
                    font.weight: Style.w500
                    elide: Text.ElideRight
                    width: parent.width
                }
            }
        }
    }
}
