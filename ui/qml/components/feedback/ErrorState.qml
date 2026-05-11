import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../style"
import "../controls"

Rectangle {
    id: root
    property string title: "Something went wrong"
    property string subtitle: "Please retry this action."
    signal retry()

    color: Style.bg2
    radius: Style.r12
    border.color: Style.danger
    border.width: 1

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Style.s10
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.title
            color: Style.text0
            font.family: Style.fontBody
            font.pixelSize: Style.f14
            font.weight: Style.w600
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.subtitle
            color: Style.text2
            font.family: Style.fontBody
            font.pixelSize: Style.f12
        }
        GlowButton {
            Layout.alignment: Qt.AlignHCenter
            label: "Retry"
            glowColor: Style.red
            onClicked: root.retry()
        }
    }
}
