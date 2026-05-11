import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/foundation"

Item {
    id: root
    anchors.fill: parent
    property string activeTab: "processes"

    ColumnLayout {
        anchors.fill: parent
        spacing: Style.s12

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s8
            Repeater {
                model: [
                    { id: "processes", label: "Processes" },
                    { id: "storage", label: "Storage" },
                    { id: "network", label: "Network" },
                    { id: "startup", label: "Startup" },
                    { id: "ram", label: "RAM" },
                    { id: "temps", label: "Thermals" }
                ]
                delegate: Rectangle {
                    required property var modelData
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 34
                    radius: Style.r10
                    color: root.activeTab === modelData.id ? Style.bg3 : Style.bg2
                    border.color: root.activeTab === modelData.id ? Style.border2 : Style.border1
                    border.width: 1
                    Text { anchors.centerIn: parent; text: modelData.label; color: root.activeTab === modelData.id ? Style.text0 : Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.activeTab = modelData.id }
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            asynchronous: true
            sourceComponent: root.activeTab === "processes" ? processesScreen
                            : root.activeTab === "storage" ? storageScreen
                            : root.activeTab === "network" ? networkScreen
                            : root.activeTab === "startup" ? startupScreen
                            : root.activeTab === "ram" ? ramScreen
                            : tempsScreen
        }
    }

    Component { id: processesScreen; ProcessManager {} }
    Component { id: storageScreen; StorageAnalyzer {} }
    Component { id: networkScreen; NetworkMonitor {} }
    Component { id: startupScreen; StartupManager {} }
    Component { id: ramScreen; RamOptimizer {} }
    Component { id: tempsScreen; Temps {} }
}
