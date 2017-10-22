import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

LineEdit {
    id: searchEdit;

    property Item desktop;
    property LocationEntry location;
    property double searchCenterLat;
    property double searchCenterLon;

    // Internal constant
    property int listCellHeight: Theme.textFontSize*2+4

    signal showLocation(LocationEntry location)

    // Public

    function enforceLocationValue() {
        if (location != null) {
            return;
        }

        setupSearchCenter();
        suggestionModel.setPattern(searchEdit.text)

        if (suggestionModel.count>=1) {
            if (searchEdit.text===suggestionModel.get(0).label) {
              location=suggestionModel.get(0)
              hidePopup()
            }
            else  {
              showPopup()
            }
        }
        else {
            hidePopup()
        }
    }

    // Private

    function setupSearchCenter() {
        suggestionModel.lat=searchCenterLat;
        suggestionModel.lon=searchCenterLon;
    }

    function updateSuggestions() {
        setupSearchCenter();
        suggestionModel.setPattern(searchEdit.text)

        if (suggestionModel.count>=1 &&
            searchEdit.text===suggestionModel.get(0).label) {
            location=suggestionModel.get(0)
        }

        updatePopup()
    }

    function updatePopup() {
        if (suggestionModel.count>0) {
            showPopup()
        }
        else {
            hidePopup();
        }
    }

    function handleTextChanged() {
        // If the text set is equal to the location, do NOT erase the location
        if (location !=null && location.label !== text) {
            location = null;
        }

        // If the text set is equal to the location, do NOT trigger an suggestion update on timer
        if (location === null || location.label !== text) {
            suggestionTimer.restart()
        }
    }

    function handleFocusGained() {
        console.log("Gain focus, update popup");
        updatePopup()
    }

    function handleFocusLost() {
        console.log("Lost focus, hide popup");
        suggestionTimer.stop()
        hidePopup()
    }

    function selectLocation(selectedLocation) {
        searchEdit.location = selectedLocation
        searchEdit.text = selectedLocation.label
        showLocation(searchEdit.location)

        hidePopup()

        setupSearchCenter();
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
        }
        else {
            updateSuggestions()
        }
    }

    function handleCancel() {
        hidePopup();
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
        overlay.parent = desktop
        overlay.visible = true

        var mappedPosition = desktop.mapFromItem(searchEdit, 0, 0)
        var desktopFreeSpace = desktop.getFreeRect()

        popup.x = mappedPosition.x
        popup.y = desktopFreeSpace.y

        var popupHeight = suggestionView.contentHeight+2

        if (popupHeight > desktopFreeSpace.height) {
            popupHeight = desktopFreeSpace.height
        }

        suggestionBox.width = searchEdit.width;
        suggestionBox.height = popupHeight

        // If nothing is selected, select first line
        if (suggestionView.currentIndex < 0 || suggestionView.currentIndex >= suggestionView.count) {
            suggestionView.currentIndex = 0
        }

        popup.parent = desktop
        popup.visible = true
    }

    function hidePopup() {
        overlay.visible = false
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

    onAccepted: {
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

        // compute rank for location, it should be in range 0~1
        function locationRank(loc){

            if (loc.type=="coordinate"){
                return 1;
            } else if (loc.type=="object"){
                var rank=1;

                if (loc.objectType=="boundary_country"){
                    rank*=1;
                }else if (loc.objectType=="boundary_state"){
                    rank*=0.93;
                } else if (loc.objectType=="boundary_administrative" ||
                           loc.objectType=="place_town"){
                    rank*=0.9;
                } else if (loc.objectType=="highway_residential" ||
                           loc.objectType=="address"){
                    rank*=0.8;
                } else if (loc.objectType=="railway_station" ||
                           loc.objectType=="railway_tram_stop" ||
                           loc.objectType=="railway_subway_entrance" ||
                           loc.objectType=="highway_bus_stop"
                          ){
                    rank*=0.7;
                }else{
                    rank*=0.5;
                }
                var distance=loc.distanceTo(searchCenterLat, searchCenterLon);
                rank*= 1 / Math.log( (distance/1000) + Math.E);

                //console.log("rank " + loc.label + ": " + rank + "");
                return rank;
            }

            return 0;
        }

        compare: function(a, b){
            // console.log("compare " + a.label + " ("+locationRank(a)+") <> " + b.label + " ("+locationRank(b)+")");
            return locationRank(b) - locationRank(a);
        }

        equals: function(a, b){
            if (a.objectType == b.objectType &&
                a.distanceTo(b.lat, b.lon) < 300 &&
                a.distanceTo(searchCenterLat, searchCenterLon) > 3000
                ){
                // console.log("equals " + a.label + " <> " + b.label + ", " + a.objectType + " <> " + b.objectType + " distance: " + a.distanceTo(b.lat, b.lon));
                return true;
            }
            return false;
        }

        onCountChanged:{
            if (focus){
              updatePopup();
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
            popup.visible = false
        }
    }

    Item {
        id: popup

        visible: false
        z: 2

        width: suggestionBox.width
        height: suggestionBox.height

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

                delegate: Item{
                    width: suggestionView.width
                    height: labelLabel.height + entryRegion.height

                    Text {
                        id: labelLabel

                        width: parent.width - 2*Theme.horizSpace
                        x: Theme.horizSpace

                        text: label
                        font.pixelSize: Theme.textFontSize
                    }
                    Text {
                        id: typeLabel

                        //width: parent.width - 2*Theme.horizSpace
                        anchors.right: labelLabel.right

                        text: type
                        font.pixelSize: Theme.textFontSize * 0.8
                        opacity: 0.6
                    }
                    Text {
                        id: entryRegion

                        width: parent.width - 3*Theme.horizSpace
                        wrapMode: Text.WordWrap
                        anchors.top: labelLabel.bottom
                        x: 2*Theme.horizSpace

                        text: {
                            var str = "";
                            if (region.length > 0){
                                var start = 0;
                                while (start < region.length && region[start] == label){
                                    start++;
                                }
                                if (start < region.length){
                                    str = region[start];
                                    for (var i=start+1; i<region.length; i++){
                                        str += ", "+ region[i];
                                    }
                                }
                            }
                            return str;
                        }
                        font.pixelSize: Theme.textFontSize * 0.8
                        opacity: 0.9
                        visible: region.length > 0
                    }
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


