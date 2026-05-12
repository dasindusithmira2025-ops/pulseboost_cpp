import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

AppCard {
    id: root
    property var actionData: ({})
    signal dryRunRequested()
    signal reviewRequested()
    signal applyRequested()

    implicitHeight: 178
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Spacing.lg
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Spacing.sm
            RowLayout {
                spacing: Spacing.sm
                RiskBadge { level: root.actionData.riskLevel || "manual"; size: "sm" }
                StatusPill { text: root.actionData.dryRunSupported ? "Dry-run available" : "Live gated"; tone: root.actionData.dryRunSupported ? Theme.successGreen : Theme.warningAmber }
                StatusPill { text: root.actionData.rollbackAvailable ? "Rollback available" : "Restore recommended"; tone: root.actionData.rollbackAvailable ? Theme.successGreen : Theme.warningAmber }
            }
            Text {
                text: root.actionData.name || "Action"
                color: Theme.textPrimary
                font.family: Typography.display
                font.pixelSize: Typography.cardTitle
                font.weight: Font.Bold
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Text {
                text: root.actionData.expectedEffect || "Review the dry-run before applying changes."
                color: Theme.textMuted
                font.family: Typography.body
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Text {
                text: "Dry-run result: " + (root.actionData.dryRunResult || "Not run yet")
                color: Theme.accentCyan
                font.family: Typography.body
                font.pixelSize: 12
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }
}
