import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../style"

Rectangle {
    id: root
    property string title: "No data"
    property string subtitle: "Telemetry is still loading."

    color: Style.bg2
    radius: Style.r12
    border.color: Style.border0
    border.width: 1

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Style.s8
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.title
            color: Style.text1
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
    }
}
