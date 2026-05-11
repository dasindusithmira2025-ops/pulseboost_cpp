import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/feedback"
import "../components/foundation"

Flickable {
    id: root
    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + Style.s32

    property var summary: SystemCtrl.driverSummary
    property var drivers: SystemCtrl.driverList

    function toneColor(tone) {
        if (tone === "error") return Style.red
        if (tone === "warning") return Style.amber
        return Style.green
    }
    
    function toneGlow(tone) {
        if (tone === "error") return Style.redGlow
        if (tone === "warning") return Style.amberGlow
        return Style.greenGlow
    }

    ColumnLayout {
        id: contentColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Style.pagePad
        spacing: Style.s20

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s20

            Repeater {
                model: [
                    { "title": "Total Devices", "value": Number(summary.total || 0), "accent": Style.violet },
                    { "title": "Requires Attention", "value": Number(summary.outdated || 0), "accent": Style.amber },
                    { "title": "Unsigned Packages", "value": Number(summary.unsigned || 0), "accent": Style.red },
                    { "title": "Optimal Validation", "value": Number(summary.current || 0), "accent": Style.green }
                ]

                delegate: GlassCard {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    borderColor: Style.borderGlass

                    Column {
                        anchors.fill: parent
                        anchors.margins: Style.s16
                        spacing: Style.s12

                        Text {
                            text: modelData.title.toUpperCase()
                            color: Style.text2
                            font.family: Style.fontMono
                            font.pixelSize: Style.f11
                            font.letterSpacing: 1
                        }
                        Text {
                            text: Number(modelData.value).toFixed(0)
                            color: modelData.accent
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f36
                            font.weight: Style.w700
                            
                            layer.enabled: true
                            layer.effect: DropShadow { color: modelData.accent; radius: 10; spread: 0.1; samples: 21; opacity: 0.5 }
                        }
                    }
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 800

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.s16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text {
                            text: "Driver & Hardware Intelligence"
                            color: Style.text0
                            font.family: Style.fontDisplay
                            font.pixelSize: Style.f24
                            font.weight: Style.w700
                        }
                        Text {
                            text: "Querying installed kernel modules and comparing software signatures. Outdated devices can drastically reduce hardware performance."
                            color: Style.text2
                            font.family: Style.fontBody
                            font.pixelSize: Style.f13
                            wrapMode: Text.WordWrap
                        }
                    }
                    
                    Item { Layout.preferredWidth: Style.s16 }

                    GlowButton {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 48
                        Layout.alignment: Qt.AlignTop
                        label: "Request Fresh Audit"
                        glowColor: Style.violet
                        onClicked: SystemCtrl.createRestorePoint()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    color: Style.bg2
                    radius: Style.r8
                    border.color: Style.border1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Style.s16
                        anchors.rightMargin: Style.s16
                        spacing: Style.s12

                        Text { text: "DEVICE NAMESPACE"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; font.weight: Style.w600; Layout.fillWidth: true }
                        Text { text: "VENDOR"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; font.weight: Style.w600; Layout.preferredWidth: 160 }
                        Text { text: "BUILD VER."; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; font.weight: Style.w600; Layout.preferredWidth: 160 }
                        Text { text: "INTEGRITY"; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11; font.weight: Style.w600; Layout.preferredWidth: 100 }
                        Item { Layout.preferredWidth: 160 }
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: Style.s8
                    model: root.drivers

                    delegate: GlassCard {
                        required property var modelData
                        width: ListView.view.width
                        height: contentRow.implicitHeight + Style.s24
                        fillColor: Style.bg1
                        borderColor: root.toneColor(modelData.statusTone)
                        contentMargin: 0

                        RowLayout {
                            id: contentRow
                            anchors.fill: parent
                            anchors.leftMargin: Style.s16
                            anchors.rightMargin: Style.s16
                            spacing: Style.s12

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: Style.s4
                                Text {
                                    text: modelData.deviceName
                                    color: Style.text0
                                    font.family: Style.fontBody
                                    font.pixelSize: Style.f14
                                    font.weight: Style.w600
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: "TIMESTAMP: " + modelData.driverDate
                                    color: Style.text3
                                    font.family: Style.fontMono
                                    font.pixelSize: Style.f10
                                }
                            }

                            Text {
                                text: modelData.provider
                                color: Style.text1
                                font.family: Style.fontBody
                                font.pixelSize: Style.f13
                                Layout.preferredWidth: 160
                                Layout.alignment: Qt.AlignVCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                text: modelData.driverVersion
                                color: Style.text1
                                font.family: Style.fontMono
                                font.pixelSize: Style.f12
                                font.weight: Style.w600
                                Layout.preferredWidth: 160
                                Layout.alignment: Qt.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                Layout.preferredWidth: 100
                                Layout.preferredHeight: 28
                                Layout.alignment: Qt.AlignVCenter
                                radius: Style.r999
                                color: root.toneGlow(modelData.statusTone)

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.statusLabel
                                    color: root.toneColor(modelData.statusTone)
                                    font.family: Style.fontMono
                                    font.pixelSize: Style.f10
                                    font.weight: Style.w700
                                }
                            }

                            GlowButton {
                                Layout.preferredWidth: 160
                                Layout.alignment: Qt.AlignVCenter
                                label: "Cross-Reference"
                                glowColor: root.toneColor(modelData.statusTone)
                                variant: "outlined"
                                onClicked: Qt.openUrlExternally(modelData.lookupUrl)
                            }
                        }
                    }
                }

                EmptyState {
                    visible: root.drivers.length === 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "Hardware list unavailable"
                    subtitle: "PulseBoost requires Administrative clearance to map System Driver telemetry models."
                }
            }
        }
    }
}
