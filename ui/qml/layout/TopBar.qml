import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/foundation"

Rectangle {
    id: root
    color: Style.topBarBackground
    border.color: Style.borderSubtle
    border.width: 1

    property string title: "PulseBoost AI"
    signal closeRequested()
    signal minRequested()
    signal maxRequested()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 14
        spacing: 12

        Text {
            text: Icons.glyph("shield")
            color: Style.primary
            font.family: Style.fontDisplay
            font.pixelSize: 24
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: root.title
            color: Style.text0
            font.family: Style.fontBody
            font.pixelSize: 18
            font.weight: Style.w500
            Layout.alignment: Qt.AlignVCenter
        }

        Item { Layout.fillWidth: true }

        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: Style.green
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.45; duration: 900 }
                    NumberAnimation { to: 1.0; duration: 900 }
                }
            }

            Text {
                text: "System Protected"
                color: Style.text2
                font.family: Style.fontBody
                font.pixelSize: 13
            }
        }

        Text {
            text: "Last scan: live telemetry"
            color: Style.text2
            font.family: Style.fontBody
            font.pixelSize: 13
            Layout.alignment: Qt.AlignVCenter
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
