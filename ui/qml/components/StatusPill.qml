import QtQuick 2.15
import "../theme"

Rectangle {
    id: root
    property string text: "Status"
    property color tone: Theme.accentCyan
    implicitWidth: label.implicitWidth + 20
    implicitHeight: 24
    radius: 12
    color: Qt.rgba(tone.r, tone.g, tone.b, 0.12)
    border.color: Qt.rgba(tone.r, tone.g, tone.b, 0.38)
    border.width: 1
    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.tone
        font.family: Typography.body
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }
}
