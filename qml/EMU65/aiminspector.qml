import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 720
    height: 640
    title: "EMU65 Debugger"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Text {
            id: aimInspCpu
            Layout.fillWidth: true
            font.family: "monospace"
            text: qsTr("CPU Status: ") + aimInspector.cpuStatus

            Connections {
                target: aimInspector
                function onCpuStatusChanged() {
                    aimInspCpu.text = qsTr("CPU Status: ") + aimInspector.cpuStatus
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Text {
                id: aimInspLed
                Layout.alignment: Qt.AlignTop
                font.family: "monospace"
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
                text: qsTr("Printer Registers:\n")

                Connections {
                    target: aimInspector
                    function onPrinterStatusChanged() {
                        aimInspPrinter.text = qsTr("Printer Registers:\n") + aimInspector.printerStatus
                    }
                }
            }

            Item {
                // Absorbs any extra horizontal space so the register panels
                // above stay left-aligned and don't get stretched apart.
                Layout.fillWidth: true
            }
        }

        Text {
            text: qsTr("Memory Contents")
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Text {
                id: aimInspComponent
                font.family: "monospace"
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
