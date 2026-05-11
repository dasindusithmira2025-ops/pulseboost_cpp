import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.pagePad * 2

    property var tweaks: []
    property string searchQuery: ""
    property string activeCategory: "all"
    property var presets: SystemCtrl.optimizationPresets
    property var advisors: SystemCtrl.advisorItems

    function refreshTweaks() {
        tweaks = SystemCtrl.listTweaks()
    }

    function matchesCategory(item) {
        return activeCategory === "all" || String(item.category) === activeCategory
    }

    function matchesSearch(item) {
        if (!searchQuery || searchQuery.length === 0) return true
        var needle = searchQuery.toLowerCase()
        return String(item.name).toLowerCase().indexOf(needle) !== -1 || String(item.description).toLowerCase().indexOf(needle) !== -1 || String(item.detailedInfo).toLowerCase().indexOf(needle) !== -1
    }

    function visibleTweaks() {
        var out = []
        for (var i = 0; i < tweaks.length; i += 1) {
            if (matchesCategory(tweaks[i]) && matchesSearch(tweaks[i])) out.push(tweaks[i])
            if (out.length >= 72) break
        }
        return out
    }

    Component.onCompleted: refreshTweaks()

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 164
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s24
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8
                    Text { text: "Optimizations"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "Preset-first workflow for safe, reversible improvements with visible proof."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        spacing: Style.s10
                        StatusPill { text: Number(SystemCtrl.pulseScore.tweaksApplied || 0).toFixed(0) + " applied"; tone: "success" }
                        StatusPill { text: Number(SystemCtrl.pulseScore.tweaksAvailable || 0).toFixed(0) + " available"; tone: "warning" }
                        StatusPill { text: "Restore point surfaced"; tone: "neutral" }
                    }
                }
                ColumnLayout {
                    Layout.preferredWidth: 280
                    spacing: Style.s8
                    Text { text: "Advisor Lead"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                    Text { text: advisors.length > 0 ? advisors[0].title : "Safe recommendations are ready."; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f14; font.weight: Style.w600; wrapMode: Text.WordWrap }
                    Text { text: advisors.length > 0 ? advisors[0].description : "Start with the safe preset to establish a clean baseline."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1100 ? 3 : 2
            columnSpacing: Style.s16
            rowSpacing: Style.s16

            Repeater {
                model: root.presets
                delegate: GlassPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 124
                    fillColor: Style.glassPanel
                    borderColor: Style.border1
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.s14
                        spacing: Style.s8
                        Text { text: modelData.name; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f20; font.weight: Style.w700 }
                        Text { text: modelData.description; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Item { Layout.fillHeight: true }
                        GlowButton {
                            label: modelData.cta
                            glowColor: modelData.tone === "error" ? Style.red : (modelData.tone === "warning" ? Style.amber : Style.cyan)
                            onClicked: {
                                SystemCtrl.applyOptimizationPreset(modelData.id)
                                root.refreshTweaks()
                            }
                        }
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s16
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Style.r10
                    color: Style.bg1
                    border.color: Style.border1
                    border.width: 1
                    TextField {
                        id: searchField
                        anchors.fill: parent
                        anchors.margins: Style.s12
                        background: null
                        color: Style.text0
                        font.family: Style.fontBody
                        font.pixelSize: Style.f14
                        placeholderText: "Search tweaks, registry keys, services, or descriptions"
                        placeholderTextColor: Style.text3
                        onTextChanged: root.searchQuery = text
                    }
                }
                GlowButton { Layout.alignment: Qt.AlignVCenter; label: "Apply Safe"; glowColor: Style.cyan; onClicked: { SystemCtrl.applyOptimizationPreset("safe-recommended"); root.refreshTweaks() } }
                GlowButton { Layout.alignment: Qt.AlignVCenter; label: "Apply Advanced"; variant: "outlined"; glowColor: Style.amber; onClicked: { SystemCtrl.applyOptimizationPreset("advanced"); root.refreshTweaks() } }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s10
            Repeater {
                model: ["all", "windows", "gaming", "network", "ram", "gpu", "privacy", "power"]
                delegate: Rectangle {
                    required property string modelData
                    Layout.preferredWidth: 112
                    Layout.preferredHeight: 36
                    radius: Style.r12
                    color: root.activeCategory === modelData ? Style.glassHover : Style.glassCard
                    border.color: root.activeCategory === modelData ? Style.border2 : Style.border1
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: modelData === "all" ? "All" : modelData.charAt(0).toUpperCase() + modelData.slice(1)
                        color: root.activeCategory === modelData ? Style.text0 : Style.text1
                        font.family: Style.fontBody
                        font.pixelSize: Style.f12
                        font.weight: Style.w600
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.activeCategory = modelData
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s8
                SectionHeader {
                    Layout.fillWidth: true
                    title: "Recommended Right Now"
                    subtitle: "Advisor recommendations are backed by the same native analysis engine."
                }
                Repeater {
                    model: Math.min(3, advisors.length)
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: Style.r12
                        color: Style.glassCard
                        border.color: Style.border1
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s16
                            anchors.rightMargin: Style.s16
                            spacing: Style.s12
                            Text { text: advisors[index].title; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: advisors[index].impact; color: advisors[index].impact === "high" ? Style.red : (advisors[index].impact === "medium" ? Style.amber : Style.cyan); font.family: Style.fontMono; font.pixelSize: Style.f11 }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Style.s12
            Repeater {
                model: root.visibleTweaks()
                delegate: GlassPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 112
                    fillColor: Style.glassPanel
                    borderColor: Style.border1
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Style.s14
                        spacing: Style.s14
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Style.s6
                            RowLayout {
                                spacing: Style.s8
                                StatusPill { text: String(modelData.impact).toUpperCase(); tone: modelData.impact === "high" ? "error" : (modelData.impact === "medium" ? "warning" : "success") }
                                StatusPill { text: String(modelData.risk).toUpperCase(); tone: modelData.risk === "advanced" ? "error" : (modelData.risk === "moderate" ? "warning" : "success") }
                                StatusPill { visible: modelData.requiresRestart; text: "RESTART"; tone: "warning" }
                            }
                            Text { text: modelData.name; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f20; font.weight: Style.w700 }
                            Text { text: modelData.description; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Text { text: modelData.detailedInfo; color: Style.text3; font.family: Style.fontMono; font.pixelSize: Style.f10; elide: Text.ElideRight; Layout.fillWidth: true }
                        }
                        GlowButton {
                            Layout.alignment: Qt.AlignVCenter
                            label: modelData.isApplied ? "Revert" : "Apply"
                            glowColor: modelData.isApplied ? Style.red : Style.cyan
                            variant: modelData.isApplied ? "outlined" : "filled"
                            onClicked: {
                                if (modelData.isApplied) SystemCtrl.revertTweak(modelData.id)
                                else SystemCtrl.applyTweak(modelData.id)
                                root.refreshTweaks()
                            }
                        }
                    }
                }
            }
        }
    }
}
