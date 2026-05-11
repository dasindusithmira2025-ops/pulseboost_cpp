import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../style"

ColumnLayout {
    id: root
    property string title: ""
    property string subtitle: ""
    default property alias trailing: trailingContainer.data

    spacing: Style.s4

    RowLayout {
        Layout.fillWidth: true
        spacing: Style.s8

        Text {
            text: root.title
            color: Style.text0
            font.family: Style.fontDisplay
            font.pixelSize: Style.f20
            font.weight: Style.w700
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Item {
            id: trailingContainer
            Layout.alignment: Qt.AlignVCenter
        }
    }

    Text {
        visible: subtitle.length > 0
        text: root.subtitle
        color: Style.text2
        font.family: Style.fontBody
        font.pixelSize: Style.f12
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
