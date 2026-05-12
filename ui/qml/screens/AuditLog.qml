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
    property string query: ""
    property var selectedEntry: entries && entries.length > 0 ? entries[0] : null

    Component.onCompleted: SystemCtrl.refreshAuditLog()

    function filteredEntries() {
        const result = []
        const needle = query.toLowerCase()
        if (!entries) return result
        for (let i = 0; i < entries.length; i += 1) {
            const row = entries[i]
            const text = String(row.createdAt + " " + row.actionType + " " + row.riskLevel + " " + row.summary).toLowerCase()
            if (needle === "" || text.indexOf(needle) !== -1) result.push(row)
        }
        return result
    }

    function rollbackText(row) {
        const text = String((row.actionType || "") + " " + (row.resultJson || "")).toLowerCase()
        if (text.indexOf("restore") !== -1 || text.indexOf("revert") !== -1 || text.indexOf("rollback") !== -1 || text.indexOf("quarantine") !== -1) return "Tracked"
        return "Review"
    }

    function confirmationText(row) {
        return row.dryRun ? "Dry-run" : "Backend gate"
    }

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

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s12
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                radius: Style.r10
                color: Style.bg2
                border.color: Style.border1
                border.width: 1
                TextField {
                    anchors.fill: parent
                    anchors.leftMargin: Style.s12
                    anchors.rightMargin: Style.s12
                    background: null
                    color: Style.text0
                    placeholderText: "Search audit entries by action, risk, result, or time"
                    placeholderTextColor: Style.text3
                    font.family: Style.fontBody
                    font.pixelSize: Style.f13
                    onTextChanged: root.query = text
                }
            }
            StatusPill { text: "Audit visible"; tone: "success" }
            StatusPill { text: "Rollback tracked"; tone: "neutral" }
            StatusPill { text: "Confirmation logged"; tone: "warning" }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(430, root.filteredEntries().length * 54 + 104)
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s16
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Style.s10
                    SectionHeader { Layout.fillWidth: true; title: "System-Changing Actions"; subtitle: Number(root.filteredEntries().length).toFixed(0) + " visible entries loaded from action_audit_log" }
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
                        Text { text: "Result"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 80 }
                        Text { text: "Rollback"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 86 }
                        Text { text: "Confirmation"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 104 }
                        Text { text: "Details"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.fillWidth: true }
                    }
                }
                    Repeater {
                        model: root.filteredEntries()
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: Style.r8
                            color: root.selectedEntry === modelData ? Style.bg3 : Style.bg1
                            border.color: root.selectedEntry === modelData ? Style.cyan : Style.border1
                            border.width: 1
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Style.s10
                                anchors.rightMargin: Style.s10
                                spacing: Style.s10
                                Text { text: modelData.createdAt; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10; Layout.preferredWidth: 156; elide: Text.ElideRight }
                                Text { text: modelData.actionType; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600; Layout.preferredWidth: 170; elide: Text.ElideRight }
                                StatusPill { text: String(modelData.riskLevel).toUpperCase(); tone: modelData.riskLevel === "high" || modelData.riskLevel === "critical" ? "error" : (modelData.riskLevel === "moderate" ? "warning" : "success") }
                                Text { text: modelData.status || (modelData.dryRun ? "dry-run" : "live"); color: modelData.status === "success" ? Style.green : Style.amber; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 80; elide: Text.ElideRight }
                                Text { text: root.rollbackText(modelData); color: Style.cyan; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 86; elide: Text.ElideRight }
                                Text { text: root.confirmationText(modelData); color: modelData.dryRun ? Style.cyan : Style.amber; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 104; elide: Text.ElideRight }
                                Text { text: modelData.summary; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; Layout.fillWidth: true; elide: Text.ElideRight }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.selectedEntry = modelData }
                        }
                    }
                }
                GlassPanel {
                    Layout.preferredWidth: 300
                    Layout.fillHeight: true
                    fillColor: Style.bg1
                    borderColor: Style.border1
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.s14
                        spacing: Style.s10
                        SectionHeader { Layout.fillWidth: true; title: "Details"; subtitle: root.selectedEntry ? root.selectedEntry.actionType : "Select an audit row" }
                        StatusPill { text: root.selectedEntry ? String(root.selectedEntry.riskLevel).toUpperCase() : "REVIEW"; tone: root.selectedEntry && (root.selectedEntry.riskLevel === "high" || root.selectedEntry.riskLevel === "critical") ? "error" : "neutral" }
                        Text { text: "Human explanation"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; font.weight: Style.w600 }
                        Text { text: root.selectedEntry ? root.selectedEntry.summary : "No entry selected."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Text { text: "Technical summary"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; font.weight: Style.w600 }
                        Text { text: root.selectedEntry ? ("Time: " + root.selectedEntry.createdAt + "\nResult: " + root.selectedEntry.status + "\nRollback: " + root.rollbackText(root.selectedEntry) + "\nConfirmation: " + root.confirmationText(root.selectedEntry) + "\nSafety policy result: recorded in action_audit_log") : "--"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Text { text: root.selectedEntry ? ("Affected paths/settings payload:\n" + (root.selectedEntry.requestJson || root.selectedEntry.resultJson || "No backend payload supplied.")) : "Affected paths/settings are shown when supplied by the backend payload."; color: Style.text3; font.family: Style.fontBody; font.pixelSize: Style.f11; wrapMode: Text.WordWrap; Layout.fillWidth: true; elide: Text.ElideRight; maximumLineCount: 5 }
                    }
                }
            }
        }
    }
}
