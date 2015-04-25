import QtQuick 2.3
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

LineEdit {
    id: searchEdit;

    property Item desktop;
    property rect desktopFreeSpace;
    property Location location;

    // Internal constant
    property int listCellHeight: Theme.textFontSize*2+4

    signal showLocation(Location location)

    // Public

    function enforceLocationValue() {
        if (location != null) {
            return;
        }

        suggestionModel.setPattern(searchEdit.text)

        if (suggestionModel.count>0) {
            location=suggestionModel.get(0)
            searchEdit.text = location.name
        }

        hidePopup()
    }

    // Private

    function updateSuggestions() {
        suggestionModel.setPattern(searchEdit.text)

        if (suggestionModel.count>=1 &&
            searchEdit.text===suggestionModel.get(0).name) {
            location=suggestionModel.get(0)
        }

        updatePopup()
    }

    function updatePopup() {
        if (suggestionView.count>0) {
            /*
            // Set size of popup content

            console.log("Updating suggestionBox size:");
            console.log("  Number of entries: " + suggestionView.count);
            console.log("  Height of one entry: " + listCellHeight);

            var visibleEntries = suggestionView.count;

            if (visibleEntries > 10) {
                visibleEntries = 10;
            }

            console.log("  Visible entries: " + visibleEntries);

            var resultingHeight = visibleEntries*listCellHeight+2*suggestionBox.border.width

            console.log("  Resulting height: " + resultingHeight);

            suggestionBox.height = resultingHeight*/

            showPopup()
        }
        else {
            hidePopup();
        }
    }

    function handleTextChanged() {
        // If the text set is equal to the location, do NOT erase the location
        if (location !=null && location.name !== text) {
            location = null;
        }

        // If the text set is equal to the location, do NOT trigger an suggestion update on timer
        if (location === null || location.name !== text) {
            suggestionTimer.restart()
        }
    }

    function handleFocusGained() {
        updatePopup()
    }

    function handleFocusLost() {
        suggestionTimer.stop()

        hidePopup()
    }

    function delegateFocus() {
        desktop.focus = true
    }

    function selectLocation(selectedLocation) {
        searchEdit.location = selectedLocation
        searchEdit.text = selectedLocation.name
        showLocation(searchEdit.location)
        delegateFocus();

        hidePopup()

        suggestionModel.setPattern(searchEdit.text)
    }

    function handleOK()  {
        suggestionTimer.stop()

        if (popup.visible) {
            var index = suggestionView.currentIndex
            var selectedLocation = suggestionModel.get(index)

            // the else case should never happen
            if (selectedLocation !== null) {
                selectLocation(selectedLocation);
            }

        }
        else if (searchEdit.location !== null) {
            showLocation(searchEdit.location)
            delegateFocus();
        }
    }

    function handleCancel() {
        hidePopup();
        delegateFocus();
    }

    function gotoPrevious() {
        suggestionTimer.stop()

        if (popup.visible) {
            suggestionView.decrementCurrentIndex()
        }
    }

    function gotoNext() {
        suggestionTimer.stop()

        if (popup.visible) {
            suggestionView.incrementCurrentIndex()
        }
        else {
            updateSuggestions()
        }
    }

    function showPopup() {
        popup.x = desktopFreeSpace.x;
        popup.y = desktopFreeSpace.y;
        suggestionBox.width = desktopFreeSpace.width;
        suggestionBox.height = desktopFreeSpace.height;

        // If nothing is selected, select first line
        if (suggestionView.currentIndex < 0 || suggestionView.currentIndex >= suggestionView.count) {
            suggestionView.currentIndex = 0
        }

        popup.parent = desktop
        popup.visible = true
    }

    function hidePopup() {
        popup.visible = false
    }

    onTextChanged: {
        handleTextChanged();
    }

    onFocusChanged: {
        if (focus) {
            handleFocusGained();
        }
        else {
            handleFocusLost();
        }
    }

    Keys.onReturnPressed: {
        handleOK();
    }

    Keys.onEscapePressed: {
        handleCancel();
    }

    Keys.onUpPressed: {
        gotoPrevious();
    }

    Keys.onDownPressed: {
        gotoNext();
    }

    onLocationChanged: {
        if (location == null) {
            searchEdit.backgroundColor = searchEdit.defaultBackgroundColor
        }
        else {
            searchEdit.backgroundColor = "#ddffdd"
        }
    }

    Timer {
        id: suggestionTimer
        interval: 1000
        repeat: false

        onTriggered: {
            updateSuggestions();
        }
    }

    LocationListModel {
        id: suggestionModel
    }

    Item {
        id: popup
        visible: false
        z: 1

        width: suggestionBox.width
        height: suggestionBox.height

        /*
        MouseArea {
            id: overlay
            anchors.fill: popup.parent

            onClicked: {
                popup.visible = false
            }
        }*/

        Rectangle {
            id: suggestionBox

            border.color: searchEdit.focusColor
            border.width: 1

            ListView {
                id: suggestionView

                anchors.fill: parent
                anchors.margins: 1
                clip: true

                model: suggestionModel

                delegate: Text {
                    id: text

                    width: suggestionView.width

                    text: label
                    font.pixelSize: Theme.textFontSize

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            suggestionView.currentIndex = index;

                            var selectedLocation = suggestionModel.get(index)

                            selectLocation(selectedLocation);
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
    }
}


