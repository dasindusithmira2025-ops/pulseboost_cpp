import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import "../../style"
import "../charts"
import "../foundation"

GlassCard {
    id: root
    property string label: "CPU"
    property real value: 0
    property string unit: "%"
    property string detail: ""
    property color accentColor: Style.cyan
    property var sparklineData: []

    fillColor: hover.hovered ? Style.glassHover : Style.glassPanel
    borderColor: hover.hovered ? accentColor : Style.borderGlass
    interactive: true
    liftOnHover: true

    implicitHeight: 140
    implicitWidth: 280

    Behavior on borderColor { ColorAnimation { duration: Animations.fast } }

    NumberAnimation on value {
        duration: Animations.normal
        easing.type: Animations.easeStandard
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Style.s6

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: root.label.toUpperCase()
                color: Style.text1
                font.family: Style.fontMono
                font.pixelSize: Style.f12
                font.weight: Style.w600
                font.letterSpacing: 1
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: Style.s8
                height: Style.s8
                radius: Style.r999
                color: Style.usageColor(root.value)
                
                RectangularGlow {
                    anchors.fill: parent
                    glowRadius: 6
                    spread: 0.2
                    color: parent.color
                    opacity: 0.6
                }

                SequentialAnimation on opacity {
                    running: true
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: Animations.slow; easing.type: Animations.easeInOut }
                    NumberAnimation { to: 1.0; duration: Animations.slow; easing.type: Animations.easeInOut }
                }
            }
        }

        Row {
            spacing: Style.s4
            Layout.alignment: Qt.AlignLeft
            
            Text {
                text: Number(root.value).toFixed(1)
                color: Style.text0
                font.family: Style.fontDisplay
                font.pixelSize: Style.f40
                font.weight: Style.w700
            }
            Text {
                text: root.unit
                color: Style.text2
                font.family: Style.fontMono
                font.pixelSize: Style.f14
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 6
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.detail
            color: Style.text2
            font.family: Style.fontBody
            font.pixelSize: Style.f12
            elide: Text.ElideRight
        }

        Item { Layout.fillHeight: true }

        SparkLine {
            Layout.fillWidth: true
            Layout.preferredHeight: Style.s24
            lineColor: accentColor
            data: sparklineData
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: root.clicked() }
}
