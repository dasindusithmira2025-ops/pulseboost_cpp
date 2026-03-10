import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: dashboard

    readonly property color surface: "#101b2c"
    readonly property color surfaceRaised: "#17253b"
    readonly property color border: "#24334d"
    readonly property color accent: "#56b2ff"
    readonly property color accent2: "#20c997"
    readonly property color text: "#eef5ff"
    readonly property color textMuted: "#91a4bf"
    readonly property color success: "#3fb950"
    readonly property color warning: "#d29922"
    readonly property color danger: "#f85149"

    property var metricModel: [
        {
            "title": "CPU",
            "value": SystemCtrl.cpuUsage,
            "unit": "%",
            "subtitle": "Live processor load",
            "accent": toneFor(SystemCtrl.cpuUsage, 55, 82)
        },
        {
            "title": "Memory",
            "value": SystemCtrl.ramUsage,
            "unit": "%",
            "subtitle": "Working set pressure",
            "accent": toneFor(SystemCtrl.ramUsage, 60, 85)
        },
        {
            "title": "Disk",
            "value": SystemCtrl.diskUsage,
            "unit": "%",
            "subtitle": "System volume occupancy",
            "accent": toneFor(SystemCtrl.diskUsage, 70, 90)
        },
        {
            "title": "Network",
            "value": SystemCtrl.networkMbps,
            "unit": " Mbps",
            "subtitle": "Current throughput",
            "accent": "#58a6ff"
        }
    ]

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
                radius: 26
                color: dashboard.surface
                border.color: dashboard.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 24

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "System command center"
                            color: dashboard.text
                            font.pixelSize: 30
                            font.bold: true
                        }

                        Text {
                            text: SystemCtrl.summary !== "" ? SystemCtrl.summary : "Collecting system telemetry."
                            color: dashboard.textMuted
                            font.pixelSize: 14
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 10

                            InfoBadge {
                                badgeLabel: "Health"
                                badgeValue: SystemCtrl.healthLabel
                                accentColor: toneFor(SystemCtrl.healthScore, 60, 82)
                            }

                            InfoBadge {
                                badgeLabel: "Startup"
                                badgeValue: SystemCtrl.startupCount + " entries"
                                accentColor: dashboard.warning
                            }

                            InfoBadge {
                                badgeLabel: "Actions"
                                badgeValue: (SystemCtrl.recentActions ? SystemCtrl.recentActions.length : 0) + " logged"
                                accentColor: dashboard.accent
                            }
                        }

                        GridLayout {
                            Layout.topMargin: 10
                            columns: 2
                            columnSpacing: 14
                            rowSpacing: 14

                            ActionButton {
                                buttonLabel: "Safe Cleanup"
                                buttonHint: "Clear disposable temp files and caches."
                                accentColor: dashboard.accent2
                                onTriggered: SystemCtrl.runClean()
                            }

                            ActionButton {
                                buttonLabel: "Restore Then Optimize"
                                buttonHint: "Create a restore point and run safe cleanup."
                                accentColor: dashboard.accent
                                onTriggered: SystemCtrl.runOptimize()
                            }

                            ActionButton {
                                buttonLabel: "Game Focus"
                                buttonHint: "Raise priority for the most likely active game."
                                accentColor: "#8a63ff"
                                onTriggered: SystemCtrl.enableGameMode()
                            }

                            ActionButton {
                                buttonLabel: "Restore Point"
                                buttonHint: "Create a manual system restore checkpoint."
                                accentColor: dashboard.warning
                                onTriggered: SystemCtrl.createRestorePoint()
                            }
                        }
                    }

                    HealthDial {
                        Layout.alignment: Qt.AlignTop
                        score: SystemCtrl.healthScore
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
                columns: 2
                rowSpacing: 18
                columnSpacing: 18

                Repeater {
                    model: dashboard.metricModel.length

                    delegate: Item {
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: 156

                        MetricTile {
                            anchors.fill: parent
                            metricData: dashboard.metricModel[parent.index] || ({})
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
                Layout.bottomMargin: 22
                spacing: 18

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    radius: 24
                    color: dashboard.surface
                    border.color: dashboard.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 16

                        Text {
                            text: "Recent optimization history"
                            color: dashboard.text
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            text: "Each action is recorded so the AI agent can reason from prior changes."
                            color: dashboard.textMuted
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Repeater {
                                model: Math.min(6, SystemCtrl.recentActions ? SystemCtrl.recentActions.length : 0)

                                delegate: Rectangle {
                                    property var record: (SystemCtrl.recentActions && index < SystemCtrl.recentActions.length)
                                                         ? SystemCtrl.recentActions[index]
                                                         : ({})
                                    Layout.fillWidth: true
                                    implicitHeight: 58
                                    radius: 16
                                    color: dashboard.surfaceRaised
                                    border.color: dashboard.border
                                    border.width: 1

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 14
                                        spacing: 12

                                        Rectangle {
                                            width: 10
                                            height: 10
                                            radius: 5
                                            color: record.success ? dashboard.success : dashboard.danger
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            Text {
                                                text: humanizeAction(record.action)
                                                color: dashboard.text
                                                font.pixelSize: 13
                                                font.bold: true
                                            }

                                            Text {
                                                text: record.details || ""
                                                color: dashboard.textMuted
                                                font.pixelSize: 12
                                                wrapMode: Text.Wrap
                                                Layout.fillWidth: true
                                            }
                                        }

                                        Text {
                                            text: shortTimestamp(record.timestamp)
                                            color: dashboard.textMuted
                                            font.pixelSize: 11
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 64
                                radius: 16
                                visible: !SystemCtrl.recentActions || SystemCtrl.recentActions.length === 0
                                color: dashboard.surfaceRaised
                                border.color: dashboard.border
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "No optimization actions have been recorded yet."
                                    color: dashboard.textMuted
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    radius: 24
                    color: dashboard.surface
                    border.color: dashboard.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 16

                        Text {
                            text: "Storage outlook"
                            color: dashboard.text
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            text: "Largest categories on the system drive based on the latest scan."
                            color: dashboard.textMuted
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Repeater {
                                model: Math.min(5, SystemCtrl.storageCategories ? SystemCtrl.storageCategories.length : 0)

                                delegate: Rectangle {
                                    property var entry: (SystemCtrl.storageCategories && index < SystemCtrl.storageCategories.length)
                                                        ? SystemCtrl.storageCategories[index]
                                                        : ({})
                                    Layout.fillWidth: true
                                    implicitHeight: 66
                                    radius: 18
                                    color: dashboard.surfaceRaised
                                    border.color: dashboard.border
                                    border.width: 1

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 14
                                        spacing: 8

                                        RowLayout {
                                            Layout.fillWidth: true

                                            Text {
                                                text: entry.name || "Unknown"
                                                color: dashboard.text
                                                font.pixelSize: 13
                                                font.bold: true
                                            }

                                            Item { Layout.fillWidth: true }

                                            Text {
                                                text: formatBytes(entry.bytes || 0)
                                                color: dashboard.textMuted
                                                font.pixelSize: 12
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 8
                                            radius: 4
                                            color: "#0e1624"

                                            Rectangle {
                                                width: parent.width * Math.min(1, entry.fraction || 0)
                                                height: parent.height
                                                radius: parent.radius
                                                color: entry.accent || dashboard.accent

                                                Behavior on width {
                                                    NumberAnimation { duration: 550; easing.type: Easing.OutCubic }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 64
                                radius: 16
                                visible: !SystemCtrl.storageCategories || SystemCtrl.storageCategories.length === 0
                                color: dashboard.surfaceRaised
                                border.color: dashboard.border
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "Storage categories will appear after the next disk sample."
                                    color: dashboard.textMuted
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function toneFor(value, warnAt, dangerAt) {
        if (value >= dangerAt)
            return dashboard.danger
        if (value >= warnAt)
            return dashboard.warning
        return dashboard.success
    }

    function shortTimestamp(value) {
        if (!value)
            return ""
        if (value.length >= 16)
            return value.slice(11, 16)
        return value
    }

    function humanizeAction(value) {
        if (!value)
            return "Action"
        const spaced = value.replace(/-/g, " ")
        return spaced.charAt(0).toUpperCase() + spaced.slice(1)
    }

    function formatBytes(bytes) {
        if (bytes >= 1073741824)
            return (bytes / 1073741824).toFixed(1) + " GB"
        if (bytes >= 1048576)
            return (bytes / 1048576).toFixed(0) + " MB"
        if (bytes >= 1024)
            return (bytes / 1024).toFixed(0) + " KB"
        return bytes + " B"
    }

    component InfoBadge: Rectangle {
        required property string badgeLabel
        required property string badgeValue
        required property color accentColor
        radius: 12
        color: "#0d1728"
        border.color: accentColor
        border.width: 1
        implicitWidth: 170
        implicitHeight: 52

        Column {
            anchors.centerIn: parent
            spacing: 2

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: badgeLabel
                color: dashboard.textMuted
                font.pixelSize: 11
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: badgeValue
                color: dashboard.text
                font.pixelSize: 13
                font.bold: true
            }
        }
    }

    component ActionButton: Rectangle {
        required property string buttonLabel
        required property string buttonHint
        required property color accentColor
        signal triggered

        implicitWidth: 260
        implicitHeight: 84
        radius: 18
        color: hover.hovered ? Qt.lighter(accentColor, 1.18) : Qt.darker(accentColor, 2.7)
        border.color: accentColor
        border.width: 1

        Behavior on color {
            ColorAnimation { duration: 140 }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 6

            Text {
                text: buttonLabel
                color: dashboard.text
                font.pixelSize: 14
                font.bold: true
            }

            Text {
                text: buttonHint
                color: "#dce8ff"
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        HoverHandler { id: hover }
        TapHandler { onTapped: triggered() }
    }

    component MetricTile: Rectangle {
        required property var metricData

        radius: 22
        color: dashboard.surface
        border.color: dashboard.border
        border.width: 1
        implicitHeight: 156

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: metricData.title || "Metric"
                    color: dashboard.text
                    font.pixelSize: 16
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: metricData.accent || dashboard.accent
                }
            }

            Text {
                text: Number(metricData.value || 0).toFixed(metricData.unit === " Mbps" ? 2 : 1) + (metricData.unit || "")
                color: metricData.accent || dashboard.accent
                font.pixelSize: 34
                font.bold: true
            }

            Text {
                text: metricData.subtitle || ""
                color: dashboard.textMuted
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                height: 8
                radius: 4
                color: "#0d1728"

                Rectangle {
                    width: parent.width * Math.min(1, Number(metricData.value || 0) / 100.0)
                    height: parent.height
                    radius: parent.radius
                    color: metricData.accent || dashboard.accent

                    Behavior on width {
                        NumberAnimation { duration: 600; easing.type: Easing.OutCubic }
                    }
                }
            }
        }
    }

    component HealthDial: Item {
        required property real score

        width: 190
        height: 190

        property color dialColor: toneFor(score, 60, 82)

        Canvas {
            id: ringCanvas
            anchors.fill: parent
            antialiasing: true

            onPaint: {
                const ctx = getContext("2d")
                const w = width
                const h = height
                const radius = Math.min(w, h) / 2 - 10
                const centerX = w / 2
                const centerY = h / 2
                const start = -Math.PI / 2
                const end = start + Math.max(0, Math.min(1, score / 100.0)) * Math.PI * 2

                ctx.clearRect(0, 0, w, h)
                ctx.lineWidth = 14
                ctx.strokeStyle = "#16233a"
                ctx.beginPath()
                ctx.arc(centerX, centerY, radius, 0, Math.PI * 2)
                ctx.stroke()

                ctx.strokeStyle = dialColor
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.arc(centerX, centerY, radius, start, end)
                ctx.stroke()
            }
        }

        onScoreChanged: ringCanvas.requestPaint()
        onWidthChanged: ringCanvas.requestPaint()
        onHeightChanged: ringCanvas.requestPaint()
        Component.onCompleted: ringCanvas.requestPaint()

        Column {
            anchors.centerIn: parent
            spacing: 4

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.round(score) + ""
                color: dashboard.text
                font.pixelSize: 42
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "System health"
                color: dashboard.textMuted
                font.pixelSize: 12
            }
        }
    }
}
