import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../style"

Rectangle {
    id: root
    property alias text: input.text
    property alias placeholderText: input.placeholderText
    property bool focused: input.activeFocus
    property bool enabledInput: true

    radius: Style.r10
    color: Style.bg2
    border.width: 1
    border.color: focused ? Style.cyan : Style.border0

    Behavior on border.color { ColorAnimation { duration: Animations.fast } }

    TextField {
        id: input
        anchors.fill: parent
        anchors.leftMargin: Style.s12
        anchors.rightMargin: Style.s12
        enabled: root.enabledInput
        color: Style.text1
        font.family: Style.fontBody
        font.pixelSize: Style.f13
        selectedTextColor: Style.text0
        selectionColor: Style.cyanGlow
        placeholderTextColor: Style.text3
        background: null
    }
}
