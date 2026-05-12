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

    property var recent: SystemCtrl.recentActions

    function proofActions() {
        var out = []
        for (var i = 0; i < recent.length; i += 1) {
            var action = String(recent[i].action).toLowerCase()
            if (action.indexOf("clean") !== -1 || action.indexOf("ram") !== -1 || action.indexOf("net") !== -1 || action.indexOf("disk") !== -1 || action.indexOf("opt") !== -1) {
                out.push(recent[i])
            }
            if (out.length >= 6) break
        }
        return out
    }

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s24
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s8
                    Text { text: "Boost-Up"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "Run low-risk prep before gaming or heavy work. Advanced RAM and TCP changes stay in manual review through Action Center."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        spacing: Style.s10
                        GlowButton { label: "Safe Prep"; glowColor: Style.cyan; onClicked: { SystemCtrl.runClean(); SystemCtrl.flushDns() } }
                        GlowButton { label: "Restore Point"; variant: "outlined"; glowColor: Style.green; onClicked: SystemCtrl.createRestorePoint() }
                    }
                }
                GridLayout {
                    Layout.preferredWidth: 320
                    columns: 2
                    columnSpacing: Style.s12
                    rowSpacing: Style.s12
                    Repeater {
                        model: [
                            { label: "Free Memory", value: Number(SystemCtrl.memoryOverview.freeMb).toFixed(0) + " MB", tone: Style.cRam },
                            { label: "Memory Used", value: Number(SystemCtrl.memoryOverview.usedMb).toFixed(0) + " MB", tone: Style.cCpu },
                            { label: "Latency", value: Number(SystemCtrl.networkOverview.latency).toFixed(0) + " ms", tone: Style.cNet },
                            { label: "Saved Today", value: Number(SystemCtrl.savedTodayMb).toFixed(0) + " MB", tone: Style.cDisk }
                        ]
                        delegate: GlassPanel {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 64
                            fillColor: Style.bg1
                            borderColor: Style.border1
                            Column {
                                anchors.fill: parent
                                anchors.margins: Style.s10
                                spacing: 2
                                Text { text: modelData.label; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f10 }
                                Text { text: modelData.value; color: modelData.tone; font.family: Style.fontDisplay; font.pixelSize: Style.f20; font.weight: Style.w700 }
                            }
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 1100 ? 3 : 2
            columnSpacing: Style.s16
            rowSpacing: Style.s16

            Repeater {
                model: [
                    { title: "Clean Junk", text: "Quarantine safe temporary clutter before heavy sessions.", action: "clean", enabled: true },
                    { title: "RAM Manual Review", text: "Preview memory pressure; working-set trim is an advanced action.", action: "ram", enabled: false },
                    { title: "Refresh Network", text: "Flush DNS only. TCP tuning requires manual review.", action: "network", enabled: true },
                    { title: "Optimize Disk", text: "Trigger the system drive optimization path.", action: "disk" },
                    { title: "TCP Manual Review", text: "Use Action Center dry-run and confirmation before TCP changes.", action: "tcp", enabled: false },
                    { title: "Safety Backup", text: "Create a restore point before advanced changes.", action: "backup", enabled: true }
                ]
                delegate: GlassPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    fillColor: Style.bg2
                    borderColor: Style.border1
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.s14
                        spacing: Style.s8
                        Text { text: modelData.title; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f20; font.weight: Style.w700 }
                        Text { text: modelData.text; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Item { Layout.fillHeight: true }
                        GlowButton {
                            label: modelData.enabled ? "Run" : "Manual Review"
                            enabled: modelData.enabled
                            glowColor: modelData.enabled ? Style.cyan : Style.amber
                            onClicked: {
                                if (modelData.action === "clean") SystemCtrl.runClean()
                                else if (modelData.action === "network") SystemCtrl.flushDns()
                                else if (modelData.action === "disk") SystemCtrl.optimizeDisk()
                                else if (modelData.action === "backup") SystemCtrl.createRestorePoint()
                            }
                        }
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 260
            fillColor: Style.bg2
            borderColor: Style.border1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s10
                SectionHeader {
                    Layout.fillWidth: true
                    title: "Proof of Work"
                    subtitle: "Recent native actions that changed machine state."
                }
                Repeater {
                    model: root.proofActions()
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: Style.r10
                        color: Style.bg1
                        border.color: Style.border1
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.s10
                            anchors.rightMargin: Style.s10
                            spacing: Style.s12
                            Text { text: modelData.timeLabel || "--:--"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; Layout.preferredWidth: 60 }
                            Text { text: modelData.action; color: Style.text0; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600; Layout.preferredWidth: 150; elide: Text.ElideRight }
                            Text { text: modelData.details; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                    }
                }
                Text {
                    visible: root.proofActions().length === 0
                    text: "No cleanup or boost actions have completed yet. Run one of the actions above to populate this proof feed."
                    color: Style.text2
                    font.family: Style.fontBody
                    font.pixelSize: Style.f12
                }
            }
        }
    }
}
