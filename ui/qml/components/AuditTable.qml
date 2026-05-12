import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

ColumnLayout {
    id: root
    property var entries: []
    signal entrySelected(var entry)
    spacing: Spacing.sm

    Repeater {
        model: root.entries
        delegate: Rectangle {
            required property var modelData
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: Theme.radiusLarge
            color: Theme.cardBackgroundElevated
            border.color: Theme.borderSubtle
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Spacing.md
                anchors.rightMargin: Spacing.md
                spacing: Spacing.md
                Text { text: modelData.createdAt || ""; color: Theme.textMuted; font.family: Typography.mono; font.pixelSize: 11; Layout.preferredWidth: 150; elide: Text.ElideRight }
                Text { text: modelData.actionType || ""; color: Theme.textPrimary; font.family: Typography.body; font.pixelSize: 12; Layout.preferredWidth: 150; elide: Text.ElideRight }
                RiskBadge { level: modelData.riskLevel || "manual"; size: "sm" }
                Text { text: modelData.dryRun ? "Dry-run" : "Live"; color: modelData.dryRun ? Theme.accentCyan : Theme.warningAmber; font.family: Typography.mono; font.pixelSize: 11; Layout.preferredWidth: 76 }
                Text { text: modelData.summary || ""; color: Theme.textMuted; font.family: Typography.body; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
            }
            MouseArea { anchors.fill: parent; onClicked: root.entrySelected(modelData) }
        }
    }
}
