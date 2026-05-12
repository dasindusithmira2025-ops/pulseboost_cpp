import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

AppCard {
    id: root
    property var entry: ({})
    SectionHeader {
        title: "Audit Detail"
        subtitle: entry.actionType || "Select an audit row."
    }
    RiskBadge { level: entry.riskLevel || "manual" }
    Text { text: "Human explanation: " + (entry.summary || "No entry selected."); color: Theme.textSecondary; font.family: Typography.body; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Text { text: "Confirmation: " + (entry.confirmation || (entry.dryRun ? "Dry-run only" : "Recorded live action")); color: Theme.textMuted; font.family: Typography.body; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Text { text: "Safety policy result: audit recorded, rollback visibility retained when available."; color: Theme.textMuted; font.family: Typography.body; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
}
