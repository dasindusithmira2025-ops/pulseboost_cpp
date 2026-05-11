import QtQuick 2.15
import "../../style"

Canvas {
    id: root
    property var data: []
    property color lineColor: Style.violet
    property real maxValue: 100
    property real minValue: 0

    onDataChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        if (!data || data.length < 2) return

        const stepX = width / (data.length - 1)
        const range = Math.max(1, maxValue - minValue)

        ctx.beginPath()
        for (let i = 0; i < data.length; i += 1) {
            const x = i * stepX
            const y = height - ((data[i] - minValue) / range) * height
            if (i === 0) ctx.moveTo(x, y)
            else ctx.lineTo(x, y)
        }
        ctx.lineWidth = Style.s2
        ctx.strokeStyle = lineColor
        ctx.stroke()
    }
}
