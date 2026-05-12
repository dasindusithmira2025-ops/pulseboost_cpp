import QtQuick 2.15
import "../theme"

Item {
    id: root
    property real score: 0
    property string label: "PC Health"
    property color ringColor: score >= 85 ? Theme.successGreen : (score >= 65 ? Theme.accentCyan : (score >= 45 ? Theme.warningAmber : Theme.highRed))

    implicitWidth: 180
    implicitHeight: 180

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const line = 14
            const cx = width / 2
            const cy = height / 2
            const r = Math.min(width, height) / 2 - line
            ctx.lineCap = "round"
            ctx.lineWidth = line
            ctx.strokeStyle = Theme.cardBackgroundElevated
            ctx.beginPath()
            ctx.arc(cx, cy, r, 0, Math.PI * 2)
            ctx.stroke()
            ctx.strokeStyle = root.ringColor
            ctx.beginPath()
            ctx.arc(cx, cy, r, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * Math.max(0, Math.min(100, root.score)) / 100)
            ctx.stroke()
        }
        Connections {
            target: root
            function onScoreChanged() { canvas.requestPaint() }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 2
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Number(root.score).toFixed(0)
            color: Theme.textPrimary
            font.family: Typography.display
            font.pixelSize: 46
            font.weight: Font.Bold
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            color: Theme.textMuted
            font.family: Typography.body
            font.pixelSize: 12
        }
    }
}
