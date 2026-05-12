import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

AppCard {
    id: root
    property string label: "Metric"
    property string value: "--"
    property string detail: ""
    property color accent: Theme.accentCyan

    implicitHeight: 108
    spacing: Spacing.xs

    Text {
        text: root.label
        color: Theme.textMuted
        font.family: Typography.mono
        font.pixelSize: Typography.tinyMeta
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
    Text {
        text: root.value
        color: root.accent
        font.family: Typography.display
        font.pixelSize: Typography.metricValue
        font.weight: Font.Bold
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
    Text {
        text: root.detail
        color: Theme.textMuted
        font.family: Typography.body
        font.pixelSize: Typography.tinyMeta
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
}
