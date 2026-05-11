import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as Platform
import "../style"
import "../components/feedback"
import "../components/controls"
import "../components/foundation"
import "../screens"

ApplicationWindow {
    id: root
    flags: Qt.Window | Qt.FramelessWindowHint
    color: Style.bg0

    property bool sidebarCollapsed: false
    property string currentScreen: "home"
    property bool allowQuit: false
    property int highCpuSeconds: 0
    property int lowMemorySeconds: 0
    property int lastAlertHealth: SystemCtrl.healthScore
    property double lastAlertTime: 0
    property string lastScheduleStamp: UiPrefs.lastScheduleRunStamp
    property bool showOnboarding: !UiPrefs.onboardingCompleted
    property bool showCrashBanner: StartupCrashDetected
    property bool showUpdateBanner: StartupUpdateAvailable

    Rectangle {
        anchors.fill: parent
        color: Style.bg0

        // Ambient background glowing orbs to give glassmorphism something to refract
        Rectangle {
            width: 800; height: 800
            radius: 400
            color: Style.violetGlow
            opacity: 0.3
            x: -200; y: -200
            
            SequentialAnimation on x {
                loops: Animation.Infinite
                NumberAnimation { to: 0; duration: 24000; easing.type: Animations.easeInOut }
                NumberAnimation { to: -200; duration: 24000; easing.type: Animations.easeInOut }
            }
        }
        
        Rectangle {
            width: 1000; height: 1000
            radius: 500
            color: Style.cyanGlow
            opacity: 0.2
            x: parent.width - 600
            y: parent.height - 400
            
            SequentialAnimation on y {
                loops: Animation.Infinite
                NumberAnimation { to: parent.height - 800; duration: 28000; easing.type: Animations.easeInOut }
                NumberAnimation { to: parent.height - 400; duration: 28000; easing.type: Animations.easeInOut }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Style.s16
        spacing: Style.s16

        Sidebar {
            id: sidebar
            Layout.fillHeight: true
            Layout.preferredWidth: sidebarCollapsed ? Style.sidebarCol : Style.sidebarW
            collapsed: sidebarCollapsed
            currentScreen: root.currentScreen
            onScreenSelected: root.currentScreen = screenId
            onCollapseToggled: sidebarCollapsed = !sidebarCollapsed
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelRadius: Style.r20
            fillColor: Style.glassPanel
            borderColor: Style.borderGlass
            contentMargin: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.s16
                spacing: Style.s12

                TopBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Style.topbarH
                    title: screenTitle(root.currentScreen)
                    onCloseRequested: { root.allowQuit = true; root.close() }
                    onMinRequested: root.showMinimized()
                    onMaxRequested: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 44 : 0
                    visible: root.showCrashBanner || root.showUpdateBanner || !FeatureGate.isPro
                    radius: Style.r12
                    color: root.showCrashBanner ? Style.redGlow : (root.showUpdateBanner ? Style.cyanGlow : Style.bg2)
                    border.color: root.showCrashBanner ? Style.red : (root.showUpdateBanner ? Style.cyan : Style.border1)
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Style.s16
                        anchors.rightMargin: Style.s16
                        spacing: Style.s12

                        Text {
                            Layout.fillWidth: true
                            text: root.showCrashBanner
                                  ? "PulseBoost recovered from a previous crash. Review diagnostics in Settings."
                                  : (root.showUpdateBanner
                                     ? ("Update " + StartupUpdateVersion + " is available.")
                                     : (FeatureGate.isPro ? "" : (FeatureGate.trialExpired ? "Trial expired. Settings stays available for activation." : ("Trial active | " + FeatureGate.trialDaysLeft + " days left."))))
                            color: Style.text0
                            font.family: Style.fontBody
                            font.pixelSize: Style.f13
                            font.weight: Style.w600
                        }

                        GlowButton {
                            visible: root.showUpdateBanner || !FeatureGate.isPro || root.showCrashBanner
                            label: root.showUpdateBanner ? "Settings" : (root.showCrashBanner ? "Open Settings" : "Upgrade")
                            glowColor: root.showCrashBanner ? Style.red : Style.cyan
                            variant: "outlined"
                            onClicked: root.currentScreen = "settings"
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ContentArea {
                        anchors.fill: parent
                        currentScreen: root.currentScreen
                    }
                }

                StatusBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Style.statusH
                    currentScreen: screenTitle(root.currentScreen)
                }
            }
        }
    }

    DragHandler {
        target: null
        acceptedButtons: Qt.LeftButton
        enabled: root.visibility !== Window.Maximized && root.visibility !== Window.FullScreen
        grabPermissions: PointerHandler.TakeOverForbidden
        onActiveChanged: {
            if (active && root.visibility !== Window.Maximized && root.visibility !== Window.FullScreen) {
                root.startSystemMove()
            }
        }
    }

    GlassOverlay {
        anchors.fill: parent
        z: 300
        visible: root.showOnboarding
        overlayColor: Qt.rgba(0, 0, 0, 0.82)
        contentMargin: Style.s48

        Onboarding {
            anchors.fill: parent
            onFinished: {
                UiPrefs.onboardingCompleted = true
                root.showOnboarding = false
                root.currentScreen = "home"
            }
        }
    }

    ToastContainer {
        id: toast
        anchors.fill: parent
    }

    Connections {
        target: SystemCtrl
        function onActionFeedback(message, success) {
            toast.push(success ? "Success" : "Warning", message, success ? "success" : "warning")
            if (UiPrefs.nativeTrayMessages) {
                tray.showMessage(success ? "PulseBoost" : "PulseBoost Warning", message, Platform.SystemTrayIcon.Information, 5000)
            }
        }

        function onMetricsChanged() {
            if (!UiPrefs.backgroundMonitoringEnabled) return
            root.highCpuSeconds = SystemCtrl.cpuUsage >= 90 ? root.highCpuSeconds + 1 : 0
            root.lowMemorySeconds = (100 - SystemCtrl.ramUsage) <= 10 ? root.lowMemorySeconds + 1 : 0
            root.checkSmartAlerts()
        }
    }

    Timer { interval: 1000; repeat: true; running: true; onTriggered: root.checkSmartAlerts() }
    Timer { interval: 60000; repeat: true; running: true; onTriggered: root.checkScheduledTasks() }
    Component.onCompleted: root.checkScheduledTasks()

    Platform.SystemTrayIcon {
        id: tray
        visible: true
        tooltip: "PulseBoost AI | Health " + SystemCtrl.healthScore
        icon.name: "system-run"
        onActivated: { root.show(); root.raise(); root.requestActivate() }
        menu: Platform.Menu {
            Platform.MenuItem { text: "Open PulseBoost"; onTriggered: { root.show(); root.raise(); root.requestActivate() } }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Quick Optimize"; onTriggered: SystemCtrl.applyOptimizationPreset("safe-recommended"); enabled: !FeatureGate.trialExpired }
            Platform.MenuItem { text: "Quit"; onTriggered: { root.allowQuit = true; Qt.quit() } }
        }
    }

    onClosing: function(close) {
        if (!root.allowQuit && UiPrefs.minimizeToTrayOnClose) {
            close.accepted = false
            root.hide()
        }
    }

    function canIssueAlert() {
        if (!UiPrefs.smartAlertsEnabled) return false
        return (Date.now() - root.lastAlertTime) >= UiPrefs.alertCooldownSeconds * 1000
    }

    function issueSmartAlert(title, message, tone) {
        if (!canIssueAlert()) return
        root.lastAlertTime = Date.now()
        root.lastAlertHealth = SystemCtrl.healthScore
        toast.push(title, message, tone)
    }

    function checkSmartAlerts() {
        if (!UiPrefs.smartAlertsEnabled || !UiPrefs.backgroundMonitoringEnabled) return
        if (root.highCpuSeconds >= 30) {
            issueSmartAlert("High CPU", "CPU stayed above 90% for 30 seconds.", "warning")
            root.highCpuSeconds = 0
        }
    }

    function scheduleStamp(now) {
        return Qt.formatDateTime(now, "yyyy-MM-dd")
    }

    function checkScheduledTasks() {}

    function screenTitle(screenId) {
        if (screenId === "home") return "Home"
        if (screenId === "optimizations") return "Optimizations"
        if (screenId === "boost") return "Boost-Up"
        if (screenId === "games") return "Games"
        if (screenId === "ai") return "AI"
        if (screenId === "backup") return "Backup"
        if (screenId === "settings") return "Settings"
        return "PulseBoost AI"
    }
}
