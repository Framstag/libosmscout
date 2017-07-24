import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

FocusScope {
    id: searchDialog

    property Item desktop;

    property alias startLocation: searchEdit.location;
    property alias destinationLocation: destinationEdit.location;

    signal showLocation(LocationEntry location)

    width: searchRectangle.width
    height: searchRectangle.height

    state: "NORMAL"

    onFocusChanged: {
        if (focus) {
            state = "SEARCH"
        }
        else {
            state = "NORMAL"
        }
    }

    function route() {
        if (startLocation !== null && destinationLocation !== null) {
            routingModel.setStartAndTarget(startLocation,
                                           destinationLocation)
        }
        else {
            routingModel.clear()
        }
    }

    function showRoutingResult() {
        overlay.parent = desktop
        overlay.visible = true

        var mappedPosition = desktop.mapFromItem(searchDialog, 0, 0)
        var desktopFreeSpace = desktop.getFreeRect()

        routingPopup.x = mappedPosition.x
        routingPopup.y = desktopFreeSpace.y

        var popupHeight

        if (routingView.count === 0) {
            popupHeight = Theme.mapButtonFontSize
        }
        else {
            popupHeight = routingView.contentHeight+2
        }

        if (popupHeight > desktopFreeSpace.height) {
            popupHeight = desktopFreeSpace.height
        }

        routingBox.width = searchDialog.width
        routingBox.height = popupHeight

        routingPopup.parent = desktop
        routingPopup.visible = true

        map.addOverlayWay(0,routingModel.routeWay);
        console.log("Show routing result; dialog height: "+routingBox.height);
    }

    Rectangle {
        id: searchRectangle;

        width: searchContent.width+4;
        height: searchContent.height+4;

        GridLayout {
            id: searchContent

            x: 2
            y: 2

            columns: 2
            columnSpacing: 0

            LocationSearch {
                id: searchEdit;

                focus: true

                Layout.column: 1
                Layout.row: 1
                Layout.fillWidth: true
                Layout.minimumWidth: Theme.averageCharWidth*5
                Layout.preferredWidth: Theme.averageCharWidth*45
                Layout.maximumWidth: Theme.averageCharWidth*60

                desktop: searchDialog.desktop

                onShowLocation: {
                    if (searchDialog.state === "SEARCH") {
                        searchDialog.showLocation(startLocation)
                        focus=false;
                    }
                    else if (searchDialog.state == "ROUTE") {
                        // TODO: Store as startLocation
                    }
                }
            }

            DialogActionButton {
                id: stateToggle

                visible: false

                Layout.column: 2
                Layout.row: 1

                width: searchEdit.height
                height: searchEdit.height

                Image {
                    id: stateToggleImage

                    width: parent.height
                    height: parent.height

                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.width: parent.height
                    sourceSize.height: parent.height
                }

                onClicked: {
                    console.log("State change...")
                    if (searchDialog.state === "SEARCH") {
                        searchDialog.state = "ROUTE"
                    }
                    else if (searchDialog.state === "ROUTE") {
                        searchDialog.state = "SEARCH"
                    }
                }
            }

            LocationSearch {
                id: destinationEdit;

                visible: false

                Layout.column: 1
                Layout.row: 2
                Layout.fillWidth: true
                Layout.minimumWidth: Theme.averageCharWidth*5
                Layout.preferredWidth: Theme.averageCharWidth*45
                Layout.maximumWidth: Theme.averageCharWidth*60

                desktop: searchDialog.desktop

                onShowLocation: {
                    console.log("Routing...")
                    route()
                    showRoutingResult()
                }
            }

            DialogActionButton {
                id: routeButton

                visible: false

                Layout.column: 2
                Layout.row: 2

                width: searchEdit.height
                height: searchEdit.height

                Image {
                    id: routeButtonImage

                    width: parent.height
                    height: parent.height

                    source: "qrc:///pics/Route.svg"
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.width: parent.height
                    sourceSize.height: parent.height
                }

                onClicked: {
                    console.log("Routing...")
                    route()
                    showRoutingResult()
                }
            }
        }
    }

    MouseArea {
        id: overlay

        visible: false
        z: 1

        anchors.fill: parent

        onClicked: {
            overlay.visible = false
            routingPopup.visible = false
        }
    }

    Item {
        id: routingPopup

        visible: false
        z: 2

        width: routingBox.width
        height: routingBox.height

        Rectangle {
            id: routingBox

            border.color: "lightgrey"
            border.width: 1

            RoutingListModel {
                id: routingModel
                onReadyChanged: {
                  showRoutingResult();
                }
            }

            ListView {
                id: routingView

                model: routingModel

                anchors.fill: parent
                anchors.margins: 1
                clip: true

                delegate: Item {
                    id: item

                    anchors.right: parent.right;
                    anchors.left: parent.left;
                    height: text.implicitHeight+5

                    Text {
                        id: text

                        y:2
                        x: 2
                        width: parent.width-4
                        text: label
                        font.pixelSize: Theme.textFontSize
                    }

                    Rectangle {
                        x: 2
                        y: parent.height-2
                        width: parent.width-4
                        height: 1
                        color: "lightgrey"
                    }
                }
            }

            ScrollIndicator {
                flickableArea: routingView
            }
        }
    }


    states: [
        State {
          name: "NORMAL"
          PropertyChanges { target: stateToggle; visible: false }
          PropertyChanges { target: destinationEdit; visible: false }
          PropertyChanges { target: routeButton; visible: false }
        },
        State {
            name: "SEARCH"
            PropertyChanges { target: stateToggleImage; source: "qrc:///pics/Plus.svg" }
            PropertyChanges { target: stateToggle; visible: true }
            PropertyChanges { target: destinationEdit; visible: false }
            PropertyChanges { target: routeButton; visible: false }
        },
        State {
            name: "ROUTE"
            PropertyChanges { target: stateToggleImage; source: "qrc:///pics/Minus.svg" }
            PropertyChanges { target: stateToggle; visible: true }
            PropertyChanges { target: destinationEdit; visible: true }
            PropertyChanges { target: routeButton; visible: true }
        }
    ]
}
