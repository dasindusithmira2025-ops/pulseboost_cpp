import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

AppCard {
    id: root
    property string title: "Restore Path"
    property string detail: ""
    property string status: ""
    property string type: "Manual"
    implicitHeight: 112
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.md
        RiskBadge { level: root.type.toLowerCase().indexOf("snapshot") !== -1 ? "safe" : "manual"; size: "sm" }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text { text: root.title; color: Theme.textPrimary; font.family: Typography.body; font.pixelSize: 14; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
            Text { text: root.detail; color: Theme.textMuted; font.family: Typography.body; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
            Text { text: root.status; color: Theme.accentCyan; font.family: Typography.mono; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
        }
    }
}
