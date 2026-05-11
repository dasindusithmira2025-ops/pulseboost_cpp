import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../style"

Rectangle {
    id: root
    property string iconText: "\u25A1"
    property bool danger: false
    signal clicked()

    implicitWidth: Style.s32
    implicitHeight: Style.s32
    radius: Style.r8
    color: hover.hovered ? (danger ? Qt.rgba(Style.red.r, Style.red.g, Style.red.b, 0.2) : Style.bg4) : Style.bg3
    border.color: danger ? Style.red : Style.border0
    border.width: 1
    opacity: root.enabled ? 1 : 0.45
    scale: tap.pressed ? Style.pressScale : 1
    Behavior on scale { NumberAnimation { duration: Style.fast } }
    Behavior on color { ColorAnimation { duration: Style.fast } }

    Text {
        anchors.centerIn: parent
        text: root.iconText
        color: danger ? Style.red : Style.text1
        font.family: Style.fontMono
        font.pixelSize: Style.f14
    }

    HoverHandler { id: hover; enabled: root.enabled }
    TapHandler { id: tap; enabled: root.enabled; onTapped: root.clicked() }
}
