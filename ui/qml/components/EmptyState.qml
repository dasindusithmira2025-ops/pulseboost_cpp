import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

Item {
    id: root
    property string title: "Nothing to show"
    property string subtitle: ""
    implicitHeight: 160
    Column {
        anchors.centerIn: parent
        spacing: Spacing.sm
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: root.title; color: Theme.textPrimary; font.family: Typography.display; font.pixelSize: Typography.cardTitle; font.weight: Font.DemiBold }
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: root.subtitle; color: Theme.textMuted; font.family: Typography.body; font.pixelSize: 13; width: Math.min(root.width - 40, 420); horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap }
    }
}
