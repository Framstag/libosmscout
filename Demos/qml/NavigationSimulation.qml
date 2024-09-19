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
    title: "OSMScout navigation simulation"
    visible: true
    width: 1024
    height: 768

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
                var startLoc = routingModel.locationEntryFromPosition(fromLat, fromLon);
                var destinationLoc = routingModel.locationEntryFromPosition(simulator.endLat, simulator.endLon);
                console.log("We leave route, reroute from " + Utils.locationStr(startLoc) + " -> " + Utils.locationStr(destinationLoc) + ", bearing: " + bearingAngle);
                routingModel.setStartAndTarget(startLoc, destinationLoc, "car", bearingAngle);
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
            text: "reset"
            Layout.preferredWidth: Theme.mapButtonHeight *3
            onClicked: {
                simulator.track=PositionSimulationTrack;
            }
        }
        MapButton{
            text: simulator.running ? "pause" : "resume"
            Layout.preferredWidth: Theme.mapButtonHeight *3
            onClicked: {
                simulator.running=!simulator.running
            }
        }
        MapButton{
            text: "+ 10s"
            Layout.preferredWidth: Theme.mapButtonHeight *3
            onClicked: {
                simulator.skipTime(10 * 1000);
            }
        }
        MapButton{
            text: "+ minute"
            Layout.preferredWidth: Theme.mapButtonHeight *3
            onClicked: {
                simulator.skipTime(60 * 1000);
            }
        }
        MapButton{
            text: "+ hour"
            Layout.preferredWidth: Theme.mapButtonHeight *3
            onClicked: {
                simulator.skipTime(3600 * 1000);
            }
        }
    }

    Rectangle {

        x: Theme.horizSpace
        y: simulatorControl.y-height-Theme.vertSpace
        width: voiceControl.width
        height: voiceControl.height

        border.color: "lightgrey"
        border.width: 1
        color: "white"


        RowLayout {
            id: voiceControl

            spacing: Theme.mapButtonSpace

            Text{
                font.pixelSize: Theme.textFontSize
                text: "Voice:"
            }

            ComboBox {
                id: voiceComboBox
                editable: true

                property bool initialized: false

                function getData(row, role){
                    return voiceModel.data(voiceModel.index(row, 0), role);
                }

                function update(){
                    initialized = false;

                    var voices = [];
                    for (var row = 0; row < voiceModel.rowCount(); row++){
                        if (!getData(row, InstalledVoicesModel.ValidRole)){
                            voices.push("No voice");
                        } else {
                            var voice = getData(row, InstalledVoicesModel.LangRole) + " - " + getData(row, InstalledVoicesModel.NameRole);
                            console.log("voice[" + row + "] = " + voice);
                            voices.push(voice);
                        }
                    }
                    voiceComboBox.model = voices;

                    for (var row = 0; row < voiceModel.rowCount(); row++){
                        if (getData(row, InstalledVoicesModel.SelectedRole)){
                            currentIndex = row;
                            break;
                        }
                    }
                    initialized = true;
                }

                InstalledVoicesModel{
                    id: voiceModel

                    onVoiceChanged: {
                        voiceComboBox.update();
                    }
                    onRowsInserted: {
                        voiceComboBox.update();
                    }
                    onRowsRemoved: {
                        voiceComboBox.update();
                    }
                    onModelReset: {
                        voiceComboBox.update();
                    }
                }
                Component.onCompleted: {
                    update();
                }
                onCurrentIndexChanged: {
                    if (!initialized){
                        return;
                    }
                    console.log("Set voice to "
                                + getData(currentIndex, InstalledVoicesModel.LangRole) + " - "
                                + getData(currentIndex, InstalledVoicesModel.NameRole));
                    var indexObj = voiceModel.index(currentIndex, 0);
                    voiceModel.select(indexObj);
                }
            }

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

                function openVoiceDownloadDialog() {
                    var component = Qt.createComponent("VoiceDownloadDialog.qml")
                    var dialog = component.createObject(mainWindow, {})

                    //dialog.opened.connect(onDialogOpened)
                    //dialog.closed.connect(onDialogClosed)
                    dialog.open()
                }

                onClicked: {
                    openVoiceDownloadDialog();
                }
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
            Text {
                id: destinationsText
                visible: navigationModel.nextRouteStep.destinations.length > 0
                opacity: 0.7
                text: qsTr("Destinations: %1").arg(navigationModel.nextRouteStep.destinations.join(", "))
                font.pixelSize: Theme.textFontSize
                wrapMode: Text.NoWrap
                clip: true
                anchors{
                    top: nextStepDescription.bottom
                    left: nextStepIcon.right
                    right: parent.right
                }
            }
        }

        LaneTurns {
            id: laneTurnsComponent

            laneTurns: navigationModel.laneTurns
            visible: navigationModel.laneSuggested
            suggestedLaneFrom: navigationModel.suggestedLaneFrom
            suggestedLaneTo: navigationModel.suggestedLaneTo

            color: "white"
            border.color: "lightgrey"
            border.width: 1
            height: Math.max(60, nextStepBox.height * 0.5)

            anchors{
                top: nextStepBox.top
                left: nextStepBox.right
                margins: Theme.horizSpace
            }
            //maxWidth: parent.width - (speedIndicator.width + menuBtn.width + 4*Theme.paddingMedium)
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
