import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Window 2.0

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

Window {
    id: mainWindow
    objectName: "main"
    title: "ElevationProfileChart"
    visible: true
    width: 800
    height: 600

    // provided as global context:
    //  - string routeFrom
    //  - string routeTo
    //  - string routeVehicle

    property LocationEntry routeFromLocation
    property LocationEntry routeToLocation

    function locationStr(location){
        if (location==null){
            return "null";
        }
        return (location.label=="" ) ?
                    location.lat.toFixed(5) + " " + location.lon.toFixed(5) :
                    "\"" + location.label + "\"";
    }


    Item {
        id: stateMachine
        state: "resolvingFromLocation"
        states: [
            State {
                name: "resolvingFromLocation"
                PropertyChanges {
                   target: locationSearchModel
                   pattern: routeFrom
                }
                PropertyChanges {
                   target: statusText
                   text: "Looking location " + routeFrom + "..."
                }
            },
            State {
                name: "resolvingToLocation"
                PropertyChanges {
                   target: locationSearchModel
                   pattern: routeTo
                }
                PropertyChanges {
                   target: statusText
                   text: "Looking location " + routeTo + "..."
                }
            },
            State {
                name: "routing"
                PropertyChanges {
                   target: statusText
                   text: "Routing from " + locationStr(routeFromLocation) + " to " + locationStr(routeToLocation) + "..."
                }
            },
            State {
                name: "computingElevation"
                PropertyChanges {
                   target: statusText
                   text: "Computing route elevation profile..."
                }
            },
            State {
                name: "done"
                PropertyChanges {
                   target: statusText
                   visible: false
                }
            },
            State { name: "error"}
        ]

        onStateChanged: {
            console.log("State changed: " + state);
            if (state=="routing" && routeFromLocation != null && routeToLocation != null){
                console.log("Routing from " + locationStr(routeFromLocation) + " to " + locationStr(routeToLocation) + " with " + routeVehicle + "...");
                routingModel.setStartAndTarget(routeFromLocation, routeToLocation, routeVehicle);
            }
        }
    }

    LocationListModel {
        id: locationSearchModel

        onCountChanged:{
            console.log("Current state: " + stateMachine.state + ", entry count: " + count);
            if (stateMachine.state=="resolvingFromLocation" || stateMachine.state=="resolvingToLocation"){
                if (count==0){
                    if (pattern!="" && !searching){
                        statusText.text = "No location found for " + pattern;
                        stateMachine.state = "error";
                    }

                    console.log("empty!");
                    return;
                }
                console.log("NOT empty!");

                console.log("First location: " + locationStr(locationSearchModel.get(0)));
                if (stateMachine.state=="resolvingFromLocation"){
                    routeFromLocation = locationSearchModel.get(0);
                    stateMachine.state = "resolvingToLocation";
                } else if (stateMachine.state=="resolvingToLocation"){
                    routeToLocation = locationSearchModel.get(0);
                    stateMachine.state = "routing";
                }
            }
        }
    }

    RoutingListModel {
        id: routingModel
        onReadyChanged: {
            if (ready){
                var routeWay = routingModel.routeWay;
                if (routeWay==null){
                    statusText.text = "No route way!";
                    stateMachine.state = "error";
                    return;
                }

                stateMachine.state = "computingElevation";
                //elevationChart.way = routingModel.routeWay;
            }
        }
        onRouteFailed: {
            statusText.text = "Failed to compute reoute: " + reason;
            stateMachine.state = "error";
        }
    }

    ElevationChart {
        id: elevationChart
        anchors{
            fill: parent
        }

        way: routingModel.routeWay

        onLoadingChanged: {
            if (!loading && stateMachine.state == "computingElevation"){
                stateMachine.state = "done";
            }
        }
    }

    Text {
        id: statusText
        anchors{
            fill: parent
        }

        Layout.fillWidth: true

        text: "Loading..."
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
