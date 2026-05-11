import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../style"
import "../components/controls"
import "../components/foundation"

Rectangle {
    id: root
    color: Qt.rgba(0, 0, 0, 0.8)
    radius: Style.r16

    property int step: 0
    signal finished()

    readonly property var steps: [
        { "title": "Welcome to PulseBoost AI", "subtitle": "A new class of predictive PC intelligence." },
        { "title": "Initial System Scan", "subtitle": "Building your first baseline health profile." },
        { "title": "PulseModel Setup", "subtitle": "Your local adaptive AI engine is ready with offline diagnostics." },
        { "title": "Personalization", "subtitle": "Choose accent and alert profile." },
        { "title": "First Insight", "subtitle": "PulseBoost generated your first optimization plan." }
    ]

    GlassCard {
        anchors.centerIn: parent
        width: Math.min(600, parent.width * 0.9)
        height: 380
        fillColor: Style.bg2
        borderColor: Style.borderGlass

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width * 0.8
            spacing: Style.s20

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 64; height: 64; radius: 24
                color: Style.violetGlow
                border.color: Style.violet
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: Icons.glyph("bolt")
                    color: Style.violet
                    font.family: Style.fontDisplay
                    font.pixelSize: 32
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: steps[step].title
                color: Style.text0
                font.family: Style.fontDisplay
                font.pixelSize: Style.f28
                font.weight: Style.w700
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: steps[step].subtitle
                color: Style.text2
                font.family: Style.fontBody
                font.pixelSize: Style.f14
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: Style.s16 }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 6
                radius: 3
                color: Style.bg3
                Rectangle {
                    width: parent.width * ((step + 1) / steps.length)
                    height: parent.height
                    radius: parent.radius
                    color: Style.violet
                    
                    RectangularGlow {
                        anchors.fill: parent
                        glowRadius: 6; spread: 0.2; color: parent.color; opacity: 0.6
                    }
                    
                    Behavior on width { NumberAnimation { duration: Animations.normal; easing.type: Animations.easeStandard } }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Style.s16
                GlowButton {
                    label: step === steps.length - 1 ? "Finish" : "Next"
                    glowColor: Style.violet
                    Layout.preferredWidth: 160
                    onClicked: {
                        if (step < steps.length - 1) step += 1
                        else root.finished()
                    }
                }
                GlowButton {
                    label: "Skip Wizard"
                    variant: "ghost"
                    glowColor: Style.text2
                    onClicked: root.finished()
                }
            }
        }
    }
}
