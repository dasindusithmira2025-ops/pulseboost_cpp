import QtQuick 2.15
import "../../style"

Item {
    id: root
    default property alias content: body.data

    property bool interactive: false
    property bool liftOnHover: false
    property color fillColor: Style.glassPanel
    property color borderColor: Style.borderGlass
    property real panelRadius: Style.r16
    property real contentMargin: Style.s16
    property real blurStrength: 0

    signal clicked()

    readonly property bool hovered: hover.hovered
    readonly property bool pressed: press.pressed

    implicitWidth: 200
    implicitHeight: 160

    HoverHandler {
        id: hover
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        id: press
        enabled: root.interactive
        onTapped: root.clicked()
    }

    // Drop Shadow Simulation
    Rectangle {
        anchors.fill: parent
        anchors.margins: -1  // slightly larger
        anchors.topMargin: 2
        anchors.bottomMargin: -6
        anchors.leftMargin: 2
        anchors.rightMargin: -2
        radius: root.panelRadius
        color: "#18000000" // soft dark shadow
        visible: root.interactive || root.liftOnHover
        Behavior on anchors.bottomMargin { NumberAnimation { duration: Animations.fast; easing.type: Animations.easeStandard } }
    }

    // Main Glass Panel
    Rectangle {
        id: bgRect
        anchors.fill: parent
        anchors.topMargin: root.interactive && root.hovered && root.liftOnHover ? -2 : 0
        radius: root.panelRadius
        color: root.interactive && root.hovered ? Style.glassHover : root.fillColor
        border.color: root.interactive && root.hovered ? Style.border2 : root.borderColor
        border.width: 1
        
        Behavior on anchors.topMargin { NumberAnimation { duration: Animations.fast; easing.type: Animations.easeStandard } }
        Behavior on color { ColorAnimation { duration: Animations.fast; easing.type: Animations.easeStandard } }
        Behavior on border.color { ColorAnimation { duration: Animations.fast; easing.type: Animations.easeStandard } }

        // Inner glossy rim (Glass edge simulation)
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: root.panelRadius - 1
            color: "transparent"
            border.color: "#1AFFFFFF" // Soft white top edge
            border.width: 1
            clip: true
            Rectangle {
                width: parent.width
                height: 1
                color: "#2DFFFFFF"
                anchors.top: parent.top
            }
        }
    }

    Item {
        id: body
        anchors.fill: parent
        anchors.margins: root.contentMargin
    }
}
