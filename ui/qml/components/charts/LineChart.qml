import QtQuick 2.15
import "../../style"

Canvas {
    id: root
    property var data: []
    property color lineColor: Style.violet
    property real maxValue: 100
    property real minValue: 0
    property real hoverRatio: -1
    property real revealProgress: 1.0

    onDataChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onHoverRatioChanged: requestPaint()
    onRevealProgressChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        if (!data || data.length < 2) return

        const clampedReveal = Math.max(0.02, Math.min(1.0, revealProgress))
        const visibleCount = Math.max(2, Math.min(data.length, Math.floor(1 + (data.length - 1) * clampedReveal)))
        const range = Math.max(1, maxValue - minValue)
        const stepX = width / Math.max(1, visibleCount - 1)

        ctx.strokeStyle = Style.border0
        ctx.lineWidth = 1
        for (let g = 1; g <= 3; g += 1) {
            const gy = (height / 4) * g
            ctx.beginPath()
            ctx.moveTo(0, gy)
            ctx.lineTo(width, gy)
            ctx.stroke()
        }

        ctx.setLineDash([4, 4])
        const thresholdY = height - ((80 - minValue) / range) * height
        ctx.beginPath()
        ctx.moveTo(0, thresholdY)
        ctx.lineTo(width, thresholdY)
        ctx.strokeStyle = Qt.rgba(Style.warning.r, Style.warning.g, Style.warning.b, 0.4)
        ctx.stroke()
        ctx.setLineDash([])

        ctx.beginPath()
        for (let i = 0; i < visibleCount; i += 1) {
            const x = i * stepX
            const y = height - ((data[i] - minValue) / range) * height
            if (i === 0) ctx.moveTo(x, y)
            else ctx.lineTo(x, y)
        }
        const lastVisibleX = (visibleCount - 1) * stepX
        ctx.lineTo(lastVisibleX, height)
        ctx.lineTo(0, height)
        ctx.closePath()

        const gradient = ctx.createLinearGradient(0, 0, 0, height)
        gradient.addColorStop(0, Qt.rgba(lineColor.r, lineColor.g, lineColor.b, 0.24))
        gradient.addColorStop(1, Qt.rgba(lineColor.r, lineColor.g, lineColor.b, 0))
        ctx.fillStyle = gradient
        ctx.fill()

        ctx.beginPath()
        for (let j = 0; j < visibleCount; j += 1) {
            const px = j * stepX
            const py = height - ((data[j] - minValue) / range) * height
            if (j === 0) ctx.moveTo(px, py)
            else ctx.lineTo(px, py)
        }
        ctx.lineWidth = Style.s2
        ctx.strokeStyle = lineColor
        ctx.stroke()

        if (hoverRatio >= 0 && hoverRatio <= 1) {
            const hoverX = hoverRatio * width
            const hoverIndex = Math.max(0, Math.min(data.length - 1, Math.round(hoverRatio * (data.length - 1))))
            const hoverY = height - ((data[hoverIndex] - minValue) / range) * height
            ctx.strokeStyle = Qt.rgba(Style.text1.r, Style.text1.g, Style.text1.b, 0.55)
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(hoverX, 0)
            ctx.lineTo(hoverX, height)
            ctx.stroke()

            ctx.fillStyle = lineColor
            ctx.beginPath()
            ctx.arc(hoverX, hoverY, Style.s4, 0, Math.PI * 2)
            ctx.fill()
        }
    }
}
