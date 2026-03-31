import QtQuick 2.15
import "../../style"

Rectangle {
    id: root
    default property alias content: body.data

    property bool interactive: false
    property bool hovered: false
    property bool pressed: false
    property bool liftOnHover: false
    property color fillColor: Style.bg2
    property color strokeColor: Style.border0
    property real panelRadius: Style.r16
    property real contentMargin: Style.s12

    radius: panelRadius
    color: fillColor
    border.color: interactive && hovered ? Style.borderAccentSoft : strokeColor
    border.width: 1
    y: interactive && liftOnHover && hovered ? Style.hoverLift : 0
    scale: interactive && pressed ? Style.pressScale : 1

    Behavior on y { NumberAnimation { duration: Style.fast; easing.type: Easing.OutCubic } }
    Behavior on scale { NumberAnimation { duration: Style.fast; easing.type: Easing.OutCubic } }
    Behavior on border.color { ColorAnimation { duration: Style.fast } }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: Math.max(1, parent.height * 0.22)
        radius: parent.radius
        color: Style.glassHigh
        opacity: 0.22
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Math.max(1, parent.height * 0.3)
        radius: parent.radius
        color: Style.glassLow
        opacity: 0.22
    }

    Item {
        id: body
        anchors.fill: parent
        anchors.margins: contentMargin
    }

    HoverHandler {
        id: hoverHandler
        enabled: root.interactive
        onHoveredChanged: root.hovered = hovered
    }

    TapHandler {
        id: tapHandler
        enabled: root.interactive
        onPressedChanged: root.pressed = pressed
    }
}

