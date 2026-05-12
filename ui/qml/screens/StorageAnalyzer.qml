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

    property var categories: SystemCtrl.storageCategories
    property var largeFiles: []
    property var selectedNode: null
    property real cleanDeltaMb: SystemCtrl.savedTodayMb

    function colorForCategory(name) {
        const lower = (name || "").toLowerCase()
        if (lower.indexOf("game") !== -1) return Style.violet
        if (lower.indexOf("video") !== -1 || lower.indexOf("media") !== -1) return Style.magenta
        if (lower.indexOf("user") !== -1 || lower.indexOf("document") !== -1) return Style.green
        if (lower.indexOf("download") !== -1) return Style.amber
        if (lower.indexOf("temp") !== -1) return Style.red
        if (lower.indexOf("windows") !== -1 || lower.indexOf("system") !== -1) return Style.text2
        return Style.cyan
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
            Layout.preferredHeight: 72
            contentMargin: Style.s16

            RowLayout {
                anchors.fill: parent
                spacing: Style.s16

                Text {
                    text: "Storage Topography"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }
                Item { Layout.fillWidth: true }
                GlowButton {
                    label: "Deep Scan Large Files"
                    glowColor: Style.violet
                    variant: "outlined"
                    onClicked: root.largeFiles = SystemCtrl.findLargeFiles()
                }
                GlowButton {
                    label: "Preview Cleanup"
                    glowColor: Style.cyan
                    variant: "outlined"
                    onClicked: root.largeFiles = SystemCtrl.findLargeFiles()
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1180 ? 2 : 1
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 460

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s12

                    Text {
                        text: "Disk Footprint Analysis"
                        color: Style.text1
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f16
                        font.weight: Style.w600
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Canvas {
                            id: mapCanvas
                            anchors.fill: parent
                            property var localNodes: []
                            property int hoveredIndex: -1

                            onWidthChanged: refresh()
                            onHeightChanged: refresh()
                            onLocalNodesChanged: requestPaint()

                            function refresh() {
                                localNodes = SystemCtrl.storageTreemap(width, height)
                                requestPaint()
                            }

                            Connections {
                                target: SystemCtrl
                                function onStorageChanged() { mapCanvas.refresh() }
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onPositionChanged: {
                                    let found = -1
                                    for (let i = 0; i < mapCanvas.localNodes.length; i += 1) {
                                        const n = mapCanvas.localNodes[i]
                                        if (mouse.x >= n.x && mouse.x <= (n.x + n.width) && mouse.y >= n.y && mouse.y <= (n.y + n.height)) {
                                            found = i
                                            break
                                        }
                                    }
                                    mapCanvas.hoveredIndex = found
                                }
                                onClicked: {
                                    if (mapCanvas.hoveredIndex >= 0) {
                                        root.selectedNode = mapCanvas.localNodes[mapCanvas.hoveredIndex]
                                    }
                                }
                            }

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                ctx.clearRect(0, 0, width, height)
                                if (!localNodes || localNodes.length === 0) return

                                for (let i = 0; i < localNodes.length; i += 1) {
                                    const n = localNodes[i]
                                    const c = colorForCategory(n.name)
                                    const alpha = i === hoveredIndex ? 0.95 : 0.65
                                    
                                    ctx.fillStyle = Qt.rgba(c.r, c.g, c.b, alpha)
                                    // Simulated border radius for Canvas (manual path drawing for simple rectangles)
                                    ctx.beginPath()
                                    ctx.rect(n.x, n.y, n.width, n.height)
                                    ctx.fill()

                                    ctx.strokeStyle = Style.bg0
                                    ctx.lineWidth = i === hoveredIndex ? 3 : 2
                                    ctx.strokeRect(n.x, n.y, n.width, n.height)

                                    if (n.width > 110 && n.height > 60) {
                                        ctx.fillStyle = Style.text0
                                        ctx.font = "bold " + Style.f14 + "px " + Style.fontDisplay
                                        ctx.fillText(n.name, n.x + Style.s12, n.y + Style.s24)
                                        ctx.fillStyle = Style.bg0
                                        ctx.font = Style.f12 + "px " + Style.fontMono
                                        ctx.fillText(Style.formatBytes(Number(n.bytes)), n.x + Style.s12, n.y + 42)
                                    }
                                }
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: Style.s10
                        Repeater {
                            model: root.categories
                            delegate: Rectangle {
                                required property var modelData
                                height: 32
                                radius: Style.r999
                                color: Style.bg2
                                border.color: Style.borderGlass
                                border.width: 1
                                implicitWidth: legendRow.implicitWidth + Style.s20

                                Row {
                                    id: legendRow
                                    anchors.centerIn: parent
                                    spacing: Style.s8
                                    Rectangle {
                                        width: 10
                                        height: 10
                                        radius: 5
                                        color: root.colorForCategory(modelData.name)
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: modelData.name + " " + Style.formatBytes(Number(modelData.bytes))
                                        color: Style.text0
                                        font.family: Style.fontMono
                                        font.pixelSize: Style.f11
                                    }
                                }
                            }
                        }
                    }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 460

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s12

                    Text {
                        text: root.selectedNode ? ("Category Focus: " + root.selectedNode.name) : "File Analysis"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f18
                        font.weight: Style.w700
                    }

                    GlassCard {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Style.s64
                        fillColor: Style.bg1

                        Column {
                            anchors.fill: parent
                            anchors.margins: Style.s12
                            spacing: 4

                            Text {
                                text: root.selectedNode ? root.selectedNode.name : "Select a treemap segment"
                                color: Style.text0
                                font.family: Style.fontBody
                                font.pixelSize: Style.f14
                                font.weight: Style.w600
                            }
                            RowLayout {
                                width: parent.width
                                spacing: Style.s10
                                Text {
                        text: root.selectedNode ? ("Occupies " + Number(root.selectedNode.fraction * 100).toFixed(1) + "% of analyzed volume.") : "Cleanup is preview-first. Use Action Center for dry-run, quarantine, confirmation, and rollback visibility."
                                    color: Style.text2
                                    font.family: Style.fontBody
                                    font.pixelSize: Style.f12
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: root.selectedNode ? Style.formatBytes(Number(root.selectedNode.bytes)) : ("Last pulse sweep recovered " + Number(root.cleanDeltaMb).toFixed(0) + " MB")
                                    color: root.selectedNode ? root.colorForCategory(root.selectedNode.name) : Style.cyan
                                    font.family: Style.fontMono
                                    font.pixelSize: Style.f12
                                    font.weight: Style.w700
                                }
                            }
                        }
                    }

                    Text {
                        text: "Largest Files Discovered"
                        color: Style.text1
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f14
                        font.weight: Style.w600
                        Layout.topMargin: Style.s10
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Style.s8
                        model: root.largeFiles

                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width
                            height: 44
                            radius: Style.r8
                            color: Style.bg2
                            border.color: Style.borderGlass
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Style.s12
                                anchors.rightMargin: Style.s12
                                spacing: Style.s12

                                Text {
                                    text: Icons.glyph("storage")
                                    color: Style.text3
                                    font.family: Style.fontDisplay
                                    font.pixelSize: Style.f14
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.path
                                    color: Style.text1
                                    font.family: Style.fontBody
                                    font.pixelSize: Style.f12
                                    elide: Text.ElideMiddle
                                }
                                Text {
                                    text: Style.formatBytes(Number(modelData.bytes))
                                    color: Style.amber
                                    font.family: Style.fontMono
                                    font.pixelSize: Style.f11
                                    font.weight: Style.w600
                                }
                            }
                        }
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: root.largeFiles.length === 0
                        title: "No heavy files identified"
                        subtitle: "Trigger a deep scan to unearth large unused artifacts."
                    }
                }
            }
        }
    }
}
