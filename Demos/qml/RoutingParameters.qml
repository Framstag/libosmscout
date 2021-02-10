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

    function reroute(){
        var startLoc = routingModel.locationEntryFromPosition(simulator.latitude, simulator.longitude);
        var destinationLoc = routingModel.locationEntryFromPosition(simulator.endLat, simulator.endLon);
        console.log("We leave route, reroute from " + Utils.locationStr(startLoc) + " -> " + Utils.locationStr(destinationLoc));
        routingModel.setStartAndTarget(startLoc, destinationLoc);
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
        focus: true
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
