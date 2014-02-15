import QtQuick 2.2

import net.sf.libosmscout.map 1.0

import "custom"

Rectangle {
    id: mainWindow
    objectName: "main"
    color: "white"

    function showCoordinates(lat, lon) {
        map.showCoordinates(lat, lon)
    }

    function showLocation(location) {
        map.showLocation(location)
    }

    Map {
        id: map
        anchors.fill: parent

        focus: true

        Keys.onPressed: {
            if (event.key === Qt.Key_Plus) {
                map.zoomIn(2.0)
            }
            else if (event.key === Qt.Key_Minus) {
                map.zoomOut(2.0)
            }
            else if (event.key === Qt.Key_Up) {
                map.up()
            }
            else if (event.key === Qt.Key_Down) {
                map.down()
            }
            else if (event.key === Qt.Key_Left) {
                map.left()
            }
            else if (event.key === Qt.Key_Right) {
                map.right()
            }
        }

        // Use PinchArea for multipoint zoom in/out?
    }

    Column {
        id: menu
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 10
        spacing: 10

        MapButton {
            id: searchGeocode
            label: ","

            onClicked: {
                var component = Qt.createComponent("SearchGeocodeDialog.qml")
                var dialog = component.createObject(mainWindow, {"lat": map.lat, "lon": map.lon})

                dialog.showCoordinates.connect(showCoordinates)
                dialog.open()
            }
        }

        MapButton {
            id: searchLocation
            label: "l"

            onClicked: {
                var component = Qt.createComponent("SearchLocationDialog.qml")
                var dialog = component.createObject(mainWindow, {})

                dialog.showLocation.connect(showLocation)
                dialog.open()
            }
        }

        MapButton {
            id: route
            label: "#"

            onClicked: {
                var component = Qt.createComponent("RoutingDialog.qml")
                var dialog = component.createObject(mainWindow, {})

                dialog.open()
            }
        }

        MapButton {
            id: settings
            label: "*"

            onClicked: {
                console.log("Show settings")
            }
        }
    }

    Column {
        id: navigation
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        spacing: 10

        MapButton {
            id: zoomIn
            label: "+"

            onClicked: {
                map.zoomIn(2.0)
            }
        }

        MapButton {
            id: zoomOut
            label: "-"

            onClicked: {
                map.zoomOut(2.0)
            }
        }
    }
}
