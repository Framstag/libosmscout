import QtQuick 2.7

import QtQuick.Controls 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Window 2.0

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"

Window {
    id: mainWindow
    objectName: "main"
    title: "OSMScout"
    visible: true
    width: 480
    height: 800

    function openAboutDialog() {
        var component = Qt.createComponent("AboutDialog.qml")
        var dialog = component.createObject(mainWindow, {})

        dialog.opened.connect(onDialogOpened)
        dialog.closed.connect(onDialogClosed)
        dialog.open()
    }

    function openMapDownloadDialog() {
        var component = Qt.createComponent("MapDownloadDialog.qml")
        var dialog = component.createObject(mainWindow, {})

        dialog.opened.connect(onDialogOpened)
        dialog.closed.connect(onDialogClosed)
        dialog.open()
    }

    function showLocation(location) {
        map.showLocation(location)
    }

    function onDialogOpened() {
        info.visible = false
        navigation.visible = false
    }

    function onDialogClosed() {
        info.visible = true
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

    Settings {
        id: settings
        //mapDPI: 50
    }
    AppSettings{
        id: appSettings
    }

    GridLayout {
        id: content
        anchors.fill: parent

        Map {
            id: map
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            renderingType: "plane" // or "tiled"
            interactiveIcons: true

            function getFreeRect() {
                return Qt.rect(Theme.horizSpace,
                               Theme.vertSpace+searchDialog.height+Theme.vertSpace,
                               map.width-2*Theme.horizSpace,
                               map.height-searchDialog.y-searchDialog.height-3*Theme.vertSpace)
            }

            function setupInitialPosition(){
                if (map.databaseLoaded){
                  if (map.isInDatabaseBoundingBox(appSettings.mapView.lat, appSettings.mapView.lon)){
                    map.view = appSettings.mapView;
                    console.log("restore last position: " + appSettings.mapView.lat + " " + appSettings.mapView.lon);
                  }else{
                    console.log("position " + appSettings.mapView.lat + " " + appSettings.mapView.lon + " is outside db, recenter");
                    map.recenter();
                  }
                }else{
                  map.view = appSettings.mapView;
                }
            }

            TiledMapOverlay {
                anchors.fill: parent
                view: map.view
                enabled: true
                opacity: 0.5
                // Please consider own tile server hosting before usage of this source in your app
                // https://www.karry.cz/en/karry/blog/2019/01/08/hillshade_tile_server/
                provider: {
                    "id": "OSMSCOUT_HILLSHADE",
                    "name": "Hillshade",
                    "servers": [
                        "https://osmscout.karry.cz/hillshade/tile.php?z=%1&x=%2&y=%3"
                    ],
                    "maximumZoomLevel": 19,
                    "copyright": "© IAT, METI, NASA, NOAA",
                    }
            }

            onTap: {
                console.log("tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon + " (map center "+ map.view.lat + " " + map.view.lon + ")");
                map.focus=true;
            }
            onLongTap: {
                console.log("long tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon);
                map.focus=true;
            }
            onViewChanged: {
                //console.log("map center "+ map.view.lat + " " + map.view.lon + "");
                appSettings.mapView = map.view;
            }
            Component.onCompleted: {
                setupInitialPosition();
            }
            onDatabaseLoaded: {
                setupInitialPosition();
            }

            Keys.onPressed: {
                if (event.key === Qt.Key_Plus) {
                    map.zoomIn(2.0)
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Minus) {
                    map.zoomOut(2.0)
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Up) {
                    map.moveUp()
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Down) {
                    map.moveDown()
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Left) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        map.rotateLeft();
                    }
                    else {
                        map.moveLeft();
                    }
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Right) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        map.rotateRight();
                    }
                    else {
                        map.moveRight();
                    }
                    event.accepted = true
                }
                else if (event.modifiers===Qt.ControlModifier &&
                         event.key === Qt.Key_F) {
                    searchDialog.focus = true
                    event.accepted = true
                }
                else if (event.modifiers===Qt.ControlModifier &&
                         event.key === Qt.Key_D) {
                    map.toggleDaylight();
                }
                else if (event.modifiers===Qt.ControlModifier &&
                         event.key === Qt.Key_R) {
                    map.reloadStyle();
                }
                else if (event.modifiers===(Qt.ControlModifier | Qt.ShiftModifier) &&
                         event.key === Qt.Key_D) {
                    var debugState = map.toggleDebug();

                    if (debugState) {
                      console.log("DEBUG is ON");
                    }
                    else {
                      console.log("DEBUG is OFF");
                    }
                }
                else if (event.modifiers===(Qt.ControlModifier | Qt.ShiftModifier) &&
                         event.key === Qt.Key_I) {
                    var infoState = map.toggleInfo();

                    if (infoState) {
                      console.log("INFO is ON");
                    }
                    else {
                      console.log("INFO is OFF");
                    }
                }
            }

            SearchDialog {
                id: searchDialog

                y: Theme.vertSpace

                anchors.horizontalCenter: parent.horizontalCenter

                desktop: map
                searchCenterLat: map.lat
                searchCenterLon: map.lon

                onShowLocation: {
                    console.log("location: "+location);
                    map.showLocation(location)
                }

                onStateChanged: {
                    if (state==="NORMAL") {
                        onDialogClosed()
                    }
                    else {
                        onDialogOpened()
                    }
                }
            }

            // Bottom left column
            ColumnLayout {
                id: info

                x: Theme.horizSpace
                y: parent.height-height-Theme.vertSpace

                spacing: Theme.mapButtonSpace

                MapButton {
                    id: downloadBtn
                    Image {
                        id: downloadIcon

                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * 0.8
                        height: parent.height * 0.8

                        source: "qrc:///pics/Download.svg"
                        fillMode: Image.PreserveAspectFit
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: Image.AlignVCenter
                        sourceSize.width: width
                        sourceSize.height: height
                    }
                    onClicked: {
                        openMapDownloadDialog();
                    }
                }

                MapButton {
                    id: about
                    text: "?"

                    onClicked: {
                        openAboutDialog()
                    }
                }
            }

            // Bottom right column
            ColumnLayout {
                id: navigation

                x: parent.width-width-Theme.horizSpace
                y: parent.height-height-Theme.vertSpace

                spacing: Theme.mapButtonSpace
/*
                MapButton {
                    id: analysis
                    checkable: true
                    text: "🔍"

                    onClicked: {
                        console.log("Toggle analysis mode "+checked);
                    }
                }*/

                MapButton {
                    id: resetRotation
                    text: "|"

                    onClicked: {
                        map.rotateTo(0);
                    }
                }

                MapButton {
                    id: recenter
                    text: "⊹"

                    onClicked: {
                        map.recenter()
                    }
                }

                MapButton {
                    id: zoomIn
                    text: "˖"

                    onClicked: {
                        map.zoomIn(2.0)
                    }
                }

                MapButton {
                    id: zoomOut
                    text: "˗"

                    onClicked: {
                        map.zoomOut(2.0)
                    }
                }
            }
        }
    }
}
