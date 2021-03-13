import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Window 2.0

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"
import "custom/Utils.js" as Utils

Window {
    id: mainWindow
    objectName: "main"
    title: "OSMScout routing"
    visible: true
    width: 1024
    height: 768

    function locationStr(location){
        if (location==null){
            return "null";
        }
        return (location.label=="" ) ?
                    location.lat.toFixed(5) + " " + location.lon.toFixed(5) :
                    "\"" + location.label + "\"";
    }

    RoutingListModel {
        id: routingModel

        onRoutingProgress: {
            routingState.text = "Searching route: " + Math.round(percent) + "%"
        }

        onReadyChanged: {
            if (!ready) {
                routingState.text = "Preparing...";
            }

            var routeWay = routingModel.routeWay;
            if (routeWay==null){
                return;
            }

            routingState.text = "Done";
            map.addOverlayObject(0,routeWay);
        }
    }

    SplitView {
        id: mainRow
        anchors.fill: parent
        orientation: Qt.Horizontal

        Map {
            id: map
            Layout.fillWidth: true
            width: parent.width*0.6
            Layout.minimumWidth: 200

            showCurrentPosition: true
            focus: true

            onTap: {
                if (startInput.inputFocus) {
                    startInput.text = Utils.formatCoord(lat, lon);
                }
                if (targetInput.inputFocus) {
                    targetInput.text = Utils.formatCoord(lat, lon);
                }
            }

            function setupInitialPosition(){
                if (map.databaseLoaded){
                    map.recenter();
                }
            }

            Component.onCompleted: {
                setupInitialPosition();
            }
            onDatabaseLoaded: {
                setupInitialPosition();
            }
        }

        Rectangle {
            id: rightPanel
            width: parent.width*0.4
            Layout.minimumWidth: 300

            GridLayout {
                id: routingControl
                columns: 2
                columnSpacing: 6
                rowSpacing: 6

                anchors{
                    top: parent.top
                    right: parent.right
                    left: parent.left
                    margins: 6
                }

                property LocationEntry routeFromLocation
                property LocationEntry routeToLocation

                function reroute(){
                    if (routeFromLocation!=null){
                        map.addPositionMark(0, routeFromLocation.lat, routeFromLocation.lon);
                    } else {
                        map.removePositionMark(0);
                    }

                    if (routeToLocation!=null){
                        map.addPositionMark(1, routeToLocation.lat, routeToLocation.lon);
                    } else {
                        map.removePositionMark(1);
                    }

                    if (routeFromLocation==null || routeToLocation==null){
                        return;
                    }

                    console.log("Route from " + locationStr(routeFromLocation) + " -> " + locationStr(routeToLocation));
                    routingModel.setStartAndTarget(routeFromLocation, routeToLocation);
                }

                Text {
                    text: qsTr("Start:")
                }

                PlaceInput {
                    id: startInput
                    onLocationChanged: {
                        routingControl.routeFromLocation = startInput.location;
                        routingControl.reroute();
                    }
                }

                Text {
                    text: qsTr("Target:")
                }

                PlaceInput {
                    id: targetInput
                    onLocationChanged: {
                        routingControl.routeToLocation = targetInput.location;
                        routingControl.reroute();
                    }
                }

                Text {
                    id: routingState
                    text: qsTr("No start or target set")
                }
            }

            Rectangle {
                id: routingStepsBox

                anchors{
                    top: routingControl.bottom
                    right: parent.right
                    bottom: parent.bottom
                    left: parent.left
                }

                border.color: "lightgrey"
                border.width: 1

                ListView {
                    id: routingView

                    model: routingModel

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
}
