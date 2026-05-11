import QtQuick 2.15
import "../../style"

Item {
    id: root
    property bool running: true
    width: 56
    height: 36

    Rectangle {
        anchors.fill: parent
        radius: Style.r12
        color: Style.bg3
        border.color: Style.border1
        border.width: 1

        Rectangle {
            width: 3
            height: 20
            radius: 2
            color: Style.violet
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }

        Row {
            anchors.centerIn: parent
            spacing: Style.s6

            Repeater {
                model: 3
                delegate: Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: Style.violet

                    SequentialAnimation on y {
                        running: root.running
                        loops: Animation.Infinite
                        PauseAnimation { duration: index * 150 }
                        NumberAnimation { to: -5; duration: 300; easing.type: Easing.OutCubic }
                        NumberAnimation { to: 0; duration: 300; easing.type: Easing.InCubic }
                        PauseAnimation { duration: 300 - (index * 150) }
                    }
                }
            }
        }
    }
}

