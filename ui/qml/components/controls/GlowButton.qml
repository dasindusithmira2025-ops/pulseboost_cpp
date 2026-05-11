import QtQuick 2.15
import "../../style"

Item {
    id: root
    property string label: "Action"
    property string icon: ""
    property color glowColor: Style.cyan
    property string variant: "filled"
    property bool loading: false
    property bool danger: false

    signal clicked()

    implicitHeight: 40
    implicitWidth: 148
    opacity: enabled ? 1.0 : 0.45

    HoverHandler {
        id: hover
        enabled: root.enabled && !root.loading
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        id: press
        enabled: root.enabled && !root.loading
        onTapped: root.clicked()
    }

    scale: press.pressed ? 0.96 : (hover.hovered ? 1.02 : 1.0)
    Behavior on scale { NumberAnimation { duration: Animations.fast; easing.type: Animations.easeSpring } }

    // Fake Glow Shadow
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 4
        anchors.bottomMargin: -4
        anchors.leftMargin: 2
        anchors.rightMargin: -2
        radius: Style.r12
        color: root.variant === "filled" ? Qt.rgba(root.glowColor.r, root.glowColor.g, root.glowColor.b, 0.4) : "transparent"
        visible: root.variant === "filled"
        opacity: hover.hovered ? 0.8 : 0.5
        Behavior on opacity { NumberAnimation { duration: Animations.fast } }
    }

    Rectangle {
        anchors.fill: parent
        radius: Style.r12
        color: {
            if (root.variant === "ghost") return hover.hovered ? Style.bg3 : "transparent"
            if (root.variant === "outlined") return hover.hovered ? Style.bg3 : Style.bg2
            return hover.hovered ? Qt.lighter(root.glowColor, 1.1) : root.glowColor
        }
        border.width: root.variant === "outlined" ? 1 : 0
        border.color: root.variant === "outlined" ? root.glowColor : "transparent"

        Behavior on color { ColorAnimation { duration: Animations.fast } }

        // Top edge glossy reflection
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Style.r12 - 1
            color: "transparent"
            border.color: root.variant === "filled" ? "#2AFFFFFF" : "transparent"
            border.width: 1
            clip: true
        }
    }

    Row {
        anchors.centerIn: parent
        spacing: Style.s8

        Text {
            visible: root.icon !== "" && !root.loading
            text: root.icon
            color: root.variant === "filled" ? Style.bg0 : root.glowColor
            font.family: Style.fontDisplay
            font.pixelSize: Style.f16
        }

        Text {
            visible: root.loading
            text: "..."
            color: root.variant === "filled" ? Style.bg0 : root.glowColor
            font.family: Style.fontMono
            font.pixelSize: Style.f14
        }

        Text {
            text: root.label
            color: root.variant === "filled" ? Style.bg0 : root.glowColor
            font.family: Style.fontBody
            font.pixelSize: Style.f13
            font.weight: Style.w600
        }
    }
}
