import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Window 2.0

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"

Window {
    id: mainWindow
    objectName: "main"
    title: "OSMScout navigation simulation"
    visible: true
    width: 800
    height: 480

    PositionSimulator {
        track: PositionSimulationTrack
        Component.onCompleted: {
            console.log("PositionSimulationTrack: "+PositionSimulationTrack)
        }
        onStartChanged: {
            map.showCoordinates(latitude, longitude);
        }
        onPositionChanged: {
            if (!map.lockToPosition){ // don't set again when it is true already
                map.lockToPosition = true;
            }
            map.locationChanged(true, // valid
                                latitude, longitude,
                                horizontalAccuracyValid, horizontalAccuracy);
            // console.log("position: " + latitude + " " + longitude);
        }
    }
    Map {
        id: map
        anchors.fill: parent
        showCurrentPosition: true
    }
}
