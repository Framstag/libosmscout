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
    width: 1024
    height: 768

    function reroute(){
        var startLoc = routingModel.locationEntryFromPosition(simulator.latitude, simulator.longitude);
        var destinationLoc = routingModel.locationEntryFromPosition(simulator.endLat, simulator.endLon);
        console.log("We leave route, reroute from " + startLoc.label + " -> " + destinationLoc.label);
        routingModel.setStartAndTarget(startLoc, destinationLoc);
    }

    PositionSimulator {
        id: simulator
        track: PositionSimulationTrack
        Component.onCompleted: {
            console.log("PositionSimulationTrack: "+PositionSimulationTrack)
        }
        onEndChanged: {
            var startLoc = routingModel.locationEntryFromPosition(simulator.startLat, simulator.startLon);
            var destinationLoc = routingModel.locationEntryFromPosition(simulator.endLat, simulator.endLon);
            routingModel.setStartAndTarget(startLoc, destinationLoc);
        }

        onPositionChanged: {
            // if (!map.lockToPosition){ // don't set again when it is true already
            //     map.lockToPosition = true;
            // }
            map.locationChanged(true, // valid
                                latitude, longitude,
                                horizontalAccuracyValid, horizontalAccuracy);
            navigationModel.locationChanged(true, // valid
                                            latitude, longitude,
                                            horizontalAccuracyValid, horizontalAccuracy);
            // console.log("position: " + latitude + " " + longitude);
        }
    }

    NavigationModel {
        id: navigationModel
        route: routingModel.route

        onRerouteRequest: {
            if (routingModel.ready){
                reroute();
            }
        }
        onPositionEstimate: {
            console.log("onPositionEstimate " + lat + " " + lon);
        }
    }

    RoutingListModel {
        id: routingModel
        onReadyChanged: {
            var routeWay = routingModel.routeWay;
            if (routeWay==null){
                return;
            }

            map.addOverlayObject(0,routeWay);
        }
    }

    Map {
        id: map
        anchors.fill: parent
        showCurrentPosition: true
        vehiclePosition: navigationModel.vehiclePosition
        followVehicle: true

        focus: true

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
                map.up()
                event.accepted = true
            }
            else if (event.key === Qt.Key_Down) {
                map.down()
                event.accepted = true
            }
            else if (event.key === Qt.Key_Left) {
                if (event.modifiers & Qt.ShiftModifier) {
                    map.rotateLeft();
                }
                else {
                    map.left();
                }
                event.accepted = true
            }
            else if (event.key === Qt.Key_Right) {
                if (event.modifiers & Qt.ShiftModifier) {
                    map.rotateRight();
                }
                else {
                    map.right();
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
    }
    RowLayout {
        id: simulatorControl

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

    Rectangle {
        id: topContainer
        anchors.left: parent.left
        anchors.top: parent.top
        width: Math.min(420, parent.width - rightContainer.width)
        height: 130
        color: "transparent"

        Rectangle {
            id: nextStepBox

            x: Theme.horizSpace
            y: Theme.vertSpace
            height: parent.height
            width: parent.width - (2*Theme.horizSpace)

            border.color: "lightgrey"
            border.width: 1

            RouteStepIcon{
                id: nextStepIcon
                stepType: navigationModel.nextRouteStep.type
                roundaboutClockwise: navigationModel.nextRouteStep.roundaboutClockwise
                height: parent.height
                width: height
                anchors{
                    top: parent.top
                    left: parent.left
                }
                Text{
                    id: roundaboutExit
                    text: navigationModel.nextRouteStep.type == "leave-roundabout" ? navigationModel.nextRouteStep.roundaboutExit : ""
                    anchors.centerIn: parent
                    font.pixelSize: Theme.textFontSize*2
                }
            }
            Rectangle {
                anchors{
                    top: parent.top
                    right: parent.right
                }
                width: currentSpeedText.width+(2*Theme.horizSpace)
                height: currentSpeedText.height+(2*Theme.vertSpace)
                color: (navigationModel.currentSpeed > 0 &&
                        navigationModel.maxAllowedSpeed > 0 &&
                        navigationModel.currentSpeed > navigationModel.maxAllowedSpeed) ?
                           "red":"transparent"

                Text{
                    id: currentSpeedText
                    anchors.centerIn: parent
                    text: navigationModel.currentSpeed < 0 ? "--" : (Math.round(navigationModel.currentSpeed)+" km/h")
                    font.pixelSize: Theme.textFontSize
                }
            }
            Text{
                id: distanceToNextStep

                function humanDistance(distance){
                    if (distance < 150){
                        return Math.round(distance/10)*10 + " "+ qsTr("meters");
                    }
                    if (distance < 2000){
                        return Math.round(distance/100)*100 + " "+ qsTr("meters");
                    }
                    return Math.round(distance/1000) + " "+ qsTr("km");
                }
                text: humanDistance(navigationModel.nextRouteStep.distanceTo)
                font.pixelSize: Theme.textFontSize*2
                anchors{
                    top: parent.top
                    left: nextStepIcon.right
                }
            }
            Text{
                id: nextStepStreets
                text: "via " + navigationModel.nextRouteStep.streetNames.join(", ")
                font.pixelSize: Theme.textFontSize
                opacity: 0.7
                visible: navigationModel.nextRouteStep.streetNames.length > 0
                wrapMode: Text.NoWrap
                clip: true
                anchors{
                    top: distanceToNextStep.bottom
                    left: nextStepIcon.right
                    right: parent.right
                }
            }
            Text{
                id: nextStepDescription
                text: navigationModel.nextRouteStep.description
                font.pixelSize: Theme.textFontSize
                wrapMode: Text.Wrap
                anchors{
                    top: nextStepStreets.bottom
                    left: nextStepIcon.right
                    right: parent.right
                }
            }
        }
    }

    Rectangle {
        id: rightContainer
        anchors.right: parent.right
        anchors.top: parent.top
        width: 220
        height: parent.height
        color: "transparent"

        Rectangle {
            id: routingStepsBox

            x: 0
            y: Theme.vertSpace
            height: parent.height - (2*Theme.vertSpace)
            width: parent.width - Theme.horizSpace

            border.color: "lightgrey"
            border.width: 1

            ListView {
                id: routingView

                model: navigationModel

                anchors.fill: parent
                anchors.margins: 1
                clip: true

                delegate: RoutingStep{}
            }

            ScrollIndicator {
                flickableArea: routingView
            }
        }
    }
}
