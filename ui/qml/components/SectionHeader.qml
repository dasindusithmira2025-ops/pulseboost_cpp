import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

ColumnLayout {
    id: root
    property string title: "Section"
    property string subtitle: ""
    spacing: 2
    Text {
        text: root.title
        color: Theme.textPrimary
        font.family: Typography.display
        font.pixelSize: Typography.sectionTitle
        font.weight: Font.DemiBold
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
    Text {
        text: root.subtitle
        visible: root.subtitle.length > 0
        color: Theme.textMuted
        font.family: Typography.body
        font.pixelSize: Typography.tinyMeta
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }
}
