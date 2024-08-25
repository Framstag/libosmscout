import QtQuick 2.12
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import net.sf.libosmscout.map 1.0

import "custom"

DialogBase {
    id: dialog
    minimumWidth: units.gu(70)

    signal showLocation(LocationEntry location)

    GridLayout {
        anchors.centerIn: parent

        columns: 3

        Text {
            Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
            font.pixelSize: units.fs("large")
            text: "Location:"
        }

        LocationEdit {
            id: locationInput
            width: units.gu(45)

            horizontalAlignment: TextInput.AlignLeft
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
