import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: dialog

    label: "About..."

    content : ColumnLayout {
        id: mainFrame


        Text {
            Layout.fillWidth: true

            text: "<h1><b>OSMScout2</b></h1>"
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true

            text: "A demo application for the libosmscout library"
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true

            text: "See http://libosmscout.sf.net for more information"
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true

            text: "All geographic data: © OpenStreetMap contributors<br/>For license information see www.openstreetmap.org/copyright"
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            id: buttonRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: 10

            Item {
                Layout.fillWidth: true
            }

            DialogActionButton {
                id: ok
                text: "OK"

                onClicked: {
                    close()
                }
            }
        }
    }
}
