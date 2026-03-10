import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: chatPage

    readonly property color surface: "#101b2c"
    readonly property color surfaceRaised: "#17253b"
    readonly property color border: "#24334d"
    readonly property color text: "#eef5ff"
    readonly property color textMuted: "#91a4bf"
    readonly property color accent: "#56b2ff"
    readonly property color accentSoft: "#173251"
    readonly property color success: "#3fb950"
    readonly property color waitingColor: "#d29922"

    property bool waiting: false
    property var quickPrompts: [
        "Why is my PC slow?",
        "Analyze performance",
        "Optimize my PC",
        "Boost gaming"
    ]

    ListModel { id: messages }

    function appendUserMessage(text) {
        messages.append({ "senderRole": "user", "body": text })
    }

    function appendAgentMessage(text) {
        messages.append({ "senderRole": "assistant", "body": text.replace(/\n/g, "<br>") })
    }

    function submitPrompt(promptText) {
        const prompt = String(promptText || "").trim()
        if (prompt === "" || waiting)
            return

        appendUserMessage(prompt)
        composer.text = ""
        waiting = true
        AgentEngine.ask(prompt)
    }

    Component.onCompleted: {
        appendAgentMessage(
                    "<b>PulseBoost AI</b><br><br>"
                    + "Ask for a diagnosis, a safe cleanup, startup guidance, or gaming optimization.<br><br>"
                    + "Suggested starting points:<br>"
                    + "- Why is my PC slow?<br>"
                    + "- Optimize my PC<br>"
                    + "- Analyze performance")
    }

    Connections {
        target: AgentEngine

        function onAgentReply(reply, actionTaken) {
            waiting = false
            appendAgentMessage(reply)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 18

        Rectangle {
            Layout.fillWidth: true
            radius: 24
            color: chatPage.surface
            border.color: chatPage.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "AI optimization agent"
                            color: chatPage.text
                            font.pixelSize: 24
                            font.bold: true
                        }

                        Text {
                            text: "The agent combines live telemetry, optimization history, and a local or remote LLM endpoint."
                            color: chatPage.textMuted
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        radius: 14
                        color: chatPage.surfaceRaised
                        border.color: chatPage.waiting ? chatPage.waitingColor : chatPage.success
                        border.width: 1
                        implicitWidth: 158
                        implicitHeight: 60

                        Row {
                            anchors.centerIn: parent
                            spacing: 8

                            Rectangle {
                                width: 10
                                height: 10
                                radius: 5
                                color: chatPage.waiting ? chatPage.waitingColor : chatPage.success
                            }

                            Text {
                                text: chatPage.waiting ? "Reasoning..." : "Ready"
                                color: chatPage.text
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    Repeater {
                        model: chatPage.quickPrompts.length

                        delegate: Rectangle {
                            property string prompt: chatPage.quickPrompts[index] || ""
                            radius: 12
                            color: chipHover.hovered ? Qt.lighter(chatPage.accentSoft, 1.22) : chatPage.surfaceRaised
                            border.color: chatPage.border
                            border.width: 1
                            implicitWidth: chipLabel.implicitWidth + 26
                            implicitHeight: 36

                            Behavior on color {
                                ColorAnimation { duration: 120 }
                            }

                            Text {
                                id: chipLabel
                                anchors.centerIn: parent
                                text: prompt
                                color: chatPage.text
                                font.pixelSize: 12
                                font.bold: true
                            }

                            HoverHandler { id: chipHover }
                            TapHandler { onTapped: submitPrompt(prompt) }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 24
            color: chatPage.surface
            border.color: chatPage.border
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                ListView {
                    id: chatList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 12
                    model: messages
                    boundsBehavior: Flickable.StopAtBounds
                    leftMargin: 18
                    rightMargin: 18
                    topMargin: 18
                    bottomMargin: 18

                    onCountChanged: Qt.callLater(function() { chatList.positionViewAtEnd() })

                    delegate: Item {
                        width: chatList.width - chatList.leftMargin - chatList.rightMargin
                        height: bubble.implicitHeight + 10

                        readonly property bool fromUser: senderRole === "user"

                        Rectangle {
                            id: bubble
                            width: Math.min(parent.width * 0.76, bubbleText.implicitWidth + 28)
                            implicitHeight: bubbleText.implicitHeight + 26
                            x: fromUser ? parent.width - width : 0
                            radius: 18
                            color: fromUser ? chatPage.accentSoft : chatPage.surfaceRaised
                            border.color: fromUser ? chatPage.accent : chatPage.border
                            border.width: 1
                            opacity: 0

                            Column {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 6

                                Text {
                                    text: fromUser ? "You" : "PulseBoost AI"
                                    color: fromUser ? "#a5d8ff" : chatPage.textMuted
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                Text {
                                    id: bubbleText
                                    width: parent.width
                                    text: body
                                    textFormat: Text.StyledText
                                    color: chatPage.text
                                    font.pixelSize: 13
                                    wrapMode: Text.Wrap
                                }
                            }

                            Component.onCompleted: bubble.opacity = 1

                            Behavior on opacity {
                                NumberAnimation { duration: 160 }
                            }
                        }
                    }

                    footer: Item {
                        width: chatList.width
                        height: chatPage.waiting ? 44 : 0
                        visible: chatPage.waiting

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 2
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8

                            Repeater {
                                model: 3

                                delegate: Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: chatPage.textMuted

                                    SequentialAnimation on opacity {
                                        running: chatPage.waiting
                                        loops: Animation.Infinite
                                        PauseAnimation { duration: index * 120 }
                                        NumberAnimation { from: 0.2; to: 1.0; duration: 260 }
                                        NumberAnimation { from: 1.0; to: 0.2; duration: 260 }
                                    }
                                }
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {}
                }

                Rectangle {
                    Layout.fillWidth: true
                    color: chatPage.surfaceRaised
                    border.color: chatPage.border
                    border.width: 1
                    implicitHeight: 92

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        TextField {
                            id: composer
                            Layout.fillWidth: true
                            enabled: !chatPage.waiting
                            placeholderText: "Ask for diagnosis, cleanup, startup advice, or safe optimization."
                            color: chatPage.text
                            placeholderTextColor: chatPage.textMuted
                            font.pixelSize: 14
                            selectByMouse: true

                            background: Rectangle {
                                radius: 16
                                color: "#0c1524"
                                border.color: composer.activeFocus ? chatPage.accent : chatPage.border
                                border.width: composer.activeFocus ? 2 : 1
                            }

                            Keys.onReturnPressed: submitPrompt(composer.text)
                            Keys.onEnterPressed: submitPrompt(composer.text)
                        }

                        Rectangle {
                            width: 120
                            height: 52
                            radius: 16
                            color: sendHover.hovered ? Qt.lighter(chatPage.accent, 1.12) : chatPage.accent
                            opacity: chatPage.waiting || composer.text.trim() === "" ? 0.55 : 1

                            Behavior on color {
                                ColorAnimation { duration: 120 }
                            }

                            Behavior on opacity {
                                NumberAnimation { duration: 120 }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "Send"
                                color: "white"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            HoverHandler { id: sendHover }
                            TapHandler {
                                enabled: !chatPage.waiting && composer.text.trim() !== ""
                                onTapped: submitPrompt(composer.text)
                            }
                        }
                    }
                }
            }
        }
    }
}
