import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/foundation"
import "../components/ai"

Item {
    id: root
    anchors.fill: parent

    property bool streaming: false
    property string currentSessionId: "local-session"
    property string pendingTemplateId: ""
    property string pendingActionId: ""
    property string pendingActionLabel: ""

    ListModel { id: chatModel }

    function sendMessage(text) {
        var trimmed = (text || "").trim()
        if (trimmed.length === 0 || root.streaming) return
        chatModel.append({ "role": "user", "content": trimmed, "timestamp": Qt.formatTime(new Date(), "hh:mm:ss"), "streaming": false, "actionResult": false, "success": true, "sessionId": root.currentSessionId, "templateId": "", "hasAction": false, "actionId": "", "actionLabel": "" })
        input.text = ""
        root.streaming = true
        AgentEngine.ask(trimmed)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Style.s16

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 118
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s6
                    Text { text: "AI Advisor"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "AI Advisory Mode: AI can recommend actions and prepare dry-runs, but medium/high-risk actions require confirmation."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                Column {
                    spacing: Style.s4
                    StatusPill { text: SystemCtrl.aiMode === "cloud" ? "CLOUD" : "LOCAL"; tone: SystemCtrl.aiMode === "cloud" ? "warning" : "success" }
                    Text { text: "Health " + Number(SystemCtrl.healthScore).toFixed(0) + "  |  CPU " + Number(SystemCtrl.cpuUsage).toFixed(0) + "%"; color: Style.text1; font.family: Style.fontMono; font.pixelSize: Style.f11 }
                }
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Style.s12
            clip: true
            model: chatModel
            ScrollBar.vertical: ScrollBar { visible: false }

            delegate: ChatBubble {
                width: list.width
                role: model.role
                content: model.content
                timestamp: model.timestamp
                isStreaming: model.streaming
                actionResult: model.actionResult
                success: model.success
                sessionId: model.sessionId
                templateId: model.templateId
                hasAction: model.hasAction
                actionId: model.actionId
                actionLabel: model.actionLabel
            }

            onCountChanged: positionViewAtEnd()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: Style.r10
            color: Style.bg2
            border.color: Style.border1
            border.width: 1
            visible: chatModel.count === 0
            Text {
                anchors.centerIn: parent
                text: "Try: Analyze current bottlenecks | Prepare a safe dry-run | Explain my health score"
                color: Style.text2
                font.family: Style.fontBody
                font.pixelSize: Style.f12
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            fillColor: Style.bg2
            borderColor: Style.border1
            contentMargin: Style.s10
            RowLayout {
                anchors.fill: parent
                spacing: Style.s12
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Style.r10
                    color: Style.bg1
                    border.color: input.activeFocus ? Style.cyan : Style.border1
                    border.width: 1
                    TextArea {
                        id: input
                        anchors.fill: parent
                        anchors.margins: Style.s10
                        background: null
                        color: Style.text0
                        font.family: Style.fontBody
                        font.pixelSize: Style.f14
                        placeholderText: "Ask PulseBoost AI anything about this system"
                        placeholderTextColor: Style.text3
                        wrapMode: Text.Wrap
                        Keys.onReturnPressed: function(event) {
                            if (event.modifiers & Qt.ShiftModifier) return
                            event.accepted = true
                            root.sendMessage(input.text)
                        }
                    }
                }
                GlowButton {
                    Layout.preferredWidth: 120
                    label: root.streaming ? "Working" : "Send"
                    glowColor: Style.cyan
                    onClicked: root.sendMessage(input.text)
                }
            }
        }
    }

    Connections {
        target: AgentEngine
        function onAgentActionMetadata(sessionId, templateId, actionId, actionLabel) {
            root.currentSessionId = sessionId
            root.pendingTemplateId = templateId
            root.pendingActionId = actionId
            root.pendingActionLabel = actionLabel
        }
        function onAgentReplyChunk(chunk) {
            if (chatModel.count === 0 || chatModel.get(chatModel.count - 1).role !== "assistant" || !chatModel.get(chatModel.count - 1).streaming) {
                chatModel.append({ "role": "assistant", "content": chunk, "timestamp": Qt.formatTime(new Date(), "hh:mm:ss"), "streaming": true, "actionResult": false, "success": true, "sessionId": root.currentSessionId, "templateId": root.pendingTemplateId, "hasAction": false, "actionId": "", "actionLabel": "" })
            } else {
                var idx = chatModel.count - 1
                chatModel.setProperty(idx, "content", chatModel.get(idx).content + chunk)
            }
            root.streaming = true
        }
        function onAgentReply(reply, actionTaken) {
            if (chatModel.count > 0 && chatModel.get(chatModel.count - 1).role === "assistant") {
                var idx = chatModel.count - 1
                chatModel.setProperty(idx, "content", reply)
                chatModel.setProperty(idx, "streaming", false)
                chatModel.setProperty(idx, "actionResult", actionTaken)
                chatModel.setProperty(idx, "hasAction", root.pendingActionId !== "")
                chatModel.setProperty(idx, "actionId", root.pendingActionId)
                chatModel.setProperty(idx, "actionLabel", root.pendingActionLabel)
            } else {
                chatModel.append({ "role": "assistant", "content": reply, "timestamp": Qt.formatTime(new Date(), "hh:mm:ss"), "streaming": false, "actionResult": actionTaken, "success": true, "sessionId": root.currentSessionId, "templateId": root.pendingTemplateId, "hasAction": root.pendingActionId !== "", "actionId": root.pendingActionId, "actionLabel": root.pendingActionLabel })
            }
            root.pendingActionId = ""
            root.pendingActionLabel = ""
            root.streaming = false
        }
    }
}
