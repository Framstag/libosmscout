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
        id: simulator
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
    RowLayout {
        id: info

        x: Theme.horizSpace
        y: parent.height-height-Theme.vertSpace

        spacing: Theme.mapButtonSpace

        Rectangle {
          id: mapButton
          color: "white"
          border.color: "grey"
          border.width: 1
          opacity: 0.8

          width: label.width + Theme.horizSpace*2
          height: Theme.mapButtonHeight

          Text {
            id: label
            anchors.centerIn: parent
            color: "black"
            font.pixelSize: Theme.mapButtonFontSize
            text: simulator.time
          }
        }
        MapButton{
            label: "reset"
            onClicked: {
                simulator.track=PositionSimulationTrack;
            }
        }
        MapButton{
            label: simulator.running ? "pause" : "resume"
            onClicked: {
                simulator.running=!simulator.running
            }
        }
        MapButton{
            label: "+ 10s"
            onClicked: {
                simulator.skipTime(10 * 1000);
            }
        }
        MapButton{
            label: "+ minute"
            onClicked: {
                simulator.skipTime(60 * 1000);
            }
        }
        MapButton{
            label: "+ hour"
            onClicked: {
                simulator.skipTime(3600 * 1000);
            }
        }
    }
}
