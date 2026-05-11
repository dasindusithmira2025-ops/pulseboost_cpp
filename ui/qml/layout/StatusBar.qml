import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/foundation"

GlassPanel {
    id: root
    property string currentScreen: "Home"
    property bool processing: false

    fillColor: "transparent"
    borderColor: "transparent"
    panelRadius: Style.r8
    contentMargin: Style.s8

    RowLayout {
        anchors.fill: parent
        spacing: Style.s12

        Text {
            text: root.processing ? "Working" : "Ready"
            font.family: Style.fontMono
            font.pixelSize: Style.f10
            color: root.processing ? Style.cyan : Style.text2
        }

        Rectangle { width: 1; height: 12; color: Style.border1 }

        Text {
            text: root.currentScreen
            font.family: Style.fontMono
            font.pixelSize: Style.f10
            color: Style.text3
        }

        Item { Layout.fillWidth: true }

        Text {
            text: "CPU " + Number(SystemCtrl.cpuUsage).toFixed(0) + "%   RAM " + Number(SystemCtrl.ramUsage).toFixed(0) + "%"
            font.family: Style.fontMono
            font.pixelSize: Style.f10
            color: Style.text2
        }

        StatusPill {
            text: "Health " + Number(SystemCtrl.healthScore).toFixed(0)
            tone: SystemCtrl.healthScore >= 65 ? "success" : (SystemCtrl.healthScore >= 45 ? "warning" : "error")
        }

        Text {
            id: clock
            text: Qt.formatDateTime(new Date(), "HH:mm")
            font.family: Style.fontMono
            font.pixelSize: Style.f10
            color: Style.text3
            Timer {
                interval: 30000
                running: true
                repeat: true
                onTriggered: clock.text = Qt.formatDateTime(new Date(), "HH:mm")
            }
        }
    }
}
