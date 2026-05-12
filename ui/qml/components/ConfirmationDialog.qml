import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"

Dialog {
    id: root
    property string actionName: "Action"
    property string riskLevel: "high"
    property string changeSummary: ""
    property string dryRunResult: ""
    property string backupRestore: ""
    property bool rollbackAvailable: false
    property bool typedConfirmationRequired: false
    property bool readyToConfirm: acknowledge.checked && (!typedConfirmationRequired || typedConfirm.text === "CONFIRM")
    signal confirmed()

    modal: true
    title: "Confirm System Change"
    standardButtons: Dialog.Cancel
    width: 460

    onOpened: {
        acknowledge.checked = false
        typedConfirm.text = ""
    }

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
            text: "What will change: " + (root.changeSummary || "Review required before applying this action.")
            color: Theme.textSecondary
            font.family: Typography.body
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            text: "Dry-run result: " + (root.dryRunResult || "Not available")
            color: Theme.accentCyan
            font.family: Typography.body
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            text: "Backup/restore: " + (root.backupRestore || "Restore point recommended") + " | Rollback: " + (root.rollbackAvailable ? "available" : "not guaranteed") + " | Audit logging: enabled"
            color: Theme.textMuted
            font.family: Typography.body
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        CheckBox {
            id: acknowledge
            text: "I reviewed the dry-run result and safety policy."
        }
        TextField {
            id: typedConfirm
            visible: root.typedConfirmationRequired
            Layout.fillWidth: true
            placeholderText: "Type CONFIRM for high-risk actions"
        }
        Button {
            text: "Confirm Reviewed Action"
            Layout.alignment: Qt.AlignRight
            enabled: root.readyToConfirm
            onClicked: {
                root.confirmed()
                root.close()
            }
        }
    }
}
