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

    property var entries: SystemCtrl.auditLogEntries

    Component.onCompleted: SystemCtrl.refreshAuditLog()

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 138
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8
                    Text { text: "Audit Log"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "SQLite-backed record of dry-runs, blocked attempts, and completed system-changing actions."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                GlowButton { label: "Refresh"; glowColor: Style.cyan; variant: "outlined"; onClicked: SystemCtrl.refreshAuditLog() }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(360, root.entries.length * 54 + 88)
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s10
                SectionHeader { Layout.fillWidth: true; title: "System-Changing Actions"; subtitle: Number(root.entries.length).toFixed(0) + " entries loaded from action_audit_log" }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    radius: Style.r8
                    color: Style.bg1
                    border.color: Style.border1
                    border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Style.s10
                        anchors.rightMargin: Style.s10
                        Text { text: "Time"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 156 }
                        Text { text: "Action"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 170 }
                        Text { text: "Risk"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 82 }
                        Text { text: "Mode"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 70 }
                        Text { text: "Summary"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.fillWidth: true }
                    }
                }
                Repeater {
                    model: root.entries
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        radius: Style.r8
                        color: Style.bg1
                        border.color: Style.border1
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s10
                            anchors.rightMargin: Style.s10
                            spacing: Style.s10
                            Text { text: modelData.createdAt; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 156; elide: Text.ElideRight }
                            Text { text: modelData.actionType; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600; Layout.preferredWidth: 170; elide: Text.ElideRight }
                            StatusPill { text: String(modelData.riskLevel).toUpperCase(); tone: modelData.riskLevel === "high" || modelData.riskLevel === "critical" ? "error" : (modelData.riskLevel === "moderate" ? "warning" : "success") }
                            Text { text: modelData.dryRun ? "Dry-run" : "Live"; color: modelData.dryRun ? Style.cyan : Style.amber; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 70 }
                            Text { text: modelData.summary; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                    }
                }
            }
        }
    }
}
