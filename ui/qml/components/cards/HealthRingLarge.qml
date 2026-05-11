import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../style"

Item {
    id: root
    property real score: 0
    property real cpuScore: 0
    property real ramScore: 0
    property real diskScore: 0
    property real trendDelta: 0

    width: 200
    height: 200

    NumberAnimation on score {
        duration: Style.xslow
        easing.type: Easing.OutCubic
    }

    Canvas {
        id: ring
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            const cx = width / 2
            const cy = height / 2
            const outerR = Math.min(width, height) / 2 - Style.s10
            const scoreR = outerR - Style.s12
            const subR = outerR - Style.s28
            const start = -Math.PI / 2

            ctx.lineWidth = Style.s2
            ctx.strokeStyle = Style.border1
            ctx.beginPath()
            ctx.arc(cx, cy, outerR, 0, Math.PI * 2)
            ctx.stroke()

            drawArc(subR, Style.cCpu, root.cpuScore, Style.s4)
            drawArc(subR - Style.s8, Style.cRam, root.ramScore, Style.s4)
            drawArc(subR - Style.s16, Style.cDisk, root.diskScore, Style.s4)

            drawArc(scoreR, Style.healthColor(root.score), root.score, Style.s12)

            function drawArc(radius, color, value, widthPx) {
                ctx.lineWidth = widthPx
                ctx.strokeStyle = color
                ctx.lineCap = "round"
                const end = start + Math.max(0, Math.min(100, value)) / 100 * Math.PI * 2
                ctx.beginPath()
                ctx.arc(cx, cy, radius, start, end)
                ctx.stroke()

                const tx = cx + Math.cos(end) * radius
                const ty = cy + Math.sin(end) * radius
                ctx.fillStyle = color
                ctx.beginPath()
                ctx.arc(tx, ty, Style.s6, 0, Math.PI * 2)
                ctx.fill()
            }
        }
    }

    onScoreChanged: ring.requestPaint()
    onCpuScoreChanged: ring.requestPaint()
    onRamScoreChanged: ring.requestPaint()
    onDiskScoreChanged: ring.requestPaint()

    Column {
        anchors.centerIn: parent
        spacing: Style.s2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Number(root.score).toFixed(0)
            color: Style.healthColor(root.score)
            font.family: Style.fontDisplay
            font.pixelSize: Style.f48
            font.weight: Style.w700
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Style.healthLabel(root.score)
            color: Style.text2
            font.family: Style.fontBody
            font.pixelSize: Style.f12
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: (root.trendDelta > 0 ? "\u2191 " : (root.trendDelta < 0 ? "\u2193 " : "\u2192 "))
                  + (root.trendDelta > 0 ? "+" : "") + Number(root.trendDelta).toFixed(0) + " today"
            color: root.trendDelta > 0 ? Style.success : (root.trendDelta < 0 ? Style.danger : Style.text2)
            font.family: Style.fontMono
            font.pixelSize: Style.f10
        }
    }
}
