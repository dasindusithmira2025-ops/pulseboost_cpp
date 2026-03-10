import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: navBtn
    width: 52
    height: 52

    required property string icon
    required property string label
    required property bool selected
    required property color accent
    required property color accentGlow
    required property color surface2
    required property color textMuted

    signal clicked

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: navBtn.selected ? navBtn.accentGlow : (hover.hovered ? navBtn.surface2 : "transparent")
        border.color: navBtn.selected ? navBtn.accent : "transparent"
        border.width: 1
    }

    Text {
        anchors.centerIn: parent
        text: navBtn.icon
        color: navBtn.selected ? navBtn.accent : navBtn.textMuted
        font.pixelSize: navBtn.icon.length > 1 ? 14 : 18
        font.bold: true
    }

    ToolTip.visible: hover.hovered
    ToolTip.text: navBtn.label
    ToolTip.delay: 250

    HoverHandler { id: hover }
    TapHandler { onTapped: navBtn.clicked() }
}
