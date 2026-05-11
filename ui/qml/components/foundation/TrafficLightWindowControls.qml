import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../style"

FocusScope {
    id: root
    signal closeClicked()
    signal minClicked()
    signal maxClicked()
    property bool maximized: false

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    Row {
        id: row
        spacing: Style.s8

        Repeater {
            model: [
                { key: "min", label: "-", hint: "Minimize" },
                { key: "max", label: root.maximized ? "[]" : "+", hint: root.maximized ? "Restore" : "Maximize" },
                { key: "close", label: "x", hint: "Close" }
            ]

            delegate: Rectangle {
                required property var modelData
                width: 34
                height: 28
                radius: Style.r8
                color: area.containsMouse ? Style.bg3 : Style.bg2
                border.color: modelData.key === "close" ? Style.red : Style.border1
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: modelData.label
                    color: modelData.key === "close" ? Style.red : Style.text1
                    font.family: Style.fontMono
                    font.pixelSize: Style.f12
                    font.weight: Style.w700
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (modelData.key === "close") root.closeClicked()
                        else if (modelData.key === "min") root.minClicked()
                        else root.maxClicked()
                    }
                }
            }
        }
    }
}
