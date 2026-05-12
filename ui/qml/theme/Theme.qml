pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color appBackground: "#0a0e1a"
    readonly property color sidebarBackground: "#0f1220"
    readonly property color cardBackground: "#13172b"
    readonly property color cardBackgroundElevated: "#1e2338"
    readonly property color borderSubtle: "#1AFFFFFF"
    readonly property color textPrimary: "#e8eaed"
    readonly property color textSecondary: "#b8bed0"
    readonly property color textMuted: "#8b92a8"
    readonly property color accentCyan: "#00d9ff"
    readonly property color accentBlue: "#3b82f6"
    readonly property color safeGreen: "#00ff88"
    readonly property color lowBlue: "#00d9ff"
    readonly property color mediumAmber: "#ffb800"
    readonly property color highRed: "#ff4757"
    readonly property color manualPurple: "#a78bfa"
    readonly property color destructiveRed: "#ff4757"
    readonly property color warningAmber: "#ffb800"
    readonly property color successGreen: "#00ff88"

    readonly property int sidebarWidth: 256
    readonly property int topBarHeight: 56
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 6
    readonly property int radiusLarge: 8
    readonly property int radiusXL: 12

    function riskColor(level) {
        const key = String(level || "").toLowerCase()
        if (key.indexOf("safe") !== -1) return safeGreen
        if (key.indexOf("low") !== -1) return lowBlue
        if (key.indexOf("medium") !== -1 || key.indexOf("moderate") !== -1) return mediumAmber
        if (key.indexOf("high") !== -1 || key.indexOf("critical") !== -1) return highRed
        if (key.indexOf("manual") !== -1 || key.indexOf("advanced") !== -1) return manualPurple
        return textMuted
    }

    function riskLabel(level) {
        const key = String(level || "").toLowerCase()
        if (key.indexOf("safe") !== -1) return "Safe"
        if (key.indexOf("low") !== -1) return "Low Risk"
        if (key.indexOf("medium") !== -1 || key.indexOf("moderate") !== -1) return "Medium Risk"
        if (key.indexOf("high") !== -1 || key.indexOf("critical") !== -1) return "High Risk"
        if (key.indexOf("manual") !== -1 || key.indexOf("advanced") !== -1) return "Manual Review"
        return "Review"
    }

    function riskTone(level) {
        const key = String(level || "").toLowerCase()
        if (key.indexOf("safe") !== -1 || key.indexOf("low") !== -1) return "success"
        if (key.indexOf("medium") !== -1 || key.indexOf("moderate") !== -1) return "warning"
        if (key.indexOf("high") !== -1 || key.indexOf("critical") !== -1) return "error"
        if (key.indexOf("manual") !== -1 || key.indexOf("advanced") !== -1) return "manual"
        return "neutral"
    }
}
