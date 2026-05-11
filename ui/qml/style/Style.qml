pragma Singleton
import QtQuick 2.15

QtObject {
    property bool isDark: true

    readonly property color bg0: "#030407"
    readonly property color bg1: "#070A11"
    readonly property color bg2: "#0C111A"
    readonly property color bg3: "#131926"
    readonly property color bg4: "#1C2536"
    readonly property color bg5: "#28354A"

    readonly property color glassCard: "#660C111A"
    readonly property color glassPanel: "#88070A11"
    readonly property color glassHover: "#AA131926"
    readonly property color glassActive: "#CC1C2536"

    readonly property color border0: "#1A2536"
    readonly property color border1: "#25344A"
    readonly property color border2: "#3A4D6B"
    readonly property color borderGlass: "#33FFFFFF"
    readonly property color borderAccentSoft: "#441AC8FF"

    readonly property color text0: "#FFFFFF"
    readonly property color text1: "#E2E8F0"
    readonly property color text2: "#94A3B8"
    readonly property color text3: "#64748B"

    readonly property color violet: "#8A5AFF"
    readonly property color cyan: "#00E5FF"
    readonly property color green: "#00E676"
    readonly property color amber: "#FFC400"
    readonly property color red: "#FF1744"
    readonly property color magenta: "#F50057"

    readonly property color violetGlow: "#228A5AFF"
    readonly property color cyanGlow: "#2200E5FF"
    readonly property color greenGlow: "#2200E676"
    readonly property color amberGlow: "#22FFC400"
    readonly property color redGlow: "#22FF1744"

    readonly property color primary: cyan
    readonly property color success: green
    readonly property color warning: amber
    readonly property color danger: red
    readonly property color info: cyan

    readonly property color trafficClose: red
    readonly property color trafficMin: amber
    readonly property color trafficMax: green
    readonly property color trafficGlyph: text0

    readonly property color cCpu: cyan
    readonly property color cRam: violet
    readonly property color cDisk: green
    readonly property color cNet: amber
    readonly property color cTemp: red

    readonly property string fontDisplay: "Rajdhani"
    readonly property string fontBody: "IBM Plex Sans"
    readonly property string fontMono: "IBM Plex Mono"

    readonly property int w400: Font.Normal
    readonly property int w500: Font.Medium
    readonly property int w600: Font.DemiBold
    readonly property int w700: Font.Bold

    readonly property real f10: 10
    readonly property real f11: 11
    readonly property real f12: 12
    readonly property real f13: 13
    readonly property real f14: 14
    readonly property real f16: 16
    readonly property real f18: 18
    readonly property real f20: 20
    readonly property real f24: 24
    readonly property real f28: 28
    readonly property real f32: 32
    readonly property real f36: 36
    readonly property real f40: 40
    readonly property real f48: 48
    readonly property real f56: 56
    readonly property real f64: 64

    readonly property real s2: 2
    readonly property real s4: 4
    readonly property real s6: 6
    readonly property real s8: 8
    readonly property real s10: 10
    readonly property real s12: 12
    readonly property real s14: 14
    readonly property real s16: 16
    readonly property real s20: 20
    readonly property real s24: 24
    readonly property real s28: 28
    readonly property real s32: 32
    readonly property real s40: 40
    readonly property real s48: 48
    readonly property real s64: 64
    readonly property real s72: 72

    readonly property real r4: 4
    readonly property real r6: 6
    readonly property real r8: 8
    readonly property real r10: 10
    readonly property real r12: 12
    readonly property real r16: 16
    readonly property real r20: 20
    readonly property real r24: 24
    readonly property real r999: 999

    readonly property real sidebarW: 244
    readonly property real sidebarCol: 80
    readonly property real topbarH: 56
    readonly property real statusH: 28
    readonly property real cardPad: 18
    readonly property real pagePad: 24

    readonly property real instant: 70
    readonly property real fast: 100
    readonly property real normal: 150
    readonly property real slow: 220
    readonly property real xslow: 360
    readonly property real xxslow: 900

    readonly property real blurLow: 0
    readonly property real blurMid: 0
    readonly property real blurHigh: 0
    readonly property real hoverLift: 0
    readonly property real pressScale: 0.98

    function healthColor(s) {
        if (s >= 85) return green
        if (s >= 65) return cyan
        if (s >= 45) return amber
        return red
    }

    function healthLabel(s) {
        if (s >= 85) return "Excellent"
        if (s >= 65) return "Good"
        if (s >= 45) return "Fair"
        return "Critical"
    }

    function usageColor(p) {
        if (p < 60) return green
        if (p < 80) return amber
        return red
    }

    function riskColor(r) {
        if (r === 0 || r === 1) return green
        if (r === 2) return amber
        if (r === 3) return red
        return magenta
    }

    function riskLabel(r) {
        return ["Safe", "Low Risk", "Caution", "High Risk", "Critical"][Math.min(r, 4)]
    }

    function formatBytes(b) {
        if (b < 1024) return b + " B"
        if (b < 1048576) return (b / 1024).toFixed(1) + " KB"
        if (b < 1073741824) return (b / 1048576).toFixed(1) + " MB"
        return (b / 1073741824).toFixed(2) + " GB"
    }
}
