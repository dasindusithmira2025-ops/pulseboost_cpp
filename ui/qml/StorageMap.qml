import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: storagePage

    readonly property color surface: "#101b2c"
    readonly property color surfaceRaised: "#17253b"
    readonly property color border: "#24334d"
    readonly property color text: "#eef5ff"
    readonly property color textMuted: "#91a4bf"
    readonly property color danger: "#f85149"
    readonly property color warning: "#d29922"
    readonly property color success: "#3fb950"

    property var storageNodes: []

    function refreshNodes() {
        if (treemapStage.width <= 0 || treemapStage.height <= 0)
            return
        storageNodes = SystemCtrl.storageTreemap(Math.floor(treemapStage.width), Math.floor(treemapStage.height))
    }

    function usageColor() {
        if (SystemCtrl.diskUsage >= 90)
            return danger
        if (SystemCtrl.diskUsage >= 70)
            return warning
        return success
    }

    Component.onCompleted: Qt.callLater(refreshNodes)

    Connections {
        target: SystemCtrl

        function onStorageChanged() {
            refreshNodes()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 18

        Rectangle {
            Layout.fillWidth: true
            radius: 24
            color: storagePage.surface
            border.color: storagePage.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "Storage treemap"
                            color: storagePage.text
                            font.pixelSize: 24
                            font.bold: true
                        }

                        Text {
                            text: "Categories are sized from the current disk scan so you can identify space pressure immediately."
                            color: storagePage.textMuted
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        radius: 14
                        color: storagePage.surfaceRaised
                        border.color: usageColor()
                        border.width: 1
                        implicitWidth: 190
                        implicitHeight: 64

                        Column {
                            anchors.centerIn: parent
                            spacing: 2

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: Number(SystemCtrl.diskUsage).toFixed(1) + "%"
                                color: usageColor()
                                font.pixelSize: 24
                                font.bold: true
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "system volume used"
                                color: storagePage.textMuted
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 12
                    radius: 6
                    color: "#0c1524"

                    Rectangle {
                        width: parent.width * Math.min(1, Number(SystemCtrl.diskUsage) / 100.0)
                        height: parent.height
                        radius: parent.radius
                        color: usageColor()

                        Behavior on width {
                            NumberAnimation { duration: 650; easing.type: Easing.OutCubic }
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    Repeater {
                        model: SystemCtrl.storageCategories ? SystemCtrl.storageCategories.length : 0

                        delegate: Rectangle {
                            property var entry: (SystemCtrl.storageCategories && index < SystemCtrl.storageCategories.length)
                                                ? SystemCtrl.storageCategories[index]
                                                : ({})
                            radius: 12
                            color: storagePage.surfaceRaised
                            border.color: entry.accent || storagePage.border
                            border.width: 1
                            implicitWidth: legendLabel.implicitWidth + 28
                            implicitHeight: 34

                            Row {
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: entry.accent || storagePage.textMuted
                                }

                                Text {
                                    id: legendLabel
                                    text: (entry.name || "Unknown") + "  " + formatBytes(entry.bytes || 0)
                                    color: storagePage.text
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 24
            color: storagePage.surface
            border.color: storagePage.border
            border.width: 1
            clip: true

            Item {
                id: treemapStage
                anchors.fill: parent
                anchors.margins: 14

                onWidthChanged: Qt.callLater(storagePage.refreshNodes)
                onHeightChanged: Qt.callLater(storagePage.refreshNodes)

                Repeater {
                    model: storagePage.storageNodes.length

                    delegate: Rectangle {
                        property var node: storagePage.storageNodes[index] || ({})
                        x: Number(node.x || 0)
                        y: Number(node.y || 0)
                        width: Math.max(0, Number(node.width || 0))
                        height: Math.max(0, Number(node.height || 0))
                        radius: 18
                        color: tinted(node.accent || "#56b2ff", 0.17)
                        border.color: node.accent || "#56b2ff"
                        border.width: 1
                        clip: true

                        Behavior on x {
                            NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                        }

                        Behavior on y {
                            NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                        }

                        Behavior on width {
                            NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                        }

                        Behavior on height {
                            NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                        }

                        Column {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6

                            Text {
                                width: parent.width
                                text: node.name || "Unknown"
                                color: storagePage.text
                                font.pixelSize: Math.max(12, Math.min(20, parent.width / 11))
                                font.bold: true
                                elide: Text.ElideRight
                                visible: parent.width > 90 && parent.height > 54
                            }

                            Text {
                                width: parent.width
                                text: formatBytes(node.bytes || 0)
                                color: storagePage.textMuted
                                font.pixelSize: Math.max(11, Math.min(14, parent.width / 16))
                                elide: Text.ElideRight
                                visible: parent.width > 120 && parent.height > 80
                            }

                            Rectangle {
                                width: percentLabel.implicitWidth + 14
                                height: 24
                                radius: 12
                                color: node.accent || "#56b2ff"
                                visible: parent.width > 84 && parent.height > 60

                                Text {
                                    id: percentLabel
                                    anchors.centerIn: parent
                                    text: (Number(node.fraction || 0) * 100).toFixed(1) + "%"
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }
                        }

                        HoverHandler { id: tileHover }

                        ToolTip.visible: tileHover.hovered
                        ToolTip.delay: 220
                        ToolTip.text: (node.name || "Unknown") + "  " + formatBytes(node.bytes || 0)
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "Waiting for storage categories."
                    color: storagePage.textMuted
                    font.pixelSize: 13
                    visible: storagePage.storageNodes.length === 0
                }
            }
        }
    }

    function tinted(hexColor, alpha) {
        const parsed = Qt.color(hexColor)
        return Qt.rgba(parsed.r, parsed.g, parsed.b, alpha)
    }

    function formatBytes(bytes) {
        if (bytes >= 1073741824)
            return (bytes / 1073741824).toFixed(1) + " GB"
        if (bytes >= 1048576)
            return (bytes / 1048576).toFixed(0) + " MB"
        if (bytes >= 1024)
            return (bytes / 1024).toFixed(0) + " KB"
        return bytes + " B"
    }
}
