import QtQuick 2.2

import net.sf.libosmscout.map 1.0

LineEdit {
    id: locationEdit
    z: 100

    property LocationEntry location;

    function updateSuggestions() {
        suggestionModel.setPattern(locationEdit.text)

        console.log("Suggestion count: " + suggestionModel.count)

        if (suggestionModel.count>=1 &&
            locationEdit.text === suggestionModel.get(0).label) {
            location=suggestionModel.get(0)

            return
        }

        if (suggestionView.count>0) {
            // Set size of popup content
            if (suggestionView.count>10) {
                suggestionBox.height=10*locationEdit.height+2*suggestionBox.border.width
            }
            else {
                suggestionBox.height=suggestionView.count*locationEdit.height+2*suggestionBox.border.width
            }

            popup.showPopup()
        }
        else {
            popup.visible=false
        }
    }

    function enforceLocationValue() {
        if (location != null) {
            return;
        }

        suggestionModel.setPattern(locationEdit.text)

        if (suggestionView.count>0) {
            location = suggestionModel.get(0)
            console.log("Enforced location: " + location.label)
        }
    }

    onTextChanged: {
        location = null;

        suggestionTimer.restart()
    }

    onLocationChanged: {
        console.log("Change")
        if (location == null) {
            locationEdit.backgroundColor = locationEdit.defaultBackgroundColor
        }
        else {
            locationEdit.backgroundColor = "#ddffdd"
        }
    }

    onFocusChanged: {
        if (!focus) {
            suggestionTimer.stop()

            if (location == null) {
                // Enforce location on focus out, if we do not have a location
                enforceLocationValue()
            }
        }
    }

    Keys.onReturnPressed: {
        suggestionTimer.stop()

        if (popup.visible) {
            var index = suggestionView.currentIndex
            var location = suggestionModel.get(index)

            locationEdit.location = location
            if (location) {
              locationEdit.text = location.label
            }
            else {
                locationEdit.text = ""
            }

            popup.visible = false

            suggestionModel.setPattern(locationEdit.text)
        }
    }

    Keys.onEscapePressed: {
        popup.visible = false
    }

    Keys.onUpPressed: {
        if (popup.visible) {
            suggestionView.decrementCurrentIndex()
        }
    }

    Keys.onDownPressed: {
        suggestionTimer.stop()
        if (popup.visible) {
            suggestionView.incrementCurrentIndex()
        }
        else {
            updateSuggestions()
        }
    }

    Timer {
        id: suggestionTimer
        interval: 1000
        repeat: false

        onTriggered: {
            updateSuggestions()
        }
    }

    LocationListModel {
        id: suggestionModel

        onCountChanged:{
            updateSuggestions();
        }
    }

    Item {
        id: popup
        visible: false

        function showPopup() {
            var root = findRoot()

            parent = root
            anchors.fill = parent

            var locationEditXY=mapFromItem(locationEdit,0,locationEdit.height)

            suggestionBox.x = locationEditXY.x
            suggestionBox.y = locationEditXY.y

            // If nothing is selected, select first line
            if (suggestionView.currentIndex < 0 || suggestionView.currentIndex >= suggestionView.count) {
                suggestionView.currentIndex = 0
            }

            popup.visible = true
        }

        function findRoot() {
            var p = locationEdit

            while (p.objectName !== "main" && p.parent) {
                p = p.parent
            }

            return p
        }

        MouseArea {
            id: overlay
            anchors.fill: parent

            onClicked: {
                popup.visible = false
            }
        }

        Rectangle {
            id: suggestionBox

            width: locationEdit.width

            border.color: locationEdit.focusColor
            border.width: 1

            ListView {
                id: suggestionView

                anchors.fill: parent
                anchors.margins: 1
                clip: true

                model: suggestionModel

                delegate: Component {
                    Item {
                        height: text.implicitHeight+4
                        width: suggestionView.width

                        Text {
                            id: text
                            anchors.fill: parent
                            text: label
                        }

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                suggestionView.currentIndex = index;

                                var location = suggestionModel.get(index)

                                locationEdit.text = location.label
                                locationEdit.location = location

                                popup.visible = false
                            }
                        }
                    }
               }

               highlight: Component {
                   id: highlightBar

                   Rectangle {
                       height: locationEdit.height
                       width: suggestionView.width

                       color: "lightblue"
                   }
               }
            }

            ScrollIndicator {
                flickableArea: suggestionView
            }
        }
    }
}
