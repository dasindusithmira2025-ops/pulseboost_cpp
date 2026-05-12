import QtQuick 2.15
import QtQuick.Controls 2.15
import "../style"
import "../screens"
import "../components/feedback"
import "../components/foundation"

GlassPanel {
    id: root
    fillColor: "transparent"
    borderColor: "transparent"
    panelRadius: Style.r12
    contentMargin: 0

    property string currentScreen: "home"
    property string screenLoadError: ""

    Loader {
        id: screenLoader
        anchors.fill: parent
        asynchronous: true
        sourceComponent: root.screenComponent(root.currentScreen)

        onStatusChanged: {
            if (status === Loader.Error) {
                root.screenLoadError = "Failed to load screen: " + root.currentScreen
            } else if (status === Loader.Ready) {
                root.screenLoadError = ""
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: !SystemCtrl.uiDataReady && root.screenLoadError === ""
        title: "Preparing PulseBoost"
        subtitle: "Loading live telemetry and native optimization services."
    }

    ErrorState {
        anchors.fill: parent
        visible: root.screenLoadError !== "" || SystemCtrl.uiErrorMessage.length > 0
        title: "Screen Error"
        subtitle: root.screenLoadError !== "" ? root.screenLoadError : SystemCtrl.uiErrorMessage
    }

    function screenComponent(screenId) {
        if (screenId === "home") return homeScreen
        if (screenId === "action-center") return actionCenterScreen
        if (screenId === "ai-advisor") return aiScreen
        if (screenId === "before-after") return proofScreen
        if (screenId === "audit-log") return auditLogScreen
        if (screenId === "restore-center") return restoreCenterScreen
        if (screenId === "processes") return processesScreen
        if (screenId === "startup-apps") return startupScreen
        if (screenId === "storage-cleanup") return storageScreen
        if (screenId === "network-tools") return networkScreen
        if (screenId === "settings") return settingsScreen
        return homeScreen
    }

    Component { id: homeScreen; Home {} }
    Component { id: actionCenterScreen; ActionCenter {} }
    Component { id: proofScreen; HealthHistory {} }
    Component { id: auditLogScreen; AuditLog {} }
    Component { id: restoreCenterScreen; RestoreCenter {} }
    Component { id: aiScreen; AiChat {} }
    Component { id: processesScreen; ProcessManager {} }
    Component { id: startupScreen; StartupManager {} }
    Component { id: storageScreen; StorageAnalyzer {} }
    Component { id: networkScreen; NetworkMonitor {} }
    Component { id: settingsScreen; Settings {} }
}
