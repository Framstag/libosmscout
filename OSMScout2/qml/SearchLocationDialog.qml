import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: dialog

    label: "Search location..."

    content : RowLayout {
        id: mainFrame

        Layout.alignment: Qt.AlignTop

        Text {
            Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

            text: "Location:"
        }

        LocationEdit {
            id: locationInput

            Layout.minimumWidth: Theme.averageCharWidth*40
            Layout.preferredWidth: Theme.averageCharWidth*60
            Layout.maximumWidth: Theme.averageCharWidth*100

            horizontalAlignment: TextInput.AlignLeft
        }

        DialogActionButton {
            id: ok
            text: "OK"

            onClicked: {
                locationInput.enforceLocationValue()

                var location = locationInput.location

                if (location != null) {
                    showLocation(location)
                }

                close()
            }
        }
    }

    signal showLocation(Location location)

    onOpened: {
        locationInput.focus = true
    }
}
