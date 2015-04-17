import QtQuick 2.3
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

Rectangle {
    id: searchRectangle;

    property Item desktop;
    property rect desktopFreeSpace;
    property Location location

    width: searchContent.width;
    height: searchContent.height;

    RowLayout {
        id: searchContent
        Layout.fillWidth: true
        height: searchEdit.height
        spacing: 0

        LocationSearch {
            id: searchEdit;

            Layout.fillWidth: true
            Layout.minimumWidth: Theme.averageCharWidth*5
            Layout.preferredWidth: Theme.averageCharWidth*35
            Layout.maximumWidth: Theme.averageCharWidth*50

            desktop: searchRectangle.desktop
            desktopFreeSpace: searchRectangle.desktopFreeSpace
            location: location

            onShowLocation: {
                searchRectangle.showLocation(location)
            }
        }

        DialogActionButton {
            width: searchEdit.height
            height: searchEdit.height
            contentColor: "#0000ff"
            textColor: "white"
            text: "o"

            onClicked: {
                if (searchEdit.location !== null) {
                    showLocation(searchEdit.location);
                }
            }
        }
    }

    signal showLocation(Location location)
}
