import QtQuick 2.3
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

FocusScope {
    id: searchDialog

    property Item desktop;
    property rect desktopFreeSpace;

    property alias location: searchEdit.location;

    signal showLocation(Location location)

    width: searchRectangle.width
    height: searchRectangle.height

    Rectangle {
        id: searchRectangle;

        width: searchContent.width;
        height: searchContent.height;

        RowLayout {
            id: searchContent

            spacing: 0

            LocationSearch {
                id: searchEdit;

                focus: true

                Layout.fillWidth: true
                Layout.minimumWidth: Theme.averageCharWidth*5
                Layout.preferredWidth: Theme.averageCharWidth*45
                Layout.maximumWidth: Theme.averageCharWidth*60

                desktop: searchDialog.desktop
                desktopFreeSpace: searchDialog.desktopFreeSpace
                location: location

                onShowLocation: {
                    searchDialog.showLocation(location)
                }
            }

            DialogActionButton {
                id: searchButton

                width: searchEdit.height
                height: searchEdit.height
                contentColor: "#0000ff"
                textColor: "white"
                text: "o"

                onClicked: {
                    searchEdit.enforceLocationValue();

                    if (searchEdit.location !== null) {
                        showLocation(searchEdit.location);
                    }
                }
            }
        }
    }
}
