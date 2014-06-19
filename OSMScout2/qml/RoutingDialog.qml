import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: dialog

    fullscreen: true
    label: "Route..."

    content : ColumnLayout {
        id: mainFrame

        Layout.fillWidth: true
        Layout.fillHeight: true

        GridLayout {
            Layout.fillWidth: true

            columns: 2

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                text: "Start:"
                font.pixelSize: Theme.textFontSize
            }

            LocationEdit {
                id: startInput

                Layout.minimumWidth: Theme.averageCharWidth*20
                Layout.preferredWidth: Theme.averageCharWidth*40
                Layout.maximumWidth: Theme.averageCharWidth*60

                horizontalAlignment: TextInput.AlignLeft
            }

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                text: "Target:"
                font.pixelSize: Theme.textFontSize
            }

            LocationEdit {
                id: targetInput

                Layout.minimumWidth: Theme.averageCharWidth*20
                Layout.preferredWidth: Theme.averageCharWidth*40
                Layout.maximumWidth: Theme.averageCharWidth*60

                horizontalAlignment: TextInput.AlignLeft
            }
        }

        RowLayout {
            id: buttonRow
            Layout.fillWidth: true
            spacing: 10

            Item {
                Layout.fillWidth: true
                implicitWidth: 0
                width: 1
            }

            DialogActionButton {
                id: route
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
                text: "Close"

                onClicked: {
                    dialog.close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
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
                flickableArea: routeView
            }
        }
    }
}
