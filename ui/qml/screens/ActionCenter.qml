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

    property bool confirmed: false
    property var actions: SystemCtrl.actionCenterActions
    property var proof: SystemCtrl.latestProofReport
    property string currentFilter: "safe"
    property var selectedAction: actions && actions.length > 0 ? actions[0] : null

    function riskColor(risk) {
        if (risk === "critical" || risk === "high") return Style.red
        if (risk === "moderate") return Style.amber
        if (risk === "low") return Style.green
        return Style.cyan
    }

    function filterLabel(filterId) {
        if (filterId === "safe") return "Safe"
        if (filterId === "manual") return "Manual"
        if (filterId === "advanced") return "Advanced"
        return "High Risk"
    }

    function matchesFilter(action) {
        const risk = String(action.riskLevel || "").toLowerCase()
        const confirmation = String(action.requiredConfirmation || "").toLowerCase()
        if (currentFilter === "safe") return risk === "safe" || risk === "low"
        if (currentFilter === "manual") return confirmation.indexOf("manual") !== -1 || risk.indexOf("moderate") !== -1 || risk.indexOf("medium") !== -1
        if (currentFilter === "advanced") return confirmation.indexOf("advanced") !== -1 || risk.indexOf("manual") !== -1
        return risk.indexOf("high") !== -1 || risk.indexOf("critical") !== -1
    }

    function filteredActions() {
        const result = []
        if (!actions) return result
        for (let i = 0; i < actions.length; i += 1) {
            if (matchesFilter(actions[i])) result.push(actions[i])
        }
        return result
    }

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 154
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8
                    Text { text: "Optimization Action Center"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "Every system-changing action is dry-run first, risk labeled, confirmed when required, and written to the audit log."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        spacing: Style.s10
                        StatusPill { text: "Proof capture enabled"; tone: "success" }
                        StatusPill { text: "SQLite audit log"; tone: "neutral" }
                        StatusPill { text: "Advanced actions gated"; tone: "warning" }
                    }
                }
                Rectangle {
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 56
                    radius: Style.r12
                    color: Style.bg1
                    border.color: root.confirmed ? Style.amber : Style.border1
                    border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Style.s12
                        CheckBox {
                            id: confirmBox
                            checked: root.confirmed
                            onCheckedChanged: root.confirmed = checked
                        }
                        Text { text: "Manual confirmation"; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f13; font.weight: Style.w600 }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s10
            Repeater {
                model: ["safe", "manual", "advanced", "high"]
                delegate: Rectangle {
                    required property string modelData
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 38
                    radius: Style.r10
                    color: root.currentFilter === modelData ? Style.cyan : Style.bg2
                    border.color: root.currentFilter === modelData ? Style.cyan : Style.border1
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: root.filterLabel(modelData)
                        color: root.currentFilter === modelData ? Style.bg0 : Style.text1
                        font.family: Style.fontBody
                        font.pixelSize: Style.f13
                        font.weight: Style.w600
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentFilter = modelData
                    }
                }
            }
            Item { Layout.fillWidth: true }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 154
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s8
                SectionHeader {
                    Layout.fillWidth: true
                    title: "Selected Action Safety Review"
                    subtitle: root.selectedAction ? root.selectedAction.name : "Select an action to review impact, dry-run evidence, confirmation, and rollback."
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.s10
                    StatusPill { text: root.selectedAction ? String(root.selectedAction.riskLevel).toUpperCase() : "MANUAL"; tone: root.selectedAction ? root.selectedAction.riskTone : "warning" }
                    StatusPill { text: root.selectedAction && root.selectedAction.dryRunSupported ? "Dry-run required" : "Review required"; tone: root.selectedAction && root.selectedAction.dryRunSupported ? "success" : "warning" }
                    StatusPill { text: root.selectedAction && root.selectedAction.rollbackAvailable ? "Rollback available" : "Restore point recommended"; tone: root.selectedAction && root.selectedAction.rollbackAvailable ? "success" : "warning" }
                }
                Text {
                    text: root.selectedAction ? ("What will change: " + root.selectedAction.expectedEffect + " | Policy gate: " + root.selectedAction.requiredConfirmation + " | Backup/restore: " + root.selectedAction.backupRestoreAvailability) : "No action selected."
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            fillColor: Style.bg2
            borderColor: Style.border1
            visible: root.proof && root.proof.actionId
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s10
                SectionHeader { Layout.fillWidth: true; title: "Before / After Proof"; subtitle: root.proof.summary || "Latest optimization comparison" }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 6
                    columnSpacing: Style.s10
                    rowSpacing: Style.s10
                    Repeater {
                        model: [
                            { "label": "CPU", "value": Number(root.proof.cpuDelta || 0).toFixed(1) + "%" },
                            { "label": "RAM", "value": Number(root.proof.ramDelta || 0).toFixed(1) + "%" },
                            { "label": "Disk", "value": Number(root.proof.diskDelta || 0).toFixed(1) + "%" },
                            { "label": "Startup", "value": Number(root.proof.startupDelta || 0).toFixed(0) },
                            { "label": "Storage", "value": Number(root.proof.recoverableStorageDeltaMb || 0).toFixed(1) + " MB" },
                            { "label": "Boot est.", "value": Number(root.proof.bootEstimateDeltaSeconds || 0).toFixed(1) + "s" }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 72
                            radius: Style.r10
                            color: Style.bg1
                            border.color: Style.border1
                            border.width: 1
                            Column {
                                anchors.centerIn: parent
                                spacing: Style.s4
                                Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                                Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.value; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f22; font.weight: Style.w700 }
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Style.s12
            Repeater {
                model: root.filteredActions()
                delegate: GlassPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 182
                    fillColor: Style.glassPanel
                    borderColor: Style.border1
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Style.s14
                        spacing: Style.s14
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Style.s7
                            RowLayout {
                                spacing: Style.s8
                                StatusPill { text: String(modelData.riskLevel).toUpperCase(); tone: modelData.riskTone }
                                StatusPill { text: modelData.dryRunSupported ? "DRY-RUN" : "LIVE ONLY"; tone: modelData.dryRunSupported ? "success" : "warning" }
                                StatusPill { text: modelData.requiredConfirmation; tone: modelData.confirmationRequired ? "warning" : "neutral" }
                                StatusPill { text: modelData.backupRestoreAvailability; tone: modelData.rollbackAvailable ? "success" : "warning" }
                            }
                            Text { text: modelData.name; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f22; font.weight: Style.w700 }
                            Text { text: modelData.expectedEffect; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Text { text: "Dry-run: " + modelData.dryRunResult; color: Style.cyan; font.family: Style.fontBody; font.pixelSize: Style.f12; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: "Actual: " + modelData.actualResult; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: "Audit: " + modelData.auditLogLink; color: Style.text3; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                        }
                        ColumnLayout {
                            Layout.preferredWidth: 150
                            spacing: Style.s8
                            GlowButton { Layout.fillWidth: true; label: "Dry Run"; glowColor: Style.cyan; variant: "outlined"; onClicked: SystemCtrl.dryRunOptimizationAction(modelData.actionId) }
                            GlowButton { Layout.fillWidth: true; label: "Review"; variant: "outlined"; glowColor: Style.amber; onClicked: root.selectedAction = modelData }
                            GlowButton { Layout.fillWidth: true; label: "Apply"; enabled: modelData.canExecuteFromCenter; glowColor: root.riskColor(modelData.riskLevel); onClicked: SystemCtrl.executeOptimizationAction(modelData.actionId, root.confirmed) }
                        }
                    }
                }
            }
        }
    }
}
