import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../style"

ScrollView {
    id: root
    clip: true

    property var chips: []
    signal selected(string text)

    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    Row {
        spacing: Style.s8
        leftPadding: Style.s16
        rightPadding: Style.s16

        Repeater {
            model: root.chips
            delegate: Rectangle {
                required property var modelData
                height: 34
                width: chipLabel.implicitWidth + Style.s24
                radius: Style.r999
                color: chipArea.containsMouse ? Style.bg4 : Style.bg3
                border.color: chipArea.containsMouse ? Style.border2 : Style.border1
                border.width: 1

                Behavior on color { ColorAnimation { duration: Style.fast } }
                Behavior on border.color { ColorAnimation { duration: Style.fast } }

                Text {
                    id: chipLabel
                    anchors.centerIn: parent
                    text: modelData
                    font.family: Style.fontBody
                    font.pixelSize: Style.f12
                    color: chipArea.containsMouse ? Style.text0 : Style.text1
                }

                scale: chipArea.pressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 80 } }

                MouseArea {
                    id: chipArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.selected(modelData)
                }
            }
        }
    }
}

