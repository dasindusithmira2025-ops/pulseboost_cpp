import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../style"

Item {
    id: root

    property string role: "assistant"
    property string content: ""
    property string timestamp: ""
    property bool isStreaming: false
    property bool actionResult: false
    property bool success: true
    property string actionId: ""
    property string actionLabel: ""
    property bool hasAction: false
    property string templateId: ""
    property string sessionId: "local-session"
    property int feedbackScore: 0

    width: parent ? parent.width : 800
    height: bubbleColumn.height + Style.s8

    Column {
        id: bubbleColumn
        width: parent.width
        spacing: Style.s4

        Item {
            width: parent.width
            height: userBubble.height + tsUser.implicitHeight + Style.s4
            visible: root.role === "user"

            Rectangle {
                id: userBubble
                anchors.right: parent.right
                anchors.rightMargin: Style.s16
                width: Math.min(userText.implicitWidth + Style.s32, parent.width * 0.65)
                height: userText.implicitHeight + Style.s24
                color: Style.violet
                radius: Style.r16

                Rectangle {
                    width: Style.r16
                    height: Style.r16
                    color: Style.violet
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                }

                Text {
                    id: userText
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        margins: Style.s16
                    }
                    text: root.content
                    font.family: Style.fontBody
                    font.pixelSize: Style.f14
                    color: Style.text0
                    wrapMode: Text.WordWrap
                    lineHeight: 1.5
                }
            }

            Text {
                id: tsUser
                anchors.right: userBubble.right
                anchors.top: userBubble.bottom
                anchors.topMargin: Style.s4
                text: root.timestamp
                font.family: Style.fontMono
                font.pixelSize: Style.f10
                color: Style.text3
            }
        }

        Item {
            width: parent.width
            height: assistantLayout.height
            visible: root.role === "assistant"

            Column {
                id: assistantLayout
                anchors.left: parent.left
                anchors.leftMargin: Style.s16
                width: parent.width - Style.s32
                spacing: Style.s4

                Rectangle {
                    width: Math.min(assistantContent.implicitWidth + Style.s32, parent.width * 0.78)
                    height: assistantContent.implicitHeight + Style.s24
                    color: Style.bg3
                    radius: Style.r16

                    Rectangle {
                        width: Style.r16
                        height: Style.r16
                        color: Style.bg3
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                    }

                    Rectangle {
                        width: 3
                        height: parent.height - Style.r16
                        color: actionResult ? (success ? Style.green : Style.red) : Style.violet
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.topMargin: Style.r16
                        radius: 2
                    }

                    Column {
                        id: assistantContent
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            leftMargin: Style.s20
                            rightMargin: Style.s16
                            topMargin: Style.s12
                        }
                        spacing: Style.s10

                        Loader {
                            width: parent.width
                            sourceComponent: root.isStreaming ? streamingComponent : staticTextComponent
                        }

                        Component {
                            id: staticTextComponent
                            Text {
                                width: parent.width
                                text: root.content
                                font.family: Style.fontBody
                                font.pixelSize: Style.f14
                                color: Style.text0
                                wrapMode: Text.WordWrap
                                lineHeight: 1.6
                            }
                        }

                        Component {
                            id: streamingComponent
                            StreamingText {
                                width: parent.width
                                fullText: root.content
                                isStreaming: root.isStreaming
                            }
                        }

                        Rectangle {
                            visible: root.hasAction && root.actionId !== ""
                            width: actionRow.implicitWidth + Style.s24
                            height: Style.s32
                            color: Style.violetGlow
                            border.color: Style.violet
                            border.width: 1
                            radius: Style.r8

                            Row {
                                id: actionRow
                                anchors.centerIn: parent
                                spacing: Style.s8

                                Text {
                                    text: Icons.glyph("bolt")
                                    font.family: Style.fontDisplay
                                    font.pixelSize: Style.f13
                                    color: Style.violet
                                }
                                Text {
                                    text: root.actionLabel !== "" ? root.actionLabel : "Run Action"
                                    font.family: Style.fontBody
                                    font.pixelSize: Style.f13
                                    font.weight: Style.w500
                                    color: Style.violet
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: AgentEngine.executeAction(root.actionId)
                            }
                        }
                    }
                }

                Row {
                    spacing: Style.s12
                    leftPadding: Style.s4

                    Text {
                        text: root.timestamp
                        font.family: Style.fontMono
                        font.pixelSize: Style.f10
                        color: Style.text3
                    }

                    Row {
                        spacing: Style.s4
                        visible: root.role === "assistant" && !root.isStreaming

                        Rectangle {
                            width: 24
                            height: 24
                            radius: Style.r4
                            color: thumbsUpArea.containsMouse ? Style.bg4 : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "+"
                                font.family: Style.fontMono
                                font.pixelSize: Style.f12
                                color: Style.green
                                opacity: root.feedbackScore === 1 ? 1.0 : (thumbsUpArea.containsMouse ? 1.0 : 0.55)
                            }
                            MouseArea {
                                id: thumbsUpArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.feedbackScore = 1
                                    AgentEngine.submitFeedback(root.sessionId, root.templateId, true)
                                }
                            }
                        }

                        Rectangle {
                            width: 24
                            height: 24
                            radius: Style.r4
                            color: thumbsDownArea.containsMouse ? Style.bg4 : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "-"
                                font.family: Style.fontMono
                                font.pixelSize: Style.f12
                                color: Style.red
                                opacity: root.feedbackScore === -1 ? 1.0 : (thumbsDownArea.containsMouse ? 1.0 : 0.55)
                            }
                            MouseArea {
                                id: thumbsDownArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.feedbackScore = -1
                                    AgentEngine.submitFeedback(root.sessionId, root.templateId, false)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
