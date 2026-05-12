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

    property int currentTab: 0
    property string pendingMode: SystemCtrl.aiMode

    ColumnLayout {
        id: contentColumn
        width: root.width - Style.pagePad * 2
        x: Style.pagePad
        y: Style.pagePad
        spacing: Style.s20

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 128
            fillColor: Style.bg2
            borderColor: Style.border1
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.cardPad
                spacing: Style.s20
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.s6
                    Text { text: "Settings"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f32; font.weight: Style.w700 }
                    Text { text: "Safety gates, audit logging, restore behavior, and local-first privacy remain visible in the native Qt/QML shell."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                Column {
                    spacing: Style.s4
                    StatusPill { text: FeatureGate.tierLabel.toUpperCase(); tone: FeatureGate.isPro ? "success" : "warning" }
                    Text { text: SystemCtrl.aiMode === "cloud" ? "Cloud AI configured" : "Local AI mode"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f12 }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.s8
            Repeater {
                model: ["General", "AI", "License", "Schedule", "Updates", "Advanced", "About"]
                delegate: Rectangle {
                    required property string modelData
                    required property int index
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 36
                    radius: Style.r10
                    color: root.currentTab === index ? Style.bg3 : Style.bg2
                    border.color: root.currentTab === index ? Style.border2 : Style.border1
                    border.width: 1
                    Text { anchors.centerIn: parent; text: modelData; color: root.currentTab === index ? Style.text0 : Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f12; font.weight: Style.w600 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.currentTab = index }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentTab

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 420
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s14
                    Text { text: "General"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f24; font.weight: Style.w700 }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Smart alerts"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.smartAlertsEnabled; onToggled: UiPrefs.smartAlertsEnabled = checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Tray notifications"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.nativeTrayMessages; onToggled: UiPrefs.nativeTrayMessages = checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Background monitoring"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.backgroundMonitoringEnabled; onToggled: UiPrefs.backgroundMonitoringEnabled = checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Audit logging"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        StatusPill { text: "Always on"; tone: "success" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Safety gates"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        StatusPill { text: "Medium/high risk requires confirmation"; tone: "warning" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Minimize to tray on close"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.minimizeToTrayOnClose; onToggled: UiPrefs.minimizeToTrayOnClose = checked }
                    }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 340
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s14
                    Text { text: "AI Mode"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f24; font.weight: Style.w700 }
                    RowLayout {
                        spacing: Style.s10
                        GlowButton { label: "Local"; glowColor: Style.cyan; variant: root.pendingMode === "local" ? "filled" : "outlined"; onClicked: root.pendingMode = "local" }
                        GlowButton { label: "Cloud"; glowColor: Style.amber; variant: root.pendingMode === "cloud" ? "filled" : "outlined"; onClicked: root.pendingMode = "cloud" }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        radius: Style.r10
                        color: Style.bg1
                        border.color: Style.border1
                        border.width: 1
                        TextField {
                            id: apiKeyField
                            anchors.fill: parent
                            anchors.margins: Style.s10
                            background: null
                            color: Style.text0
                            font.family: Style.fontMono
                            font.pixelSize: Style.f12
                            placeholderText: "Optional cloud API key"
                            placeholderTextColor: Style.text3
                            echoMode: TextInput.Password
                        }
                    }
                    Text { text: (SystemCtrl.aiCloudConfigured ? "A cloud key is already stored. " : "No cloud key stored. ") + "AI remains advisory-first and cannot silently execute medium/high-risk actions."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    GlowButton { label: "Save AI Preferences"; glowColor: Style.cyan; onClicked: SystemCtrl.setAiPreferences(root.pendingMode, apiKeyField.text) }
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
                    spacing: Style.s14
                    Text { text: "License"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f24; font.weight: Style.w700 }
                    Text { text: FeatureGate.isPro ? "PulseBoost Pro is active." : (FeatureGate.trialExpired ? "Trial expired. Activate a license to unlock premium actions." : ("Trial active with " + FeatureGate.trialDaysLeft + " days remaining.")); color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        radius: Style.r10
                        color: Style.bg1
                        border.color: Style.border1
                        border.width: 1
                        TextField {
                            id: licenseField
                            anchors.fill: parent
                            anchors.margins: Style.s10
                            background: null
                            color: Style.text0
                            font.family: Style.fontMono
                            font.pixelSize: Style.f12
                            placeholderText: "Enter license key"
                            placeholderTextColor: Style.text3
                        }
                    }
                    GlowButton { label: "Activate"; glowColor: FeatureGate.isPro ? Style.green : Style.cyan; onClicked: FeatureGate.activatePro(licenseField.text) }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 340
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s14
                    Text { text: "Schedule"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f24; font.weight: Style.w700 }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Enable recurring maintenance"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.scheduleEnabled; onToggled: UiPrefs.scheduleEnabled = checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Preview cleanup categories"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.scheduleClean; onToggled: UiPrefs.scheduleClean = checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Prepare memory dry-run"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.scheduleRam; onToggled: UiPrefs.scheduleRam = checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Network diagnostics"; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; Layout.fillWidth: true }
                        Switch { checked: UiPrefs.scheduleDns; onToggled: UiPrefs.scheduleDns = checked }
                    }
                    Text { text: "Scheduled task records: " + SystemCtrl.getScheduledOptimizations().length; color: Style.text2; font.family: Style.fontMono; font.pixelSize: Style.f11 }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 280
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s14
                    Text { text: "Updates and Diagnostics"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f24; font.weight: Style.w700 }
                    Text { text: StartupUpdateAvailable ? ("Update available: " + StartupUpdateVersion) : "You are running the current packaged build."; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13 }
                    Text { text: StartupCrashDetected ? ("Previous crash report: " + StartupCrashReportPath) : "No pending crash report detected."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap }
                    Text { text: StartupUpdateAvailable ? StartupUpdateUrl : "No download URL required."; color: Style.cyan; font.family: Style.fontMono; font.pixelSize: Style.f11; wrapMode: Text.WordWrap }
                }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                fillColor: Style.bg2
                borderColor: Style.border1
                contentMargin: Style.cardPad
                Tools { anchors.fill: parent }
            }

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 240
                fillColor: Style.bg2
                borderColor: Style.border1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Style.cardPad
                    spacing: Style.s12
                    Text { text: "About"; color: Style.text0; font.family: Style.fontDisplay; font.pixelSize: Style.f24; font.weight: Style.w700 }
                    Text { text: "PulseBoost AI now ships through the native Qt/QML shell backed directly by the C++ optimization core."; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap }
                    Text { text: "Privacy: telemetry is local-first. No silent changes are made by AI; system-changing actions keep dry-run, confirmation, rollback, and audit visibility."; color: Style.text1; font.family: Style.fontBody; font.pixelSize: Style.f13; wrapMode: Text.WordWrap }
                    Text { text: "Deprecated UI experiments are excluded from production release builds."; color: Style.text2; font.family: Style.fontBody; font.pixelSize: Style.f12; wrapMode: Text.WordWrap }
                }
            }
        }
    }
}
