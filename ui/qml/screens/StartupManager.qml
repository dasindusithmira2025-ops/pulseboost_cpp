import QtGraphicalEffects 1.15
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

    property var items: []
    property var displayRows: []
    property var pendingStartupItem: null
    property string pendingStartupAction: ""

    function categoryFor(item) {
        const lower = String(item.name).toLowerCase()
        if (lower.indexOf("steam") !== -1 || lower.indexOf("epic") !== -1 || lower.indexOf("riot") !== -1) return "Gaming"
        if (lower.indexOf("office") !== -1 || lower.indexOf("teams") !== -1 || lower.indexOf("slack") !== -1 || lower.indexOf("adobe") !== -1) return "Productivity"
        if (String(item.location).toLowerCase().indexOf("windows") !== -1) return "System Infrastructure"
        return "Uncategorized Apps"
    }

    function bootEstimate(itemsList) {
        let total = 12
        for (let i = 0; i < itemsList.length; i += 1) {
            total += Number(itemsList[i].impactScore) * 0.35
        }
        return total
    }

    function optimizedEstimate(itemsList) {
        let total = 10
        for (let i = 0; i < itemsList.length; i += 1) {
            const category = categoryFor(itemsList[i])
            const multiplier = category.indexOf("System") !== -1 ? 0.25 : 0.12
            total += Number(itemsList[i].impactScore) * multiplier
        }
        return total
    }

    function rebuildGroups() {
        const groups = ["Gaming", "Productivity", "System Infrastructure", "Uncategorized Apps"]
        const result = []
        for (let g = 0; g < groups.length; g += 1) {
            const category = groups[g]
            const groupItems = items.filter(function(item) { return categoryFor(item) === category })
            if (groupItems.length === 0) continue
            result.push({ rowType: "header", title: category })
            for (let i = 0; i < groupItems.length; i += 1) {
                result.push({ rowType: "item", payload: groupItems[i] })
            }
        }
        displayRows = result
    }

    function refresh() {
        root.items = SystemCtrl.fetchStartupItems()
        rebuildGroups()
    }

    Component.onCompleted: refresh()

    ColumnLayout {
        id: contentColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            contentMargin: Style.s16

            ColumnLayout {
                anchors.fill: parent
                spacing: Style.s12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.s16

                    Text {
                        text: "Boot Optimization Intelligence"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }
                    Item { Layout.fillWidth: true }
                    GlowButton {
                        label: "Preview Non-Essential"
                        variant: "outlined"
                        glowColor: Style.amber
                        onClicked: root.refresh()
                    }
                    GlowButton {
                        label: "Calculate Impact"
                        variant: "outlined"
                        glowColor: Style.violet
                        onClicked: root.refresh()
                    }
                    GlowButton {
                        label: "Rescan Environment"
                        glowColor: Style.cyan
                        variant: "outlined"
                        onClicked: root.refresh()
                    }
                }
                
                Rectangle { Layout.fillWidth: true; height: 1; color: Style.borderGlass }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.s20
                    
                    Text {
                        text: "Current Boot Forecast:"
                        color: Style.text2
                        font.family: Style.fontMono
                        font.pixelSize: Style.f11
                    }
                    Text {
                        text: Number(root.bootEstimate(root.items)).toFixed(1) + " s"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }
                    
                    Rectangle { width: 1; height: 16; color: Style.border1; anchors.verticalCenter: parent.verticalCenter }
                    
                    Text {
                        text: "Preview Profile Estimate:"
                        color: Style.text2
                        font.family: Style.fontMono
                        font.pixelSize: Style.f11
                    }
                    Text {
                        text: Number(root.optimizedEstimate(root.items)).toFixed(1) + " s"
                        color: Style.success
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                        layer.enabled: true
                        layer.effect: DropShadow { color: Style.greenGlow; radius: 8; samples: 16 }
                    }

                    Item { Layout.fillWidth: true }
                        Text {
                            text: root.items.length + " startup entries | rollback visible after changes"
                        color: Style.text2
                        font.family: Style.fontMono
                        font.pixelSize: Style.f12
                    }
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(0, count * (Style.s64 + Style.s8) + 120) // Approximation
            interactive: false
            spacing: Style.s8
            model: root.displayRows

            delegate: Loader {
                required property var modelData
                width: ListView.view ? ListView.view.width : root.width
                sourceComponent: modelData.rowType === "header" ? headerDelegate : itemDelegate
            }
        }

        EmptyState {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            visible: root.items.length === 0
            title: "Zero startup injections found"
            subtitle: "Registry and startup paths are scanned automatically."
        }
    }

    Component {
        id: headerDelegate
        Rectangle {
            width: ListView.view ? ListView.view.width : root.width
            height: 38
            radius: Style.r8
            color: "transparent"
            Text {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Style.s4
                anchors.left: parent.left
                anchors.leftMargin: Style.s4
                text: modelData.title.toUpperCase()
                color: Style.text2
                font.family: Style.fontMono
                font.pixelSize: Style.f11
                font.letterSpacing: 1
                font.weight: Style.w600
            }
        }
    }

    Component {
        id: itemDelegate
        GlassCard {
            property var item: modelData.payload
            width: ListView.view ? ListView.view.width : root.width
            height: 80
            fillColor: Style.bg2
            borderColor: Style.borderGlass
            contentMargin: Style.s16

            RowLayout {
                anchors.fill: parent
                spacing: Style.s16

                Rectangle {
                    Layout.preferredWidth: 32; Layout.preferredHeight: 32
                    radius: Style.r8
                    color: Style.bg3
                    border.color: Style.border1
                    Text {
                        anchors.centerIn: parent
                        text: Icons.glyph("settings")
                        color: Style.text1
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f16
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s4
                    
                    RowLayout {
                        spacing: Style.s8
                        Text {
                            text: item.name
                            color: Style.text0
                            font.family: Style.fontBody
                            font.pixelSize: Style.f14
                            font.weight: Style.w600
                            elide: Text.ElideRight
                        }
                        Rectangle {
                            width: impactText.implicitWidth + 12
                            height: 18
                            radius: 4
                            color: Number(item.impactScore) > 65 ? Style.redGlow : (Number(item.impactScore) > 35 ? Style.amberGlow : Style.greenGlow)
                            Text {
                                id: impactText
                                anchors.centerIn: parent
                                text: Number(item.impactScore).toFixed(0) + " Impact"
                                color: Number(item.impactScore) > 65 ? Style.red : (Number(item.impactScore) > 35 ? Style.amber : Style.green)
                                font.family: Style.fontMono
                                font.pixelSize: Style.f10
                                font.weight: Style.w700
                            }
                        }
                    }
                    
                    Text {
                        text: item.location
                        color: Style.text2
                        font.family: Style.fontMono
                        font.pixelSize: Style.f11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                
                RowLayout {
                    spacing: Style.s10
                    GlowButton {
                        Layout.preferredWidth: 100
                        label: "Delay 10s"
                        variant: "outlined"
                        glowColor: Style.amber
                        onClicked: {
                            root.pendingStartupItem = item
                            root.pendingStartupAction = "delay"
                            startupConfirmDialog.open()
                        }
                    }

                    GlowButton {
                        Layout.preferredWidth: 100
                        label: "Disable"
                        variant: "outlined"
                        glowColor: Style.red
                        onClicked: {
                            root.pendingStartupItem = item
                            root.pendingStartupAction = "disable"
                            startupConfirmDialog.open()
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: startupConfirmDialog
        modal: true
        title: "Confirm Startup Change"
        standardButtons: Dialog.Cancel
        width: 500
        ColumnLayout {
            anchors.fill: parent
            spacing: Style.s12
            Text {
                text: root.pendingStartupItem ? root.pendingStartupItem.name : "Startup item"
                color: Style.text0
                font.family: Style.fontDisplay
                font.pixelSize: Style.f20
                font.weight: Style.w700
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Text {
                text: "Startup changes are reversible and audited. Review publisher, path, impact, and rollback availability before confirming."
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
                    label: root.pendingStartupAction === "delay" ? "Confirm Delay" : "Confirm Disable"
                    glowColor: root.pendingStartupAction === "delay" ? Style.amber : Style.red
                    variant: "outlined"
                    onClicked: {
                        if (root.pendingStartupItem) {
                            if (root.pendingStartupAction === "delay") {
                                SystemCtrl.delayStartupItem(root.pendingStartupItem.name, root.pendingStartupItem.location, root.pendingStartupItem.command, 10)
                            } else {
                                SystemCtrl.disableStartupItem(root.pendingStartupItem.name, root.pendingStartupItem.location, root.pendingStartupItem.command)
                            }
                            root.refresh()
                        }
                        startupConfirmDialog.close()
                    }
                }
            }
        }
    }
}
