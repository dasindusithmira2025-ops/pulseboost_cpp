import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/foundation"

GlassPanel {
    id: root
    panelRadius: Style.r16
    fillColor: "transparent"
    borderColor: "transparent"
    contentMargin: Style.s10

    property string title: "Home"
    property var notifications: SystemCtrl.notifications
    signal closeRequested()
    signal minRequested()
    signal maxRequested()

    RowLayout {
        anchors.fill: parent
        spacing: Style.s10

        Column {
            spacing: 0
            Text {
                text: root.title
                color: Style.text0
                font.family: Style.fontDisplay
                font.pixelSize: Style.f24
                font.weight: Style.w700
            }
            Row {
                spacing: Style.s8
                Rectangle { width: 7; height: 7; radius: 4; color: Style.green; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: "System Protected | Last scan: live telemetry"
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f11
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            Layout.preferredWidth: 340
            Layout.preferredHeight: 38
            radius: Style.r12
            color: Style.glassCard
            border.color: Style.border1
            border.width: 1

            TextField {
                anchors.fill: parent
                anchors.leftMargin: Style.s12
                anchors.rightMargin: Style.s12
                background: null
                color: Style.text0
                font.family: Style.fontBody
                font.pixelSize: Style.f13
                placeholderText: "Search pages or actions"
                placeholderTextColor: Style.text3
            }
        }

        Rectangle {
            Layout.preferredWidth: 92
            Layout.preferredHeight: 38
            radius: Style.r12
            color: Style.glassCard
            border.color: Style.border1
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: "Refresh"
                color: Style.text1
                font.family: Style.fontBody
                font.pixelSize: Style.f12
                font.weight: Style.w600
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: SystemCtrl.refreshAll()
            }
        }

        Rectangle {
            Layout.preferredWidth: 52
            Layout.preferredHeight: 38
            radius: Style.r12
            color: Style.glassCard
            border.color: Style.border1
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: root.notifications.length > 0 ? String(root.notifications.length) : "0"
                color: root.notifications.length > 0 ? Style.cyan : Style.text2
                font.family: Style.fontMono
                font.pixelSize: Style.f12
                font.weight: Style.w700
            }
        }

        TrafficLightWindowControls {
            Layout.alignment: Qt.AlignVCenter
            maximized: root.window ? (root.window.visibility === Window.Maximized) : false
            onCloseClicked: root.closeRequested()
            onMinClicked: root.minRequested()
            onMaxClicked: root.maxRequested()
        }
    }
}
