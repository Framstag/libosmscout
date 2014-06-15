import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: dialog

    fullscreen: true
    label: "Search location..."

    property Location location

    onLocationChanged: {
        if (location == null) {
            locationInput.backgroundColor = locationInput.defaultBackgroundColor
        }
        else {
            locationInput.backgroundColor = "#ddffdd"
        }
    }

    function updateSuggestions() {
        suggestionModel.setPattern(locationInput.text)

        console.log("Suggestion count: " + suggestionModel.count)

        if (suggestionModel.count>=1 &&
            locationInput.text==suggestionModel.get(0).name) {
            location=suggestionModel.get(0)
        }
    }

    function selectSuggestion(index) {
        var location = suggestionModel.get(index)

        dialog.location = location

        locationInput.text = location.name

        showLocation(location)
    }

    content : ColumnLayout {
        id: mainFrame

        Layout.alignment: Qt.AlignTop

        LineEdit {
            id: locationInput

            Layout.minimumWidth: Theme.averageCharWidth*40
            Layout.preferredWidth: Theme.averageCharWidth*60
            Layout.maximumWidth: Theme.averageCharWidth*100

            horizontalAlignment: TextInput.AlignLeft

            Timer {
                id: suggestionTimer
                interval: 1000
                repeat: false

                onTriggered: {
                    updateSuggestions()
                }
            }

            onTextChanged: {
                location = null;
                suggestionTimer.restart()
            }

            Keys.onUpPressed: {
                suggestionView.decrementCurrentIndex()
            }

            Keys.onDownPressed: {
                suggestionView.incrementCurrentIndex()
            }

            Keys.onReturnPressed: {
                if (suggestionView.currentIndex>=0) {
                    selectSuggestion(suggestionView.currentIndex)
                    close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            border.color: "lightgrey"
            border.width: 1

            LocationListModel {
                id: suggestionModel
            }

            ListView {
                id: suggestionView

                model: suggestionModel

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
                    }

                    Rectangle {
                        x: 2
                        y: parent.height-2
                        width: parent.width-4
                        height: 1
                        color: "lightgrey"
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            suggestionView.currentIndex = index;

                            var location = suggestionModel.get(index)

                            locationInput.text = location.name
                            dialog.location = location
                        }

                        onDoubleClicked: {
                            suggestionView.currentIndex = index;

                            selectSuggestion(index)
                            close()
                        }
                    }
                }

                highlight: Rectangle {
                        color: "lightblue"
                }
            }

            ScrollIndicator {
                flickableArea: suggestionView
            }
        }

        RowLayout {
            id: buttonRow

            Layout.fillWidth: true

            spacing: 10

            Item {
                Layout.fillWidth: true
            }

            DialogActionButton {
                id: ok
                text: "OK"

                onClicked: {
                    if (location != null) {
                        showLocation(location)
                    }

                    close()
                }
            }
        }
    }

    signal showLocation(Location location)

    onOpened: {
        locationInput.focus = true
    }
}
