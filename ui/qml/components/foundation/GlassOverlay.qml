import QtQuick 2.15
import "../../style"

Rectangle {
    id: root
    default property alias content: body.data
    color: Qt.rgba(0, 0, 0, 0.52)
    property color overlayColor: Qt.rgba(0, 0, 0, 0.52)
    property real contentMargin: Style.s24

    anchors.fill: parent
    visible: false
    opacity: visible ? 1 : 0

    Behavior on opacity { NumberAnimation { duration: Style.normal } }

    Rectangle {
        anchors.fill: parent
        color: root.overlayColor
    }

    Item {
        id: body
        anchors.fill: parent
        anchors.margins: root.contentMargin
    }
}
