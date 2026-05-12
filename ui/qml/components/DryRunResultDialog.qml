import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"

Dialog {
    id: root
    property string resultText: "No dry-run result is available yet."
    modal: true
    title: "Dry-run Result"
    standardButtons: Dialog.Ok
    width: 520
    contentItem: Text {
        text: root.resultText
        color: Theme.textSecondary
        font.family: Typography.body
        font.pixelSize: 13
        wrapMode: Text.WordWrap
    }
}
