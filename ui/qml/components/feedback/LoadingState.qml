import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../style"

Rectangle {
    id: root
    property string title: "Loading telemetry"
    property string subtitle: ""
    color: Style.bg2
    radius: Style.r12
    border.color: Style.border0
    border.width: 1

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Style.s10
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: true
            width: Style.s24
            height: Style.s24
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.title
            color: Style.text1
            font.family: Style.fontBody
            font.pixelSize: Style.f13
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: root.subtitle.length > 0
            text: root.subtitle
            color: Style.text3
            font.family: Style.fontBody
            font.pixelSize: Style.f12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.maximumWidth: 320
        }
    }
}
