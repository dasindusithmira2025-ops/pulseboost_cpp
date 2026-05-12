import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.pagePad * 2

    property var restoreItems: SystemCtrl.restoreCenterItems
    property var pendingRestoreItem: null

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 146
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8
                    Text { text: "Restore / Undo Center"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "One place for restore points, PulseBoost snapshots, quarantined files, reverted tweaks, and rollback-capable actions."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        spacing: Style.s10
                        GlowButton { label: "Snapshot"; glowColor: Style.cyan; onClicked: SystemCtrl.takeSystemSnapshot() }
                        GlowButton { label: "Restore Point"; glowColor: Style.green; variant: "outlined"; onClicked: SystemCtrl.createRestorePoint() }
                    }
                }
                Column {
                    spacing: Style.s4
                    Text { text: "Recovery items"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                    Text { text: Number(root.restoreItems.length).toFixed(0); color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f40; font.weight: Style.w700 }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(360, root.restoreItems.length * 58 + 78)
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s10
                SectionHeader { Layout.fillWidth: true; title: "Available Undo Paths"; subtitle: "Shown from snapshots, quarantine storage, and audit rollback entries." }
                Repeater {
                    model: root.restoreItems
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        radius: Style.r10
                        color: Style.bg1
                        border.color: Style.border1
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s12
                            anchors.rightMargin: Style.s12
                            spacing: Style.s12
                            StatusPill { text: modelData.type; tone: modelData.tone || "neutral" }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text { text: modelData.name; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f13; font.weight: Style.w600; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: modelData.detail; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f11; Layout.fillWidth: true; elide: Text.ElideRight }
                            }
                            Text { text: modelData.status; color: Style.text1; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 140; elide: Text.ElideRight }
                            GlowButton {
                                label: "Restore"
                                variant: "outlined"
                                glowColor: Style.amber
                                enabled: modelData.type === "System Snapshot"
                                onClicked: {
                                    root.pendingRestoreItem = modelData
                                    restoreConfirmDialog.open()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: restoreConfirmDialog
        modal: true
        title: "Confirm Snapshot Restore"
        standardButtons: Dialog.Cancel
        width: 520
        ColumnLayout {
            anchors.fill: parent
            spacing: Style.s12
            Text {
                text: root.pendingRestoreItem ? root.pendingRestoreItem.name : "System Snapshot"
                color: Style.text0
                font.family: Style.fontDisplay
                font.pixelSize: Style.f20
                font.weight: Style.w700
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Text {
                text: "Snapshot restore can disable startup entries that were not present in the selected baseline. Review this rollback path before confirming."
                color: Style.text2
                font.family: Style.fontBody
                font.pixelSize: Style.f13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Text {
                text: "Restore point recommended | Rollback state visible | Audit logging enabled"
                color: Style.amber
                font.family: Style.fontMono
                font.pixelSize: Style.f11
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GlowButton {
                    label: "Confirm Restore"
                    glowColor: Style.amber
                    variant: "outlined"
                    onClicked: {
                        if (root.pendingRestoreItem) {
                            SystemCtrl.restoreSystemSnapshot(root.pendingRestoreItem.id)
                        }
                        restoreConfirmDialog.close()
                    }
                }
            }
        }
    }
}
