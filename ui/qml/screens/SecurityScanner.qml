import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/cards"
import "../components/controls"
import "../components/feedback"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.s32

    property int securityScore: Math.max(0, Math.min(100, SystemCtrl.healthScore - 8))
    property bool scanning: false
    property int scanProgress: 0
    property var scanHistory: []
    property var findings: [
        { title: "Remote Desktop Protocol", severity: "Caution", detail: "Remote Desktop is exposed locally. Keep it disabled unless you actively use it.", lowRisk: false, expanded: false },
        { title: "Unnecessary Startup Vectors", severity: "Low Risk", detail: "Several startup entries can be delayed to reduce attack surface and boot clutter.", lowRisk: true, expanded: false },
        { title: "Pending System Updates", severity: "Caution", detail: "Security updates are pending in Windows Update. Install them outside active sessions.", lowRisk: false, expanded: false }
    ]

    function severityColor(severity) {
        if (severity === "Critical") return Style.danger
        if (severity === "Caution") return Style.warning
        if (severity === "Resolved") return Style.cyan
        return Style.success
    }
    
    function severityGlow(severity) {
        if (severity === "Critical") return Style.redGlow
        if (severity === "Caution") return Style.amberGlow
        if (severity === "Resolved") return Style.cyanGlow
        return Style.greenGlow
    }

    function toggleFinding(idx) {
        const next = []
        for (let i = 0; i < findings.length; i += 1) {
            const item = findings[i]
            next.push({ title: item.title, severity: item.severity, detail: item.detail, lowRisk: item.lowRisk, expanded: i === idx ? !item.expanded : item.expanded })
        }
        findings = next
    }

    function autoFixLowRisk() {
        SystemCtrl.createRestorePoint()
        const next = []
        for (let i = 0; i < findings.length; i += 1) {
            const item = findings[i]
            if (item.lowRisk) {
                next.push({ title: item.title, severity: "Resolved", detail: item.detail, lowRisk: false, expanded: false })
            } else {
                next.push(item)
            }
        }
        findings = next
    }

    Timer {
        interval: 120
        running: root.scanning
        repeat: true
        onTriggered: {
            root.scanProgress += 4
            if (root.scanProgress >= 100) {
                root.scanProgress = 100
                root.scanning = false
                const nextHistory = root.scanHistory.slice(0)
                nextHistory.unshift({ when: Qt.formatDateTime(new Date(), "yyyy-MM-dd HH:mm"), score: root.securityScore })
                root.scanHistory = nextHistory.slice(0, 5)
            }
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            contentMargin: Style.s16
            
            RowLayout {
                anchors.fill: parent
                spacing: Style.s16
                
                Text {
                    text: "Security Surface Scanner"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }
                Item { Layout.fillWidth: true }
                GlowButton {
                    Layout.alignment: Qt.AlignVCenter
                    label: "Resolve Low Risk"
                    variant: "outlined"
                    glowColor: Style.cyan
                    onClicked: root.autoFixLowRisk()
                }
                GlowButton {
                    Layout.alignment: Qt.AlignVCenter
                    label: root.scanning ? "Scanning..." : "Execute Surface Scan"
                    glowColor: Style.amber
                    onClicked: {
                        root.scanProgress = 0
                        root.scanning = true
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1180 ? 3 : 1
            columnSpacing: Style.s20
            rowSpacing: Style.s20

            GlassPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.s16
                    spacing: Style.s16

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        HealthRingLarge {
                            anchors.centerIn: parent
                            score: root.scanning ? root.scanProgress : root.securityScore
                            cpuScore: root.securityScore
                            ramScore: Math.max(0, root.securityScore - 5)
                            diskScore: Math.max(0, root.securityScore - 10)
                            trendDelta: root.scanning ? 0 : 2
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: Style.s16
                            text: root.scanning ? ("Scan indexing " + root.scanProgress + "%") : ("Security Integrity: " + root.securityScore)
                            color: root.scanning ? Style.warning : Style.healthColor(root.securityScore)
                            font.family: Style.fontMono
                            font.pixelSize: Style.f13
                            font.weight: Style.w600
                        }
                    }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: width > 1180 ? 2 : 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s16

                    Text {
                        text: "Threat & Baseline Findings"
                        color: Style.text0
                        font.family: Style.fontDisplay
                        font.pixelSize: Style.f20
                        font.weight: Style.w700
                    }

                    Repeater {
                        model: root.findings
                        delegate: GlassCard {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: modelData.expanded ? 140 : 56
                            fillColor: Style.bg1
                            borderColor: root.severityColor(modelData.severity)

                            Column {
                                anchors.fill: parent
                                anchors.margins: Style.s12
                                spacing: Style.s10

                                RowLayout {
                                    width: parent.width
                                    spacing: Style.s16
                                    
                                    Rectangle {
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: root.severityColor(modelData.severity)
                                        layer.enabled: true
                                        layer.effect: DropShadow { color: root.severityGlow(modelData.severity); radius: 8; spread: 0.1 }
                                    }
                                    
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.title
                                        color: Style.text0
                                        font.family: Style.fontBody
                                        font.pixelSize: Style.f14
                                        font.weight: Style.w600
                                    }
                                    
                                    Rectangle {
                                        Layout.preferredWidth: 84
                                        Layout.preferredHeight: 24
                                        radius: Style.r999
                                        color: root.severityGlow(modelData.severity)
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.severity
                                            color: root.severityColor(modelData.severity)
                                            font.family: Style.fontMono
                                            font.pixelSize: Style.f10
                                            font.weight: Style.w700
                                        }
                                    }
                                }

                                Text {
                                    visible: modelData.expanded
                                    text: modelData.detail
                                    color: Style.text2
                                    font.family: Style.fontBody
                                    font.pixelSize: Style.f13
                                    wrapMode: Text.WordWrap
                                    width: parent.width
                                }

                                GlowButton {
                                    visible: modelData.expanded
                                    label: modelData.lowRisk ? "Apply Auto-Fix" : "Open System Tool"
                                    variant: "outlined"
                                    glowColor: modelData.lowRisk ? Style.cyan : Style.amber
                                    onClicked: if (modelData.lowRisk) root.autoFixLowRisk()
                                    Layout.topMargin: Style.s4
                                    Layout.alignment: Qt.AlignLeft
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.toggleFinding(index)
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s12

                Text {
                    text: "Historical Integrity Logs"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f18
                    font.weight: Style.w700
                }

                Repeater {
                    model: root.scanHistory
                    delegate: GlassCard {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        fillColor: Style.bg2
                        contentMargin: 0
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s16
                            anchors.rightMargin: Style.s16
                            Text {
                                text: Icons.glyph("security")
                                color: Style.text3
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f16
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "Scan Execution: " + modelData.when
                                color: Style.text2
                                font.family: Style.fontMono
                                font.pixelSize: Style.f13
                            }
                            Text {
                                text: modelData.score + " / 100"
                                color: Style.healthColor(modelData.score)
                                font.family: Style.fontMono
                                font.pixelSize: Style.f13
                                font.weight: Style.w700
                            }
                        }
                    }
                }

                EmptyState {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.scanHistory.length === 0
                    title: "No completed traces"
                    subtitle: "Execute a surface scan to generate baseline telemetry."
                }
            }
        }
    }
}
