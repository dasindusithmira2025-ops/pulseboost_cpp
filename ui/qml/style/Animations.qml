pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property int instant: 60
    readonly property int fast: 90
    readonly property int normal: 130
    readonly property int slow: 180
    readonly property int xslow: 260
    readonly property int xxslow: 480

    readonly property int easeStandard: Easing.OutExpo
    readonly property int easeIn: Easing.InExpo
    readonly property int easeInOut: Easing.InOutExpo
    readonly property int easeSpring: Easing.OutBack
    readonly property int easeElastic: Easing.OutElastic
}
