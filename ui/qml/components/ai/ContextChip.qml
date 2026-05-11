import QtQuick 2.15
import "../../style"

Rectangle {
    id: root
    property real cpu: 0
    property real ram: 0
    property real health: 0

    height: 32
    width: chipRow.implicitWidth + Style.s20
    radius: Style.r8
    color: Style.bg3
    border.color: Style.border1
    border.width: 1

    Row {
        id: chipRow
        anchors.centerIn: parent
        spacing: Style.s12

        Row {
            spacing: Style.s4
            Text {
                text: "CPU"
                font.family: Style.fontMono
                font.pixelSize: Style.f10
                color: Style.text2
            }
            Text {
                text: Number(root.cpu).toFixed(0) + "%"
                font.family: Style.fontMono
                font.pixelSize: Style.f11
                font.weight: Style.w500
                color: Style.usageColor(root.cpu)
            }
        }

        Rectangle { width: 1; height: 14; color: Style.border1 }

        Row {
            spacing: Style.s4
            Text {
                text: "RAM"
                font.family: Style.fontMono
                font.pixelSize: Style.f10
                color: Style.text2
            }
            Text {
                text: Number(root.ram).toFixed(0) + "%"
                font.family: Style.fontMono
                font.pixelSize: Style.f11
                font.weight: Style.w500
                color: Style.usageColor(root.ram)
            }
        }

        Rectangle { width: 1; height: 14; color: Style.border1 }

        Row {
            spacing: Style.s4
            Text {
                text: "Health"
                font.family: Style.fontMono
                font.pixelSize: Style.f10
                color: Style.text2
            }
            Text {
                text: Number(root.health).toFixed(0)
                font.family: Style.fontMono
                font.pixelSize: Style.f11
                font.weight: Style.w500
                color: Style.healthColor(root.health)
            }
        }
    }
}

