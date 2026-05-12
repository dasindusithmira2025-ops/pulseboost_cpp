import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/feedback"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.s32

    property string sortKey: "cpu"
    property bool descending: true
    property string query: ""
    property var rows: []
    property var pendingProcess: null

    function refreshRows() {
        const baseRows = SystemCtrl.sortedProcesses(sortKey, descending)
        if (!query || query.trim() === "") {
            rows = baseRows
            return
        }
        const needle = query.toLowerCase()
        rows = baseRows.filter(function(row) {
            return String(row.name).toLowerCase().indexOf(needle) !== -1
        })
    }

    function riskColor(risk) {
        if (risk >= 3) return Style.red
        if (risk === 2) return Style.amber
        return Style.green
    }

    function riskGlow(risk) {
        if (risk >= 3) return Style.redGlow
        if (risk === 2) return Style.amberGlow
        return Style.greenGlow
    }

    Component.onCompleted: refreshRows()

    Connections {
        target: SystemCtrl
        function onProcessListChanged() { root.refreshRows() }
    }

    ColumnLayout {
        id: contentColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 100

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.s12
                    Text {
                        text: "Task & Process Intelligence"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }
                    Item { Layout.fillWidth: true }
                    GlowButton {
                        label: "Scan Now"
                        variant: "outlined"
                        glowColor: Style.cyan
                        onClicked: root.refreshRows()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.s16

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: Style.bg2
                        radius: Style.r8
                        border.color: searchInput.activeFocus ? Style.cyan : Style.border1
                        Behavior on border.color { ColorAnimation { duration: Animations.fast } }

                        TextInput {
                            id: searchInput
                            anchors.fill: parent
                            anchors.leftMargin: Style.s12
                            anchors.rightMargin: Style.s12
                            verticalAlignment: TextInput.AlignVCenter
                            color: Style.text0
                            font.family: Style.fontBody
                            font.pixelSize: Style.f13
                            selectByMouse: true
                            onTextChanged: { root.query = text; root.refreshRows() }
                            
                            Text {
                                text: "Search process namespace..."
                                color: Style.text3
                                font.family: Style.fontBody
                                font.pixelSize: Style.f13
                                visible: !searchInput.text && !searchInput.activeFocus
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }

                    RowLayout {
                        spacing: Style.s8
                        Rectangle { width: 8; height: 8; radius: 4; color: Style.red; anchors.verticalCenter: parent.verticalCenter }
                        Text {
                            text: "End Process is High Risk and requires manual confirmation."
                            color: Style.text2
                            font.family: Style.fontBody
                            font.pixelSize: Style.f12
                        }
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            contentMargin: 0
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Style.s16
                anchors.rightMargin: Style.s16
                spacing: Style.s12

                HeaderCell { label: "PROCESS NAME"; isFill: true; onPressed: sort("name") }
                HeaderCell { label: "CPU CORE %"; widthPx: 100; onPressed: sort("cpu") }
                HeaderCell { label: "RAM USAGE"; widthPx: 100; onPressed: sort("ram") }
                HeaderCell { label: "RISK LEVEL"; widthPx: 120; onPressed: sort("cpu") }
                HeaderCell { label: "SUSPEND"; widthPx: 110; onPressed: sort("cpu") }
                HeaderCell { label: "HIGH RISK"; widthPx: 132; onPressed: sort("cpu") }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(0, count * (Style.s48 + Style.s8))
            interactive: false
            model: root.rows
            spacing: Style.s8

            delegate: GlassCard {
                required property var modelData
                width: ListView.view.width
                height: Style.s48
                fillColor: Style.bg1
                borderColor: Style.border1
                contentMargin: 0
                liftOnHover: false

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Style.s16
                    anchors.rightMargin: Style.s16
                    spacing: Style.s12

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: Style.s12

                        Rectangle {
                            Layout.preferredWidth: 32; Layout.preferredHeight: 32
                            radius: Style.r8
                            color: Style.bg2
                            border.color: Style.border1
                            Text {
                                anchors.centerIn: parent
                                text: modelData.nameInitial
                                color: Style.text0
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f14
                                font.weight: Style.w700
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData.name
                            color: Style.text0
                            elide: Text.ElideRight
                            font.family: Style.fontBody
                            font.pixelSize: Style.f13
                            font.weight: Style.w600
                        }
                    }

                    Text {
                        Layout.preferredWidth: 100
                        Layout.alignment: Qt.AlignVCenter
                        text: Number(modelData.cpuPercent).toFixed(1) + "%"
                        color: Style.usageColor(modelData.cpuPercent)
                        font.family: Style.fontMono
                        font.pixelSize: Style.f12
                        font.weight: Style.w600
                    }
                    Text {
                        Layout.preferredWidth: 100
                        Layout.alignment: Qt.AlignVCenter
                        text: Number(modelData.memoryMb).toFixed(0) + " MB"
                        color: Style.text2
                        font.family: Style.fontMono
                        font.pixelSize: Style.f12
                    }
                    Rectangle {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter
                        radius: Style.r999
                        color: root.riskGlow(modelData.risk)
                        border.color: root.riskColor(modelData.risk)
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: modelData.riskLabel
                            color: root.riskColor(modelData.risk)
                            font.family: Style.fontMono
                            font.pixelSize: Style.f11
                            font.weight: Style.w600
                        }
                    }

                    GlowButton {
                        Layout.preferredWidth: 110
                        Layout.alignment: Qt.AlignVCenter
                        label: modelData.isCritical ? "System" : "Suspend"
                        variant: "outlined"
                        glowColor: modelData.isCritical ? Style.border1 : Style.amber
                        enabled: !modelData.isCritical
                        onClicked: SystemCtrl.suspendProcess(modelData.pid)
                    }

                    GlowButton {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 132
                        label: modelData.isCritical ? "System" : "End Process"
                        variant: "outlined"
                        glowColor: modelData.isCritical ? Style.border1 : Style.red
                        enabled: !modelData.isCritical
                        onClicked: {
                            root.pendingProcess = modelData
                            endProcessDialog.open()
                        }
                    }
                }
            }
        }
        
        EmptyState {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            visible: root.rows.length === 0
            title: "No processes found"
            subtitle: root.query === "" ? "Scanning runtime..." : "No task matches filter."
        }
    }

    function sort(key) {
        if (root.sortKey === key) root.descending = !root.descending
        else { root.sortKey = key; root.descending = true }
        root.refreshRows()
    }

    Dialog {
        id: endProcessDialog
        modal: true
        title: "Confirm High-Risk Process Action"
        standardButtons: Dialog.Cancel
        width: 480
        ColumnLayout {
            anchors.fill: parent
            spacing: Style.s12
            Text {
                text: root.pendingProcess ? root.pendingProcess.name : "Process"
                color: Style.text0
                font.family: Style.fontDisplay
                font.pixelSize: Style.f20
                font.weight: Style.w700
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Text {
                text: "Ending a process can cause data loss or application instability. PulseBoost will keep the action gated and audited by the backend safety policy."
                color: Style.text2
                font.family: Style.fontBody
                font.pixelSize: Style.f13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GlowButton {
                    label: "Confirm End Process"
                    glowColor: Style.red
                    variant: "outlined"
                    onClicked: {
                        if (root.pendingProcess) SystemCtrl.killProcess(root.pendingProcess.pid)
                        endProcessDialog.close()
                    }
                }
            }
        }
    }

    component HeaderCell: Item {
        id: cell
        property string label: ""
        property real widthPx: 100
        property bool isFill: false
        signal pressed()
        Layout.preferredWidth: widthPx
        Layout.fillWidth: isFill
        Layout.fillHeight: true
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: cell.label
            color: Style.text2
            font.family: Style.fontMono
            font.pixelSize: Style.f11
            font.letterSpacing: 1
        }
        TapHandler { onTapped: cell.pressed() }
    }
}
