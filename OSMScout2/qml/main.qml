import QtQuick 2.2

import net.sf.libosmscout.map 1.0

import "custom"

Rectangle {
    id: mainWindow
    objectName: "main"
    color: "white"

    function openCoordinateDialog() {
        var component = Qt.createComponent("SearchGeocodeDialog.qml")
        var dialog = component.createObject(mainWindow, {"lat": map.lat, "lon": map.lon } )

        dialog.showCoordinates.connect(showCoordinates)
        dialog.opened.connect(onDialogOpened)
        dialog.closed.connect(onDialogClosed)
        dialog.open()
    }

    function openSearchLocationDialog() {
        var component = Qt.createComponent("SearchLocationDialog.qml")
        var dialog = component.createObject(mainWindow, {})

        dialog.showLocation.connect(showLocation)
        dialog.opened.connect(onDialogOpened)
        dialog.closed.connect(onDialogClosed)
        dialog.open()
    }

    function openRoutingDialog() {
        var component = Qt.createComponent("RoutingDialog.qml")
        var dialog = component.createObject(mainWindow, {})

        dialog.opened.connect(onDialogOpened)
        dialog.closed.connect(onDialogClosed)
        dialog.open()
    }

    function showCoordinates(lat, lon) {
        map.showCoordinates(lat, lon)
    }

    function showLocation(location) {
        map.showLocation(location)
    }

    function onDialogOpened() {
        menu.visible = false
        navigation.visible = false
    }

    function onDialogClosed() {
        menu.visible = true
        navigation.visible = true

        map.focus = true
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
            else if (event.modifiers===Qt.ControlModifier &&
                     event.key === Qt.Key_F) {
                openSearchLocationDialog()
            }
            else if (event.modifiers===Qt.ControlModifier &&
                     event.key === Qt.Key_R) {
                openRoutingDialog()
            }
        }

        // Use PinchArea for multipoint zoom in/out?
    }

    Column {
        id: menu
        anchors.top: parent.top
        anchors.topMargin: Theme.mapButtonSpace
        anchors.left: parent.left
        anchors.leftMargin: Theme.mapButtonSpace
        spacing: Theme.mapButtonSpace

        MapButton {
            id: searchGeocode
            label: ","

            onClicked: {
                openCoordinateDialog()
            }
        }

        MapButton {
            id: searchLocation
            label: "l"

            onClicked: {
                openSearchLocationDialog()
            }
        }

        MapButton {
            id: route
            label: "#"

            onClicked: {
                openRoutingDialog()
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
        anchors.topMargin: Theme.mapButtonSpace
        anchors.right: parent.right
        anchors.rightMargin: Theme.mapButtonSpace
        spacing: Theme.mapButtonSpace

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
