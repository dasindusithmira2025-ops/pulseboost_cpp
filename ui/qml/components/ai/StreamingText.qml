import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../style"

Item {
    id: root
    property string fullText: ""
    property bool isStreaming: false
    property int revealedWords: 0

    implicitHeight: flow.implicitHeight
    implicitWidth: flow.implicitWidth

    Flow {
        id: flow
        width: parent.width
        spacing: 0

        Repeater {
            model: root.fullText.length === 0 ? [] : root.fullText.split(" ")
            delegate: Text {
                required property string modelData
                required property int index
                text: index < root.revealedWords ? (index === 0 ? modelData : " " + modelData) : ""
                color: Style.text0
                font.family: Style.fontBody
                font.pixelSize: Style.f13
                opacity: index < root.revealedWords ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Style.fast } }
            }
        }

        Rectangle {
            width: Style.s2
            height: Style.s16
            radius: Style.r4
            color: Style.violet
            visible: root.isStreaming
            SequentialAnimation on opacity {
                running: root.isStreaming
                loops: Animation.Infinite
                NumberAnimation { to: 0; duration: Style.slow }
                NumberAnimation { to: 1; duration: Style.slow }
            }
        }
    }

    Timer {
        interval: 30
        running: root.isStreaming && root.revealedWords < (root.fullText.length === 0 ? 0 : root.fullText.split(" ").length)
        repeat: true
        onTriggered: root.revealedWords += 1
    }

    onFullTextChanged: {
        if (!isStreaming) {
            revealedWords = fullText.length === 0 ? 0 : fullText.split(" ").length
        }
    }
    onIsStreamingChanged: {
        if (!isStreaming) {
            revealedWords = fullText.length === 0 ? 0 : fullText.split(" ").length
        }
    }
}
