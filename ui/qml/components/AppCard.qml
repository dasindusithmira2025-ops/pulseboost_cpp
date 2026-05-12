import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

Rectangle {
    id: root
    default property alias content: body.data
    property alias spacing: body.spacing
    property int padding: Spacing.card
    property color fillColor: Theme.cardBackground
    property color strokeColor: Theme.borderSubtle

    radius: Theme.radiusXL
    color: fillColor
    border.color: strokeColor
    border.width: 1

    ColumnLayout {
        id: body
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: Spacing.md
    }
}
