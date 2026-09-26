import QtQuick
import QtQuick.Controls
import QtQuick.Window

ApplicationWindow {
    visible: true
    width: 800
    height: 380
    title: "EMU65"

    menuBar: MenuBar {
        Menu {
            title: qsTr("Computer")
            MenuItem {
                text: aim65Controller.powerOn ? qsTr("Power OFF") : qsTr("Power ON")
                onTriggered: aim65Controller.powerOn = !aim65Controller.powerOn
            }
            MenuItem {
                text: qsTr("Reset")
                enabled: aim65Controller.powerOn
                onTriggered: aim65Controller.Reset()
            }
            MenuItem {
                text: aim65Controller.stepMode ? qsTr("Switch to RUN Mode") : qsTr("Switch to STEP Mode")
                onTriggered: aim65Controller.stepMode = !aim65Controller.stepMode
            }
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr("Help")
            MenuItem {
                text: qsTr("Documentation")
                onTriggered: documentationDialog.open()
            }
            MenuItem {
                text: qsTr("About")
                onTriggered: aboutDialog.open()
            }
        }
    }

    /* 800 x 380 */
    Image {
        id: whiteBackground
        anchors.top: parent.top
        width: parent.width
        height: parent.height / 2
        source: "../../res/img/top-section.png"
    }

    Image {
        anchors.top: whiteBackground.bottom
        width: parent.width
        height: parent.height / 2
        source: "../../res/img/bottom-section.png"
    }

    Rectangle {
        id: switchPanel
        width: 180
        height: 100
        x: 10
        y: 160
        color: "transparent"

        Rectangle {
            id: rstRunKbRect
            width: switchPanel.width
            height: 25
            anchors.top : switchPanel.top
            anchors.left: switchPanel.left
            color: "black"
            Text {
                text: qsTr("RESET")
                color: "white"
                anchors.top: rstRunKbRect.top
                anchors.left: rstRunKbRect.left
                anchors.topMargin: 5
                anchors.leftMargin: 20
            }

            Text {
                text: qsTr("RUN")
                color: "white"
                anchors.top: rstRunKbRect.top
                anchors.horizontalCenter: rstRunKbRect.horizontalCenter
                anchors.topMargin: 5
            }

            Text {
                text: qsTr("KB")
                color: "white"
                anchors.top: rstRunKbRect.top
                anchors.right: rstRunKbRect.right
                anchors.rightMargin: 30
                anchors.topMargin: 5
            }
        }

        Rectangle
        {
            id: stepTtyRect
            width: switchPanel.width * (2/3)
            height: 25
            anchors.bottom : switchPanel.bottom
            anchors.right: switchPanel.right
            color: "black"
            Text {
                text: qsTr("STEP")
                color: "white"
                anchors.bottom: stepTtyRect.bottom
                anchors.left: stepTtyRect.left
                anchors.horizontalCenter: stepTtyRect.horizontalCenter
                anchors.leftMargin: 15
                anchors.bottomMargin: 5
            }

            Text {
                text: qsTr("TTY")
                color: "white"
                anchors.bottom: stepTtyRect.bottom
                anchors.right: stepTtyRect.right
                anchors.rightMargin: 30
                anchors.bottomMargin: 5
            }
        }

        Image {
            id: resetBtn
            width: 50
            height: 50
            source: "../../res/img/button-image.png"
            anchors.verticalCenter: switchPanel.verticalCenter
            anchors.left: switchPanel.left
            anchors.leftMargin: 10

            // Pressed look: shrink slightly while held down.
            scale: resetMouseArea.pressed ? 0.9 : 1.0

            MouseArea {
                id: resetMouseArea
                anchors.fill : parent
                onClicked: aim65Controller.Reset()
            }
        }

        Image {
            id: runStepBtn
            width: 50
            height: 50
            source: "../../res/img/switch-image.png"
            anchors.horizontalCenter: switchPanel.horizontalCenter
            anchors.verticalCenter: switchPanel.verticalCenter
            // The lever image points up (RUN, label above); flipped it
            // points down towards the STEP label below.
            rotation: aim65Controller.stepMode ? 180 : 0

            MouseArea {
                anchors.fill : parent
                onClicked: aim65Controller.stepMode = !aim65Controller.stepMode
            }
        }

        Image {
            id: kbTtyBtn
            width: 50
            height: 50
            source: "../../res/img/switch-image.png"
            anchors.verticalCenter: switchPanel.verticalCenter
            anchors.right: switchPanel.right
            anchors.rightMargin: 10
            // Up = KB, down = TTY (see runStepBtn).
            rotation: aim65Controller.ttyMode ? 180 : 0

            MouseArea {
                anchors.fill : parent
                onClicked: aim65Controller.ttyMode = !aim65Controller.ttyMode
            }
        }
    }

    Rectangle {
        width: 550
        height: 180
        anchors.left: switchPanel.right
        anchors.leftMargin: 20
        color: "black"
        y: 50

        Rectangle {
            id: display
            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            anchors.horizontalCenter: parent.Center
            height: 80
            width: 520
            border.color: "white"
            border.width: 2
            color: "black"

            Text {
                id: ledScreen
                text: {
                    qsTr(ledDisplay.GetLedDisplay())
                }
                color: "red"
                font.pointSize: 30
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter

                Connections {
                    target: ledDisplay
                    function onDisplayDigitChanged() {
                        ledScreen.text = qsTr(ledDisplay.GetLedDisplay())
                    }
                }
            }
        }

        Image {
            source: "../../res/img/rockwell-brand.png"
            width: 200
            height: 50
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 13
        }
    }

    Dialog {
        id: aboutDialog
        title: qsTr("About EMU65")
        anchors.centerIn: parent
        width: 460
        modal: true
        standardButtons: Dialog.Ok
        onClosed: keyRegistrar.forceActiveFocus()

        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("<b>EMU65</b><br>Emulator of the Rockwell AIM 65, a single-board computer based on the 6502 CPU.<br><br>"
                       + "6502 CPU core by Marat Fayzullin.<br>"
                       + "Built with Qt 6.<br><br>"
                       + "Released under the licence in LICENCE.txt.")
        }
    }

    Dialog {
        id: documentationDialog
        title: qsTr("EMU65 Documentation")
        anchors.centerIn: parent
        width: 560
        height: 340
        modal: true
        standardButtons: Dialog.Ok
        onClosed: keyRegistrar.forceActiveFocus()

        ScrollView {
            anchors.fill: parent
            clip: true

            Label {
                width: documentationDialog.availableWidth
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                text: qsTr("<h3>Front panel</h3>"
                           + "<p><b>RESET</b> (button, or Computer &rarr; Reset): restarts the 6502 from the reset vector. RAM is preserved, so the Monitor performs a warm start.</p>"
                           + "<p><b>Power</b> (Computer &rarr; Power ON/OFF): OFF stops the CPU and blanks the display; ON clears RAM and boots from scratch.</p>"
                           + "<p><b>RUN/STEP</b> (switch, or Computer menu): in STEP the CPU raises an NMI after every instruction executed outside the Monitor ROM (0xE000-0xFFFF). The Monitor stops and shows the registers; press a key to execute the next instruction.</p>"
                           + "<p><b>KB/TTY</b> (switch): selects the terminal. Only the switch position is shown: the TTY interface is not emulated, the keyboard is always used.</p>"
                           + "<h3>Keyboard</h3>"
                           + "<p>Type on the PC keyboard with the main window focused. ASCII characters 32-94 are accepted; '[', ']' and '^' act as F1, F2 and F3.</p>"
                           + "<h3>Debugger</h3>"
                           + "<p>The <i>EMU65 Debugger</i> window shows the CPU registers, the LED, keyboard and printer registers, and the memory around the most recent RAM write.</p>"
                           + "<h3>More</h3>"
                           + "<p>See README.txt for build instructions and the project layout; doc/html holds the Doxygen API reference.</p>")
            }
        }
    }

    Item {
        id: keyRegistrar
        focus: true
        anchors.fill: parent
        Keys.onPressed: {
            // A powered-off machine has no keyboard scanning: a key here
            // would leave an IRQ pending that fires on the next power on.
            if (aim65Controller.powerOn)
                keyboard.pressedKey = event.key
            event.accepted = true
        }
    }
}
