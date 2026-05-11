pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property FontLoader rajdhaniRegular: FontLoader { source: "qrc:/fonts/Rajdhani-Regular.ttf" }
    readonly property FontLoader rajdhaniMedium: FontLoader { source: "qrc:/fonts/Rajdhani-Medium.ttf" }
    readonly property FontLoader rajdhaniSemiBold: FontLoader { source: "qrc:/fonts/Rajdhani-SemiBold.ttf" }
    readonly property FontLoader rajdhaniBold: FontLoader { source: "qrc:/fonts/Rajdhani-Bold.ttf" }
    readonly property FontLoader plexSansRegular: FontLoader { source: "qrc:/fonts/IBMPlexSans-Regular.ttf" }
    readonly property FontLoader plexSansMedium: FontLoader { source: "qrc:/fonts/IBMPlexSans-Medium.ttf" }
    readonly property FontLoader plexSansSemiBold: FontLoader { source: "qrc:/fonts/IBMPlexSans-SemiBold.ttf" }
    readonly property FontLoader plexMonoRegular: FontLoader { source: "qrc:/fonts/IBMPlexMono-Regular.ttf" }
    readonly property FontLoader plexMonoMedium: FontLoader { source: "qrc:/fonts/IBMPlexMono-Medium.ttf" }
}
