import QtQuick 2.12
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import net.sf.libosmscout.map 1.0

import "custom"

DialogBase {
    id: dialog
    minimumHeight: units.gu(28)
    title: "About ..."

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Text {
            Layout.fillWidth: true

            text: "<b>StyleEditor</b>"
            font.pixelSize: units.fs("large")
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true

            text: "Style sheet editor for the libosmscout library<br/>See http://libosmscout.sf.net"
            font.pixelSize: units.fs("medium")
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true

            text: "All geographic data:<br/>© OpenStreetMap contributors<br/>See www.openstreetmap.org/copyright"
            font.pixelSize: units.fs("medium")
            horizontalAlignment: Text.AlignHCenter
        }
    }

    footer: Row {
        leftPadding: units.gu(1)
        rightPadding: units.gu(1)
        bottomPadding: units.gu(1)
        spacing: units.gu(1)
        layoutDirection: Qt.RightToLeft

        Button {
            flat: true
            text: "OK"
            onClicked: {
                dialog.close()
            }
        }
    }
}
