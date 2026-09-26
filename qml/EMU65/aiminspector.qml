import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

ApplicationWindow {
    // The Controls "system" style follows the desktop's own (possibly dark)
    // palette, but nothing here reads from that palette explicitly, so text
    // color and background can end up low-contrast (e.g. dark-grey-on-dark
    // theme). Fixed light-on-dark colors are used throughout instead, so
    // this window is always readable regardless of the desktop theme.
    id: root
    visible: true
    width: 720
    height: 640
    title: "EMU65 Debugger"
    color: "#1e1e1e"

    readonly property color textColor: "#e6e6e6"
    readonly property color headingColor: "#8ab4f8"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Text {
            id: aimInspCpu
            Layout.fillWidth: true
            font.family: "monospace"
            color: root.textColor
            text: qsTr("CPU Status: ") + aimInspector.cpuStatus

            Connections {
                target: aimInspector
                function onCpuStatusChanged() {
                    aimInspCpu.text = qsTr("CPU Status: ") + aimInspector.cpuStatus
                }
            }
        }

        // Once the CPU boots properly the LED panel alone dumps the whole
        // 0xAC00-0xAC43 range (~70 lines), which used to push "Memory
        // Contents" below the bottom of the window. Give the register panels
        // their own scrollable area sharing the height equally with the
        // memory dump (preferredHeight: 1 on both = same stretch ratio).
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 1
            clip: true

            RowLayout {
                spacing: 16

                Text {
                    id: aimInspLed
                    Layout.alignment: Qt.AlignTop
                    font.family: "monospace"
                    color: root.textColor
                    text: qsTr("LED Registers\n")

                    Connections {
                        target: aimInspector
                        function onLedStatusChanged() {
                            aimInspLed.text = qsTr("LED Registers\n") + aimInspector.ledStatus
                        }
                    }
                }

                Text {
                    id: aimInspKeyboard
                    Layout.alignment: Qt.AlignTop
                    font.family: "monospace"
                    color: root.textColor
                    text: qsTr("Keyboard Registers\n")

                    Connections {
                        target: aimInspector
                        function onKeyboardStatusChanged() {
                            aimInspKeyboard.text = qsTr("Keyboard Registers\n") + aimInspector.keyboardStatus
                        }
                    }
                }

                Text {
                    id: aimInspPrinter
                    Layout.alignment: Qt.AlignTop
                    font.family: "monospace"
                    color: root.textColor
                    text: qsTr("Printer Registers:\n")

                    Connections {
                        target: aimInspector
                        function onPrinterStatusChanged() {
                            aimInspPrinter.text = qsTr("Printer Registers:\n") + aimInspector.printerStatus
                        }
                    }
                }
            }
        }

        Text {
            text: qsTr("Memory Contents")
            color: root.headingColor
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 1
            clip: true

            Text {
                id: aimInspComponent
                font.family: "monospace"
                color: root.textColor
                text: aimInspector.componentStatus

                Connections {
                    target: aimInspector
                    function onComponentStatusChanged() {
                        aimInspComponent.text = aimInspector.componentStatus
                    }
                }
            }
        }
    }
}
