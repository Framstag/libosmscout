import QtQuick 2.3
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

LineEdit {
    id: searchEdit;

    focus: true

    property Item desktop;
    property rect desktopFreeSpace;
    property Location location;

    // Internal constant
    property int listCellHeight: Theme.textFontSize*2+4

    signal showLocation(Location location)

    function updateSuggestions() {
        console.log("Updating suggestions...");

        suggestionModel.setPattern(searchEdit.text)

        console.log("Suggestion count: " + suggestionModel.count)

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
        console.log("Focus gained")
        updatePopup()
    }

    function handleFocusLost() {
        console.log("Focus lost")
        suggestionTimer.stop()

        hidePopup()
    }

    function handleOK()  {
        console.log("handleOK")
        suggestionTimer.stop()

        if (popup.visible) {
            console.log("Selected from popup...")
            var index = suggestionView.currentIndex
            var selectedLocation = suggestionModel.get(index)

            console.log("Selected " + selectedLocation + " / " + selectedLocation.name)

            // the else case should never happen
            if (selectedLocation !== null) {
                searchEdit.location = selectedLocation
                searchEdit.text = selectedLocation.name
                console.log("Signaling selection of '"+selectedLocation.name+"'")
                showLocation(searchEdit.location)
                nextItemInFocusChain(true)

                hidePopup()

                suggestionModel.setPattern(searchEdit.text)
            }

        }
        else if (searchEdit.location !== null) {
            console.log("Selected from LineEdit...")
            console.log("Signaling selection of '"+searchEdit.location.name+"'")
            showLocation(searchEdit.location)
            nextItemInFocusChain(true)
        }
    }

    function handleCancel() {
        hidePopup();
        nextItemInFocusChain(true)
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
        console.log("Free space: "+desktopFreeSpace.x+","+desktopFreeSpace.y +" "+desktopFreeSpace.width+"x"+desktopFreeSpace.height)

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
        console.log("Focus changed")
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
        if (location !== null) {
            console.log("Location changed to " + location + " / " + location.name)
        }
        else {
            console.log("Location changed to null")
        }

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

                delegate: Component {
                    Item {
                        width: suggestionView.width
                        height: listCellHeight

                        Text {
                            id: text

                            anchors.fill: parent

                            text: label
                            font.pixelSize: Theme.textFontSize
                        }

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                suggestionView.currentIndex = index;

                                var location = suggestionModel.get(index)

                                searchEdit.text = location.name
                                searchEdit.location = location

                                hidePopup()

                                showLocation(location)
                            }
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


