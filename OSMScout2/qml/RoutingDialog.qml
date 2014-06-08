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
            }

            LocationEdit {
                id: startInput

                Layout.minimumWidth: Theme.averageCharWidth*40
                Layout.preferredWidth: Theme.averageCharWidth*60
                Layout.maximumWidth: Theme.averageCharWidth*100

                horizontalAlignment: TextInput.AlignLeft
            }

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                text: "Target:"
            }

            LocationEdit {
                id: targetInput

                Layout.minimumWidth: Theme.averageCharWidth*40
                Layout.preferredWidth: Theme.averageCharWidth*60
                Layout.maximumWidth: Theme.averageCharWidth*100

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
            /*
            Layout.minimumHeight: 100//height*3
            Layout.preferredHeight: 5000//text.height*30*/
            height: 10
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

                delegate: Component {
                    Item {
                        id: item

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
