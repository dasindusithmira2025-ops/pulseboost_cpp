import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../style"

Item {
    id: root

    function push(title, message, tone) {
        listModel.append({
            "title": title,
            "message": message,
            "tone": tone
        })
        if (listModel.count > 3) {
            listModel.remove(0)
        }
    }

    ListModel { id: listModel }

    Column {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: Style.s20
        anchors.bottomMargin: Style.s20
        spacing: Style.s8

        Repeater {
            model: listModel
            delegate: ToastNotification {
                required property string title
                required property string message
                required property string tone

                title: model.title
                message: model.message
                tone: model.tone
                onDismissed: listModel.remove(index)

                x: appeared ? 0 : width
                property bool appeared: false
                Component.onCompleted: appeared = true

                Behavior on x { NumberAnimation { duration: Style.normal; easing.type: Easing.OutCubic } }

                Timer {
                    running: tone !== "error"
                    repeat: false
                    interval: tone === "warning" ? 6000 : 4000
                    onTriggered: listModel.remove(index)
                }
            }
        }
    }
}
