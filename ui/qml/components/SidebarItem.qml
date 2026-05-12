import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

Rectangle {
    id: root
    property string label: "Item"
    property bool active: false
    signal clicked()
    implicitHeight: 44
    radius: Theme.radiusLarge
    color: active ? Theme.accentCyan : "transparent"
    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: Spacing.md
        text: root.label
        color: root.active ? Theme.appBackground : Theme.textSecondary
        font.family: Typography.body
        font.pixelSize: 14
        font.weight: root.active ? Font.DemiBold : Font.Medium
    }
    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.clicked() }
}
