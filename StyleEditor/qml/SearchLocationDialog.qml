import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: dialog

    signal showLocation(LocationEntry location)

    Rectangle {
        id: content
        width: mainFrame.implicitWidth+20
        height: mainFrame.implicitHeight+20

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 10

        GridLayout {
            id: mainFrame
            anchors.margins: 10
            anchors.fill: parent

            columns: 3

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                text: "Location:"
            }

            LocationEdit {
                id: locationInput
                width: 250

                horizontalAlignment: TextInput.AlignLeft
            }

            DialogActionButton {
                id: ok
                width: 25
                height: 25
                text: "OK"

                onClicked: {
                    locationInput.enforceLocationValue()

                    var location = locationInput.location

                    if (location != null) {
                        showLocation(location)
                    }

                    dialog.destroy()
                }
            }
        }
    }
}
