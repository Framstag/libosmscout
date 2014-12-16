import QtQuick 2.2
import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"

Rectangle {
    id: mainWindow
    objectName: "main"
    color: "white"

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

    function openAboutDialog() {
        var component = Qt.createComponent("AboutDialog.qml")
        var dialog = component.createObject(mainWindow, {})

        dialog.opened.connect(onDialogOpened)
        dialog.closed.connect(onDialogClosed)
        dialog.open()
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

    PositionSource {
        id: positionSource

        active: true

        onValidChanged: {
            console.log("Positioning is " + valid)
            console.log("Last error " + sourceError)

            for (var m in supportedPositioningMethods) {
                console.log("Method " + m)
            }
        }

        onPositionChanged: {
            console.log("Position changed:")

            if (position.latitudeValid) {
                console.log("  latitude: " + position.coordinate.latitude)
            }

            if (position.longitudeValid) {
                console.log("  longitude: " + position.coordinate.longitude)
            }

            if (position.altitudeValid) {
                console.log("  altitude: " + position.coordinate.altitude)
            }

            if (position.speedValid) {
                console.log("  speed: " + position.speed)
            }

            if (position.horizontalAccuracyValid) {
                console.log("  horizontal accuracy: " + position.horizontalAccuracy)
            }

            if (position.verticalAccuracyValid) {
                console.log("  vertical accuracy: " + position.verticalAccuracy)
            }
        }
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
                if (event.modifiers & Qt.ShiftModifier) {
                    map.rotateLeft();
                }
                else {
                    map.left();
                }
            }
            else if (event.key === Qt.Key_Right) {
                if (event.modifiers & Qt.ShiftModifier) {
                    map.rotateRight();
                }
                else {
                    map.right();
                }
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
    }

    Column {
        id: info
        anchors.left: parent.left
        anchors.leftMargin: Theme.mapButtonSpace
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.mapButtonSpace
        spacing: Theme.mapButtonSpace


        MapButton {
            id: about
            label: "?"

            onClicked: {
                openAboutDialog()
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
