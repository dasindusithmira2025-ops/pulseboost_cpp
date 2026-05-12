import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

AppCard {
    id: root
    property string title: "Recommendation"
    property string reason: ""
    property string confidence: "Medium"
    property string evidence: ""
    property string risk: "manual"
    implicitHeight: 156
    SectionHeader { title: root.title; subtitle: root.reason }
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.sm
        RiskBadge { level: root.risk; size: "sm" }
        StatusPill { text: "Confidence " + root.confidence; tone: Theme.accentCyan }
    }
    Text { text: root.evidence; color: Theme.textMuted; font.family: Typography.body; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
}
