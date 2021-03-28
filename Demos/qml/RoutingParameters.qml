import QtQuick 2.12

import QtQuick.Controls 1.4
import QtQuick.Layouts 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Window 2.12

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
            Layout.minimumWidth: 350

            Column {
                id: routingControlColumn
                anchors{
                    top: parent.top
                    right: parent.right
                    left: parent.left
                    margins: 6
                }

                GridLayout {
                    id: routingControl
                    columns: 2
                    columnSpacing: 6
                    rowSpacing: 6
                    width: parent.width

                    property LocationEntry routeFromLocation
                    property LocationEntry routeToLocation
                    property RoutingProfile routingProfile: RoutingProfile{}

                    Timer {
                        id: rerouteTimer
                        interval: 500
                        running: false
                        repeat: false
                        onTriggered: {
                            routingControl.reroute();
                        }
                    }

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


                        var speedTable = {};
                        var speedTableLines=speedTableTextArea.text.split(/[\r\n]+/);
                        for (var i in speedTableLines){
                            var line=speedTableLines[i];
                            if (line!=""){
                                var arr=line.split(/\s*=\s*/);
                                if (arr.length==2){
                                    speedTable[arr[0]] = arr[1];
                                    console.log(arr[0] + ": " + arr[1]);
                                } else {
                                    console.log("Invalid line: " + line);
                                }
                            }
                        }
                        console.log("speedTable type " + (typeof speedTable));
                        routingProfile.speedTable = speedTable;

                        routingProfile.maxSpeed = maxVehicleSpeedField.text;
                        routingProfile.penaltySameType = penaltySameTypeField.text;
                        routingProfile.penaltyDifferentType = penaltyDifferentTypesField.text;
                        routingProfile.maxPenalty = maximumPenaltyField.text;

                        console.log("Route from " + locationStr(routeFromLocation) + " -> " + locationStr(routeToLocation) + " with " + routingProfile);
                        routingModel.setStartAndTarget(routeFromLocation, routeToLocation, routingProfile);
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

                    Text{}

                    Text {
                        id: routingState
                        text: qsTr("No start or target set")
                    }

                    Text {
                        text: qsTr("Vehicle:")
                    }
                    ComboBox {
                        id: vehicleComboBox
                        width: 200
                        style: ComboBoxStyle {
                            textColor: "black"
                        }

                        model: [ "Car", "Bicycle", "Foot" ]
                        onCurrentIndexChanged: {
                            if (currentIndex==0){
                                routingControl.routingProfile.vehicle=RoutingProfile.CarVehicle;
                            } else if (currentIndex==1) {
                                routingControl.routingProfile.vehicle=RoutingProfile.BicycleVehicle;
                            } else if (currentIndex==2) {
                                routingControl.routingProfile.vehicle=RoutingProfile.FootVehicle;
                            }

                            //routingControl.reroute();
                            rerouteTimer.restart(); // need to update route table before reroute
                        }
                    }

                    Text{
                        text: qsTr("Max speed [km/h]:")
                    }
                    TextField {
                        id: maxVehicleSpeedField
                        Layout.fillWidth: true
                        text: routingControl.routingProfile.maxSpeed
                        textColor: "black"
                        onTextChanged: {
                            rerouteTimer.restart();
                        }
                    }

                    Text{}
                    CheckBox {
                        id: junctionPenaltyCheckBox
                        checked: routingControl.routingProfile.applyJunctionPenalty
                        text: qsTr("Junction penalty")
                        style: CheckBoxStyle {
                            label: Text{
                                color: "black"
                                text: junctionPenaltyCheckBox.text
                            }
                        }
                        onCheckedChanged: {
                            routingControl.routingProfile.applyJunctionPenalty = checked
                            routingControl.reroute();
                        }
                    }

                    Text{
                        text: qsTr("Penalty, same type [m]:")
                    }
                    TextField {
                        id: penaltySameTypeField
                        Layout.fillWidth: true
                        text: routingControl.routingProfile.penaltySameType
                        textColor: enabled ? "black" : "grey"
                        enabled: routingControl.routingProfile.applyJunctionPenalty
                        onTextChanged: {
                            rerouteTimer.restart();
                        }
                    }

                    Text{
                        text: qsTr("Penalty, different types [m]:")
                    }
                    TextField {
                        id: penaltyDifferentTypesField
                        Layout.fillWidth: true
                        text: routingControl.routingProfile.penaltyDifferentType
                        textColor: enabled ? "black" : "grey"
                        enabled: routingControl.routingProfile.applyJunctionPenalty
                        onTextChanged: {
                            rerouteTimer.restart();
                        }
                    }

                    Text{
                        text: qsTr("Maximum penalty [s]:")
                    }
                    TextField {
                        id: maximumPenaltyField
                        Layout.fillWidth: true
                        text: routingControl.routingProfile.maxPenalty
                        textColor: enabled ? "black" : "grey"
                        enabled: routingControl.routingProfile.applyJunctionPenalty
                        onTextChanged: {
                            rerouteTimer.restart();
                        }
                    }

                    Text{
                        text: qsTr("Route type speed table:")
                    }
                    Text{}
                }


                TextArea {
                    id: speedTableTextArea
                    height: 160
                    width: parent.width

                    backgroundVisible: false
                    textColor: "black"

                    function updateTable(){
                        speedTableTextArea.text="";
                        var table=routingControl.routingProfile.speedTable;
                        for (var type in table){
                            speedTableTextArea.text += type + "=" + table[type] + "\n";
                        }
                    }

                    onTextChanged: {
                        rerouteTimer.restart();
                    }

                    Component.onCompleted: updateTable()

                    Connections {
                        target: routingControl.routingProfile
                        onSpeedTableChanged: {
                            speedTableTextArea.updateTable();
                        }
                    }
                }


                Button {
                    id: rerouteButton
                    text: qsTr("Reroute")
                    onClicked: routingControl.reroute()
                    style: ButtonStyle {
                        label: Text{
                            color: "black"
                            text: rerouteButton.text
                        }
                    }
                }

                GridLayout {
                    id: routingInfo
                    columns: 2
                    columnSpacing: 6
                    rowSpacing: 6

                    Text {
                        text: qsTr("Distance")
                    }
                    Text {
                        text: routingModel.ready ? Utils.humanDistance(routingModel.length) : "?"
                    }
                    Text {
                        text: qsTr("Duration")
                    }
                    Text {
                        text: routingModel.ready ? Utils.humanDuration(routingModel.duration) : "?"
                    }
                }
            }


            Rectangle {
                id: routingStepsBox
                color: "transparent"

                anchors{
                    top: routingControlColumn.bottom
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
