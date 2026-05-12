import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"

Dialog {
    id: root
    property string actionName: "Action"
    property string riskLevel: "high"
    signal confirmed()

    modal: true
    title: "Confirm System Change"
    standardButtons: Dialog.Cancel
    width: 460

    contentItem: ColumnLayout {
        spacing: Spacing.md
        Text {
            text: root.actionName
            color: Theme.textPrimary
            font.family: Typography.display
            font.pixelSize: Typography.cardTitle
            font.weight: Font.Bold
            Layout.fillWidth: true
        }
        RiskBadge { level: root.riskLevel }
        Text {
            text: "Dry-run evidence, rollback availability, and audit logging stay visible. Continue only after manual review."
            color: Theme.textSecondary
            font.family: Typography.body
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Button {
            text: "Confirm Reviewed Action"
            Layout.alignment: Qt.AlignRight
            onClicked: {
                root.confirmed()
                root.close()
            }
        }
    }
}
