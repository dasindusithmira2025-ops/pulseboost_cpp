import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../style"

Rectangle {
    id: root
    property string text: ""
    property string tone: "neutral"

    radius: Style.r999
    implicitHeight: 22
    implicitWidth: label.implicitWidth + Style.s12
    Layout.minimumWidth: implicitWidth
    Layout.minimumHeight: implicitHeight

    color: tone === "success" ? Style.greenGlow
         : tone === "warning" ? Style.amberGlow
         : tone === "error" ? Style.redGlow
         : Style.bg3

    border.width: 1
    border.color: tone === "success" ? Style.success
                : tone === "warning" ? Style.warning
                : tone === "error" ? Style.danger
                : Style.border1

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: tone === "success" ? Style.success
             : tone === "warning" ? Style.warning
             : tone === "error" ? Style.danger
             : Style.text2
        font.family: Style.fontMono
        font.pixelSize: Style.f11
    }
}
