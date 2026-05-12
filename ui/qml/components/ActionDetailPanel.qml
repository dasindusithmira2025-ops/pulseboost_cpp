import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

AppCard {
    id: root
    property var actionData: ({})

    SectionHeader {
        title: "Safety Review"
        subtitle: actionData.name || "Select an action to inspect what will change."
    }
    RiskBadge { level: actionData.riskLevel || "manual" }
    Text {
        text: "What will change: " + (actionData.expectedEffect || "The action detail will appear after selection.")
        color: Theme.textSecondary
        font.family: Typography.body
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Text {
        text: "Policy gate: " + (actionData.requiredConfirmation || "Manual review required when risk is medium or higher.")
        color: Theme.warningAmber
        font.family: Typography.body
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Text {
        text: "Backup/restore: " + (actionData.backupRestoreAvailability || "Restore point recommended before live changes.")
        color: Theme.textMuted
        font.family: Typography.body
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
