import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: processPage

    readonly property color surface: "#101b2c"
    readonly property color surfaceRaised: "#17253b"
    readonly property color border: "#24334d"
    readonly property color text: "#eef5ff"
    readonly property color textMuted: "#91a4bf"
    readonly property color danger: "#f85149"
    readonly property color warning: "#d29922"
    readonly property color success: "#3fb950"
    readonly property color accent: "#56b2ff"

    property string sortKey: "cpu"
    property bool descending: true
    property var rows: []

    function refreshModel() {
        rows = SystemCtrl.sortedProcesses(sortKey, descending)
    }

    function toggleSort(nextKey) {
        if (sortKey === nextKey) {
            descending = !descending
        } else {
            sortKey = nextKey
            descending = nextKey !== "name"
        }
        refreshModel()
    }

    Component.onCompleted: refreshModel()

    Connections {
        target: SystemCtrl

        function onProcessListChanged() {
            refreshModel()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 18

        Rectangle {
            Layout.fillWidth: true
            radius: 24
            color: processPage.surface
            border.color: processPage.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 18

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "Foreground and background process pressure"
                        color: processPage.text
                        font.pixelSize: 24
                        font.bold: true
                    }

                    Text {
                        text: "Use sorting to surface CPU spikes or memory-heavy processes. Critical system processes cannot be terminated from the UI."
                        color: processPage.textMuted
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    radius: 14
                    color: processPage.surfaceRaised
                    border.color: processPage.border
                    border.width: 1
                    implicitWidth: 190
                    implicitHeight: 64

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: processPage.rows.length + " tracked"
                            color: processPage.text
                            font.pixelSize: 22
                            font.bold: true
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "heavy processes"
                            color: processPage.textMuted
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 24
            color: processPage.surface
            border.color: processPage.border
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 52
                    color: processPage.surfaceRaised

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        spacing: 0

                        SortHeader {
                            Layout.fillWidth: true
                            label: "Process"
                            keyName: "name"
                            active: processPage.sortKey === keyName
                            descending: processPage.descending
                            onTriggered: processPage.toggleSort(keyName)
                        }

                        SortHeader {
                            width: 120
                            label: "CPU"
                            keyName: "cpu"
                            active: processPage.sortKey === keyName
                            descending: processPage.descending
                            onTriggered: processPage.toggleSort(keyName)
                        }

                        SortHeader {
                            width: 140
                            label: "Memory"
                            keyName: "ram"
                            active: processPage.sortKey === keyName
                            descending: processPage.descending
                            onTriggered: processPage.toggleSort(keyName)
                        }

                        Item { width: 118 }
                    }
                }

                ListView {
                    id: processList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8
                    model: processPage.rows.length
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        property var row: processPage.rows[index] || ({})
                        width: processList.width - 28
                        height: 70
                        x: 14
                        radius: 18
                        color: row.isCritical ? "#101a2a" : "#132036"
                        border.color: processPage.border
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 16

                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: row.isCritical ? processPage.danger
                                                      : (Number(row.cpuPercent || 0) >= 18
                                                             ? processPage.warning
                                                             : processPage.success)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    text: row.name || "Unknown process"
                                    color: processPage.text
                                    font.pixelSize: 14
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: row.isCritical ? "Protected Windows process" : "PID " + (row.pid || 0)
                                    color: processPage.textMuted
                                    font.pixelSize: 12
                                }
                            }

                            Text {
                                width: 120
                                text: Number(row.cpuPercent || 0).toFixed(1) + "%"
                                horizontalAlignment: Text.AlignRight
                                color: Number(row.cpuPercent || 0) >= 20
                                       ? processPage.danger
                                       : (Number(row.cpuPercent || 0) >= 10
                                              ? processPage.warning
                                              : processPage.textMuted)
                                font.pixelSize: 13
                                font.bold: Number(row.cpuPercent || 0) >= 10
                            }

                            Text {
                                width: 140
                                text: Number(row.memoryMb || 0).toFixed(0) + " MB"
                                horizontalAlignment: Text.AlignRight
                                color: Number(row.memoryMb || 0) >= 1200
                                       ? processPage.danger
                                       : (Number(row.memoryMb || 0) >= 700
                                              ? processPage.warning
                                              : processPage.textMuted)
                                font.pixelSize: 13
                                font.bold: Number(row.memoryMb || 0) >= 700
                            }

                            Rectangle {
                                width: 102
                                height: 38
                                radius: 12
                                color: killHover.hovered && !row.isCritical ? "#411417" : "#121d30"
                                border.color: row.isCritical ? "#31435f" : processPage.danger
                                border.width: 1
                                opacity: row.isCritical ? 0.45 : 1

                                Behavior on color {
                                    ColorAnimation { duration: 120 }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: row.isCritical ? "Protected" : "Terminate"
                                    color: row.isCritical ? processPage.textMuted : processPage.danger
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                HoverHandler { id: killHover }
                                TapHandler {
                                    enabled: !row.isCritical && Number(row.pid || 0) > 0
                                    onTapped: SystemCtrl.killProcess(Number(row.pid || 0))
                                }
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {}
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 72
                    visible: processPage.rows.length === 0
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "Waiting for the next process snapshot."
                        color: processPage.textMuted
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    component SortHeader: Item {
        required property string label
        required property string keyName
        required property bool active
        required property bool descending
        signal triggered

        implicitHeight: 52

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Text {
                text: label
                color: active ? processPage.accent : processPage.textMuted
                font.pixelSize: 12
                font.bold: active
            }

            Text {
                visible: active
                text: descending ? "v" : "^"
                color: processPage.accent
                font.pixelSize: 11
                font.bold: true
            }
        }

        TapHandler { onTapped: triggered() }
    }
}
