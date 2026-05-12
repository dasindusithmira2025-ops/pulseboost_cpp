import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

Rectangle {
    id: root
    property string level: "safe"
    property string size: "md"
    readonly property color toneColor: Theme.riskColor(level)

    implicitWidth: row.implicitWidth + (size === "sm" ? 16 : 22)
    implicitHeight: size === "sm" ? 22 : 28
    radius: implicitHeight / 2
    color: Qt.rgba(toneColor.r, toneColor.g, toneColor.b, 0.12)
    border.color: Qt.rgba(toneColor.r, toneColor.g, toneColor.b, 0.42)
    border.width: 1

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 7
        Rectangle {
            width: 6
            height: 6
            radius: 3
            color: root.toneColor
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: Theme.riskLabel(root.level)
            color: root.toneColor
            font.family: Typography.body
            font.pixelSize: root.size === "sm" ? 11 : 13
            font.weight: Font.DemiBold
        }
    }
}
