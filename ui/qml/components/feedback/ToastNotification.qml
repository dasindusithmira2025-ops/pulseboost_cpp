import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../style"

Rectangle {
    id: root
    property string title: "Info"
    property string message: ""
    property string tone: "info"
    signal dismissed()

    radius: Style.r12
    color: Style.bg2
    border.color: toneColor()
    border.width: 1
    implicitWidth: 360
    implicitHeight: Style.s64

    function toneColor() {
        if (tone === "success") return Style.success
        if (tone === "warning") return Style.warning
        if (tone === "error") return Style.danger
        return Style.violet
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Style.s12
        spacing: Style.s8

        Rectangle {
            Layout.preferredWidth: Style.s4
            Layout.fillHeight: true
            radius: Style.r4
            color: root.toneColor()
        }

        ColumnLayout {
            Layout.fillWidth: true
            Text {
                text: root.title
                color: Style.text0
                font.family: Style.fontBody
                font.pixelSize: Style.f13
                font.weight: Style.w600
            }
            Text {
                text: root.message
                color: Style.text2
                font.family: Style.fontBody
                font.pixelSize: Style.f12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        ToolButton {
            text: "\u2715"
            onClicked: root.dismissed()
        }
    }
}
