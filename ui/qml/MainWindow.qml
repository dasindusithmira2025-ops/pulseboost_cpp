import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 1480
    height: 920
    minimumWidth: 1180
    minimumHeight: 760
    title: "PulseBoost AI"
    color: "#08111f"

    readonly property color bg: "#08111f"
    readonly property color bg2: "#0d1728"
    readonly property color surface: "#111c2e"
    readonly property color surfaceRaised: "#16243b"
    readonly property color border: "#24334d"
    readonly property color accent: "#56b2ff"
    readonly property color accentGlow: "#163657"
    readonly property color text: "#eef5ff"
    readonly property color textMuted: "#91a4bf"
    readonly property color success: "#3fb950"
    readonly property color danger: "#f85149"

    property var navItems: [
        { "key": "dashboard", "label": "Overview", "icon": "OV" },
        { "key": "charts", "label": "Trends", "icon": "TR" },
        { "key": "processes", "label": "Processes", "icon": "PS" },
        { "key": "storage", "label": "Storage", "icon": "DS" },
        { "key": "chat", "label": "AI Agent", "icon": "AI" }
    ]

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0a1322" }
            GradientStop { position: 1.0; color: "#070d18" }
        }
    }

    Rectangle {
        width: 460
        height: 460
        radius: 230
        x: root.width - width * 0.65
        y: -height * 0.35
        color: "#12335d"
        opacity: 0.22
    }

    Rectangle {
        width: 320
        height: 320
        radius: 160
        x: -width * 0.35
        y: root.height - height * 0.6
        color: "#0f6a58"
        opacity: 0.14
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 18

        Rectangle {
            Layout.preferredWidth: 108
            Layout.fillHeight: true
            radius: 28
            color: root.surface
            border.color: root.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 64
                    height: 64
                    radius: 22
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#0f62fe" }
                        GradientStop { position: 1.0; color: "#20c997" }
                    }
                    border.color: "#5cc8ff"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 0

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "PB"
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "AI"
                            color: "#dbeafe"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "PulseBoost"
                    color: root.text
                    font.pixelSize: 12
                    font.bold: true
                }

                Repeater {
                    model: root.navItems.length

                    delegate: Item {
                        required property int index
                        Layout.alignment: Qt.AlignHCenter
                        width: 52
                        height: 52

                        NavButton {
                            property var itemData: root.navItems[parent.index] || ({})
                            anchors.fill: parent
                            icon: itemData.icon || ""
                            label: itemData.label || ""
                            accent: root.accent
                            accentGlow: root.accentGlow
                            surface2: root.surfaceRaised
                            textMuted: root.textMuted
                            selected: stack.currentIndex === parent.index
                            onClicked: stack.currentIndex = parent.index
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 72
                    height: 72
                    radius: 18
                    color: root.surfaceRaised
                    border.color: root.border
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: SystemCtrl.healthScore + ""
                            color: root.text
                            font.pixelSize: 24
                            font.bold: true
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "health"
                            color: root.textMuted
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 30
            color: "#0b1524"
            border.color: root.border
            border.width: 1
            clip: true

            StackLayout {
                id: stack
                anchors.fill: parent
                currentIndex: 0

                Dashboard {}
                Charts {}
                ProcessTable {}
                StorageMap {}
                ChatPanel {}
            }
        }
    }

    Rectangle {
        id: toast
        width: Math.min(480, toastText.implicitWidth + 42)
        height: 52
        radius: 14
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        color: toastSuccess ? "#0d2a1a" : "#351013"
        border.color: toastSuccess ? root.success : root.danger
        border.width: 1
        opacity: 0
        visible: opacity > 0

        property bool toastSuccess: true

        Text {
            id: toastText
            anchors.centerIn: parent
            color: root.text
            font.pixelSize: 14
            font.bold: true
        }

        Behavior on opacity {
            NumberAnimation { duration: 180 }
        }

        function showMessage(message, success) {
            toastText.text = message
            toastSuccess = success
            opacity = 1
            hideTimer.restart()
        }

        Timer {
            id: hideTimer
            interval: 3200
            onTriggered: toast.opacity = 0
        }
    }

    Connections {
        target: SystemCtrl

        function onActionFeedback(message, success) {
            toast.showMessage(message, success)
        }
    }
}
