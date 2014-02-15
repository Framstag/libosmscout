import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: dialog

    Rectangle {
        id: content

        width: mainFrame.implicitWidth+20

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 10

        ColumnLayout {
            id: mainFrame

            anchors.margins: 10
            anchors.fill: parent

            GridLayout {
                anchors.left: parent.left;
                anchors.right: parent.right;

                columns: 2

                Text {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                    text: "Start:"
                }

                LocationEdit {
                    id: startInput
                    width: 250

                    horizontalAlignment: TextInput.AlignLeft
                }

                Text {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                    text: "Target:"
                }

                LocationEdit {
                    id: targetInput
                    width: 250

                    horizontalAlignment: TextInput.AlignLeft
                }
            }

            RowLayout {
                id: buttonRow
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 10

                Item {
                    Layout.fillWidth: true
                    implicitWidth: 0
                    width: 1
                }

                DialogActionButton {
                    id: route
                    width: 50
                    height: 25
                    text: "Route"

                    onClicked: {
                        startInput.enforceLocationValue()
                        targetInput.enforceLocationValue()

                        if (startInput.location && targetInput.location) {
                            routingModel.setStartAndTarget(startInput.location,
                                                           targetInput.location)
                        }
                    }
                }

                DialogActionButton {
                    id: close
                    width: 50
                    height: 25
                    text: "Close"

                    onClicked: {
                        dialog.close()
                    }
                }
            }

            Rectangle {
                height: 100
                width: 100

                anchors.left: parent.left
                anchors.right: parent.right

                Layout.fillHeight: true

                border.color: "lightgrey"
                border.width: 1

                RoutingListModel {
                    id: routingModel
                }

                ListView {
                    id: routeView

                    model: routingModel

                    anchors.fill: parent

                    clip: true

                    delegate: Component {
                        Item {
                            height: text.implicitHeight+4

                            Text {
                                id: text
                                anchors.fill: parent
                                text: label
                            }
                        }
                    }
                }

                ScrollIndicator {
                    flickableArea: routeView
                }
            }
        }
    }
}
