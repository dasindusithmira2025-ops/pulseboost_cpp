import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

AppCard {
    id: root
    property string title: "Proof"
    property string beforeValue: "--"
    property string afterValue: "--"
    property string delta: ""
    SectionHeader { title: root.title; subtitle: root.delta }
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.md
        Text { text: "Before " + root.beforeValue; color: Theme.textMuted; font.family: Typography.body; font.pixelSize: 13; Layout.fillWidth: true }
        Text { text: "After " + root.afterValue; color: Theme.successGreen; font.family: Typography.body; font.pixelSize: 13; Layout.fillWidth: true }
    }
}
