import QtQuick 2.2

import "custom"
import net.sf.libosmscout.map 1.0

Rectangle {
    property alias stylesheetFilename: mapView.stylesheetFilename
    width: 100
    height: 62

    function showCoordinates(lat, lon) {
        mapView.showCoordinates(lat, lon)
    }

    function showLocation(location) {
        mapView.showLocation(location)
    }

    function reloadStyle() {
        mapView.reloadStyle()
    }

    function reloadTmpStyle() {
        mapView.reloadTmpStyle()
    }


    Map {
        id: mapView
        anchors.fill: parent

        focus: true
/*
        Keys.onPressed: {
            if (event.key === Qt.Key_Plus) {
                mapView.zoomIn(2.0)
            }
            else if (event.key === Qt.Key_Minus) {
                mapView.zoomOut(2.0)
            }
            else if (event.key === Qt.Key_Up) {
                mapView.up()
            }
            else if (event.key === Qt.Key_Down) {
                mapView.down()
            }
            else if (event.key === Qt.Key_Left) {
                mapView.left()
            }
            else if (event.key === Qt.Key_Right) {
                mapView.right()
            }
        }
*/
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
                var dialog = component.createObject(mainWindow, {"lat": mapView.lat, "lon": mapView.lon})

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
                mapView.zoomIn(2.0)
            }
        }

        MapButton {
            id: zoomOut
            label: "-"

            onClicked: {
                mapView.zoomOut(2.0)
            }
        }
    }

    Row {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.left: parent.left
        anchors.leftMargin: 10
        spacing: 10

        Rectangle {
            color: "#ffffff"
            opacity: 0.5
            Text {
                id: zoomLevelLabel
                text: qsTr("Zoom level: ")+mapView.zoomLevelName+" ["+mapView.zoomLevel+"]"
            }
        }
    }
}
