import QtQuick 2.3
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

/*FocusScope {
    x: searchRectangle.x
    y: searchRectangle.x
    width: searchRectangle.width
    height: searchRectangle.height*/


    Rectangle {
        id: searchRectangle;

        property alias desktop : searchEdit.desktop;
        property alias desktopFreeSpace: searchEdit.desktopFreeSpace;
        property alias location: searchEdit.location;

        signal showLocation(Location location)

        width: searchContent.width;
        height: searchContent.height;

        color: "green"

        RowLayout {
            id: searchContent
            height: searchEdit.height
            spacing: 0

            LocationSearch {
                id: searchEdit;

                Layout.minimumWidth: Theme.averageCharWidth*5
                Layout.preferredWidth: Theme.averageCharWidth*45
                Layout.maximumWidth: Theme.averageCharWidth*60

                focus: true

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
                    searchEdit.enforceLocationValue();

                    if (searchEdit.location !== null) {
                        showLocation(searchEdit.location);
                    }
                }
            }
        }
    }
//}
