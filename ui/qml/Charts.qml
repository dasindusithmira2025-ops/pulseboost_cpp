import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: chartsPage

    readonly property color surface: "#101b2c"
    readonly property color surfaceRaised: "#17253b"
    readonly property color border: "#24334d"
    readonly property color text: "#eef5ff"
    readonly property color textMuted: "#91a4bf"

    property var samples: SystemCtrl.chartSeries || []

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 18

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 22
                Layout.leftMargin: 22
                Layout.rightMargin: 22
                radius: 24
                color: chartsPage.surface
                border.color: chartsPage.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "Real-time performance trends"
                            color: chartsPage.text
                            font.pixelSize: 24
                            font.bold: true
                        }

                        Text {
                            text: "Telemetry is sampled on a worker thread and streamed into a rolling local cache."
                            color: chartsPage.textMuted
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        radius: 14
                        color: chartsPage.surfaceRaised
                        border.color: chartsPage.border
                        border.width: 1
                        implicitWidth: 210
                        implicitHeight: 68

                        Column {
                            anchors.centerIn: parent
                            spacing: 2

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: chartsPage.samples.length + " samples"
                                color: chartsPage.text
                                font.pixelSize: 22
                                font.bold: true
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "rolling cache"
                                color: chartsPage.textMuted
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
                Layout.bottomMargin: 22
                columns: 2
                columnSpacing: 18
                rowSpacing: 18

                TrendCard {
                    Layout.fillWidth: true
                    chartTitle: "CPU Usage"
                    chartSubtitle: "1 second cadence"
                    metricKey: "cpu"
                    accentColor: "#56b2ff"
                    currentValue: formatValue("cpu")
                }

                TrendCard {
                    Layout.fillWidth: true
                    chartTitle: "Memory Usage"
                    chartSubtitle: "1 second cadence"
                    metricKey: "ram"
                    accentColor: "#3fb950"
                    currentValue: formatValue("ram")
                }

                TrendCard {
                    Layout.fillWidth: true
                    chartTitle: "Disk Saturation"
                    chartSubtitle: "3 second cadence"
                    metricKey: "disk"
                    accentColor: "#f0883e"
                    currentValue: formatValue("disk")
                }

                TrendCard {
                    Layout.fillWidth: true
                    chartTitle: "Health Score"
                    chartSubtitle: "Derived from live telemetry"
                    metricKey: "health"
                    accentColor: "#20c997"
                    currentValue: formatValue("health")
                }
            }
        }
    }

    function latestPoint() {
        if (!chartsPage.samples || chartsPage.samples.length === 0)
            return null
        return chartsPage.samples[chartsPage.samples.length - 1]
    }

    function formatValue(key) {
        const point = latestPoint()
        if (!point)
            return "--"

        const value = Number(point[key] || 0)
        if (key === "health")
            return Math.round(value) + "/100"
        return value.toFixed(1) + "%"
    }

    function pointValue(point, key) {
        if (!point)
            return 0
        return Number(point[key] || 0)
    }

    component TrendCard: Rectangle {
        id: trendCard

        required property string chartTitle
        required property string chartSubtitle
        required property string metricKey
        required property color accentColor
        required property string currentValue

        radius: 24
        color: chartsPage.surface
        border.color: chartsPage.border
        border.width: 1
        implicitHeight: 280
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: trendCard.chartTitle
                        color: chartsPage.text
                        font.pixelSize: 17
                        font.bold: true
                    }

                    Text {
                        text: trendCard.chartSubtitle
                        color: chartsPage.textMuted
                        font.pixelSize: 12
                    }
                }

                Rectangle {
                    radius: 12
                    color: "#0d1728"
                    border.color: trendCard.accentColor
                    border.width: 1
                    implicitWidth: 92
                    implicitHeight: 38

                    Text {
                        anchors.centerIn: parent
                        text: trendCard.currentValue
                        color: trendCard.accentColor
                        font.pixelSize: 13
                        font.bold: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: chartsPage.surfaceRaised
                border.color: chartsPage.border
                border.width: 1

                Canvas {
                    id: chartCanvas
                    anchors.fill: parent
                    anchors.margins: 10
                    antialiasing: true

                    onPaint: {
                        const ctx = getContext("2d")
                        const w = width
                        const h = height
                        const padLeft = 36
                        const padTop = 14
                        const padRight = 12
                        const padBottom = 24
                        const plotWidth = Math.max(0, w - padLeft - padRight)
                        const plotHeight = Math.max(0, h - padTop - padBottom)
                        const points = chartsPage.samples || []

                        ctx.clearRect(0, 0, w, h)

                        ctx.strokeStyle = "#2b3c58"
                        ctx.lineWidth = 1
                        for (let row = 0; row <= 4; row += 1) {
                            const y = padTop + row * plotHeight / 4
                            ctx.beginPath()
                            ctx.moveTo(padLeft, y)
                            ctx.lineTo(padLeft + plotWidth, y)
                            ctx.stroke()

                            ctx.fillStyle = "#7f96b6"
                            ctx.font = "11px sans-serif"
                            ctx.textAlign = "right"
                            ctx.fillText(String(100 - row * 25), padLeft - 6, y + 4)
                        }

                        if (!points || points.length < 2)
                            return

                        const lastIndex = Math.max(1, points.length - 1)
                        const color = Qt.color(trendCard.accentColor)
                        const gradient = ctx.createLinearGradient(0, padTop, 0, padTop + plotHeight)
                        gradient.addColorStop(0, Qt.rgba(color.r, color.g, color.b, 0.34))
                        gradient.addColorStop(1, Qt.rgba(color.r, color.g, color.b, 0.0))

                        ctx.beginPath()
                        for (let i = 0; i < points.length; i += 1) {
                            const point = points[i]
                            const x = padLeft + plotWidth * i / lastIndex
                            const normalized = Math.max(0, Math.min(100, chartsPage.pointValue(point, trendCard.metricKey)))
                            const y = padTop + plotHeight * (1 - normalized / 100.0)
                            if (i === 0)
                                ctx.moveTo(x, y)
                            else
                                ctx.lineTo(x, y)
                        }
                        ctx.lineTo(padLeft + plotWidth, padTop + plotHeight)
                        ctx.lineTo(padLeft, padTop + plotHeight)
                        ctx.closePath()
                        ctx.fillStyle = gradient
                        ctx.fill()

                        ctx.beginPath()
                        for (let j = 0; j < points.length; j += 1) {
                            const sample = points[j]
                            const chartX = padLeft + plotWidth * j / lastIndex
                            const normalizedY = Math.max(0, Math.min(100, chartsPage.pointValue(sample, trendCard.metricKey)))
                            const chartY = padTop + plotHeight * (1 - normalizedY / 100.0)
                            if (j === 0)
                                ctx.moveTo(chartX, chartY)
                            else
                                ctx.lineTo(chartX, chartY)
                        }

                        ctx.lineWidth = 2.5
                        ctx.lineJoin = "round"
                        ctx.lineCap = "round"
                        ctx.strokeStyle = trendCard.accentColor
                        ctx.stroke()
                    }
                }

                Connections {
                    target: SystemCtrl

                    function onChartSeriesChanged() {
                        chartCanvas.requestPaint()
                    }
                }
            }
        }

        onWidthChanged: chartCanvas.requestPaint()
        onHeightChanged: chartCanvas.requestPaint()
        Component.onCompleted: chartCanvas.requestPaint()
    }
}
