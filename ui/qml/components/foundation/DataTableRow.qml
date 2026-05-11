import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../style"

Rectangle {
    id: root
    default property alias content: row.data

    property bool highlighted: false

    radius: Style.r8
    color: highlighted ? Style.bg3 : Style.bg2
    border.width: 1
    border.color: highlighted ? Style.borderAccentSoft : Style.border0

    Behavior on color { ColorAnimation { duration: Style.fast } }
    Behavior on border.color { ColorAnimation { duration: Style.fast } }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Style.s8
        spacing: Style.s8
    }
}
