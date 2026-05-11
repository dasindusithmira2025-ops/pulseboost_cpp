import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/charts"
import "../components/controls"
import "../components/foundation"
import "../components/feedback"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.s32

    property var series: SystemCtrl.chartSeries
    property string range: "week"
    property var actions: SystemCtrl.recentActions
    property var snapshots: SystemCtrl.systemSnapshots

    function allHealthSeries() {
        if (!series || series.length === 0) return []
        return series.map(function(point) { return Number(point.health) })
    }

    function visibleHealthSeries() {
        const values = allHealthSeries()
        if (range === "today") return values.slice(Math.max(0, values.length - 12))
        if (range === "week") return values.slice(Math.max(0, values.length - 24))
        if (range === "month") return values.slice(Math.max(0, values.length - 45))
        return values
    }

    function avg(list) {
        if (!list || list.length === 0) return 0
        let total = 0
        for (let i = 0; i < list.length; i += 1) total += list[i]
        return total / list.length
    }

    function best(list) {
        if (!list || list.length === 0) return 0
        return Math.max.apply(Math, list)
    }

    function worst(list) {
        if (!list || list.length === 0) return 0
        return Math.min.apply(Math, list)
    }

    function reportText() {
        const values = visibleHealthSeries()
        const delta = values.length > 1 ? (values[values.length - 1] - values[0]) : 0
        return "Executive Summary\n\n"
            + "Average health: " + Number(avg(values)).toFixed(1) + "/100\n"
            + "Best score: " + Number(best(values)).toFixed(0) + "\n"
            + "Worst score: " + Number(worst(values)).toFixed(0) + "\n"
            + "Trend: " + (delta >= 0 ? "+" : "") + Number(delta).toFixed(1) + " points\n\n"
            + "Top Issues\n\n"
            + (actions.length > 0 ? String(actions[0].details) : "No optimization events recorded yet.") + "\n\n"
            + "Recommendations\n\n"
            + "Keep startup load under control, run cleanup before disk pressure crosses 90%, and act on CPU or RAM alerts early in the session."
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
            Layout.preferredHeight: Style.s72
            contentMargin: Style.s16

            RowLayout {
                anchors.fill: parent
                spacing: Style.s16
                
                Text {
                    text: "Health History Machine"
                    color: Style.text0
                    font.family: Style.fontDisplay
                    font.pixelSize: Style.f20
                    font.weight: Style.w700
                }
                
                Item { Layout.fillWidth: true }
                
                Repeater {
                    model: [
                        { id: "today", label: "Today" },
                        { id: "week", label: "7 Days" },
                        { id: "month", label: "30 Days" },
                        { id: "quarter", label: "90 Days" }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 32
                        radius: Style.r999
                        color: root.range === modelData.id ? Style.violetGlow : Style.bg3
                        border.color: root.range === modelData.id ? Style.violet : Style.border1
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: root.range === modelData.id ? Style.violet : Style.text1
                            font.family: Style.fontMono
                            font.pixelSize: Style.f11
                            font.weight: Style.w600
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.range = modelData.id
                        }
                    }
                }
                
                Item { Layout.preferredWidth: Style.s8 }
                
                GlowButton {
                    label: "Generate AI Report"
                    glowColor: Style.violet
                    variant: "solid"
                    onClicked: reportDialog.open()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s16
            
            SummaryCard { title: "Active Average"; value: Number(root.avg(root.visibleHealthSeries())).toFixed(1); accent: Style.cyan }
            SummaryCard { title: "Peak Condition"; value: Number(root.best(root.visibleHealthSeries())).toFixed(0); accent: Style.green }
            SummaryCard { title: "Lowest Dip"; value: Number(root.worst(root.visibleHealthSeries())).toFixed(0); accent: Style.red }
            SummaryCard { title: "Recorded Events"; value: String(root.actions.length); accent: Style.amber }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 280

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s12

                RowLayout {
                    Layout.fillWidth: true
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text {
                            text: "System Rollback Snapshots"
                            color: Style.text0
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f20
                            font.weight: Style.w700
                        }
                        Text {
                            text: "Snapshots capture startup/process baseline and current health. Safely rewind registry drift."
                            color: Style.text2
                            font.family: Style.fontBody
                            font.pixelSize: Style.f13
                            wrapMode: Text.WordWrap
                        }
                    }
                    
                    GlowButton {
                        label: "Take Snapshot"
                        variant: "outlined"
                        glowColor: Style.cyan
                        onClicked: SystemCtrl.takeSystemSnapshot()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: Style.s8
                    model: root.snapshots

                    delegate: GlassCard {
                        required property var modelData
                        width: ListView.view.width
                        height: 56
                        fillColor: Style.bg1
                        contentMargin: 0

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s16
                            anchors.rightMargin: Style.s16
                            spacing: Style.s12

                            Text {
                                text: Icons.glyph("clock")
                                color: Style.text2
                                font.family: Style.fontDisplay
                                font.pixelSize: Style.f16
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.id
                                color: Style.text0
                                font.family: Style.fontMono
                                font.pixelSize: Style.f13
                                font.weight: Style.w600
                                elide: Text.ElideRight
                            }
                            
                            Rectangle {
                                Layout.preferredWidth: 2
                                Layout.preferredHeight: 20
                                color: Style.border1
                            }
                            
                            Text {
                                text: "Integrity Score: "
                                color: Style.text2
                                font.family: Style.fontMono
                                font.pixelSize: Style.f11
                            }
                            
                            Text {
                                text: Number(modelData.healthScore).toFixed(0)
                                color: Style.healthColor(modelData.healthScore)
                                font.family: Style.fontMono
                                font.pixelSize: Style.f13
                                font.weight: Style.w700
                                Layout.preferredWidth: 40
                            }

                            GlowButton {
                                Layout.preferredWidth: 120
                                label: "Restore Point"
                                variant: "outlined"
                                glowColor: Style.amber
                                onClicked: SystemCtrl.restoreSystemSnapshot(modelData.id)
                            }
                        }
                    }
                }

                EmptyState {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.snapshots.length === 0
                    title: "No registry images found"
                    subtitle: "Create a snapshot to benchmark your current speeds."
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 340

            Item {
                anchors.fill: parent
                anchors.margins: Style.cardPad

                LineChart {
                    id: healthChart
                    anchors.fill: parent
                    data: root.visibleHealthSeries()
                    lineColor: Style.healthColor(SystemCtrl.healthScore)
                    revealProgress: 1
                    maxValue: 100
                    minValue: 0
                }

                Repeater {
                    model: Math.min(4, root.actions.length)
                    delegate: Rectangle {
                        width: 2
                        height: healthChart.height
                        color: Style.amberGlow
                        x: healthChart.width * ((index + 1) / (Math.min(4, root.actions.length) + 1))
                        y: 0
                        
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.top
                            anchors.bottomMargin: Style.s4
                            width: tagText.implicitWidth + 12
                            height: 20
                            radius: 4
                            color: Style.amberGlow
                            
                            Text {
                                id: tagText
                                anchors.centerIn: parent
                                text: root.actions[index].type
                                color: Style.amber
                                font.family: Style.fontMono
                                font.pixelSize: Style.f10
                                font.weight: Style.w700
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: reportDialog
        width: 680
        height: 560
        modal: true
        anchors.centerIn: Overlay.overlay
        background: GlassPanel {
            anchors.fill: parent
            blurStrength: 40
            borderColor: Style.violet
        }

        contentItem: ColumnLayout {
            spacing: Style.s20
            
            Text {
                text: "PulseBoost Intelligence Report"
                color: Style.text0
                font.family: Style.fontDisplay
                font.pixelSize: Style.f28
                font.weight: Style.w700
            }
            
            Rectangle { Layout.fillWidth: true; height: 1; color: Style.borderGlass }

            Text {
                text: root.reportText()
                color: Style.text1
                font.family: Style.fontMono
                font.pixelSize: Style.f13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.fillHeight: true
                lineHeight: 1.4
            }
            
            GlowButton {
                Layout.alignment: Qt.AlignRight
                label: "Dismiss Report"
                variant: "outlined"
                glowColor: Style.text2
                onClicked: reportDialog.close()
            }
        }
    }

    component SummaryCard: GlassCard {
        property string title: ""
        property string value: ""
        property color accent: Style.violet
        Layout.fillWidth: true
        Layout.preferredHeight: 100
        borderColor: Style.borderGlass

        Column {
            anchors.fill: parent
            anchors.margins: Style.s16
            spacing: Style.s8
            Text {
                text: title.toUpperCase()
                color: Style.text2
                font.family: Style.fontMono
                font.pixelSize: Style.f11
                font.letterSpacing: 1
            }
            Text {
                text: value
                color: accent
                font.family: Style.fontDisplay
                font.pixelSize: Style.f32
                font.weight: Style.w700
                
                layer.enabled: true
                layer.effect: DropShadow { color: accent; radius: 10; spread: 0.1; samples: 21; opacity: 0.5 }
            }
        }
    }
}
