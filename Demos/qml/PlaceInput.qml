import QtQuick 2.12

import QtQuick.Controls 1.4
import QtQuick.Layouts 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Window 2.12

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"
import "custom/Utils.js" as Utils

GridLayout {
    id: placeInput

    property alias text: inputField.text
    property alias inputFocus: inputField.focus
    property LocationEntry location: null

    columns: 2

    TextField {
        id: inputField
        Layout.fillWidth: true
        textColor: "black"

        LocationListModel {
            id: startLocationSearchModel
            pattern: inputField.text

            onCountChanged: {
                if (count==0){
                    if (pattern!="" && !searching){
                        statusText.text = "No location found for " + pattern;
                        stateText.text = "error";
                    } else {
                        if (pattern!=""){
                            stateText.text = "searching";
                        } else {
                            stateText.text = "empty";
                        }
                    }

                    console.log("empty!");
                    placeInput.location = null;
                    return;
                }
                console.log("NOT empty!");

                console.log("Start location: " + locationStr(startLocationSearchModel.get(0)));
                stateText.text = "ok";
                placeInput.location = startLocationSearchModel.get(0);
            }
        }
    }
    Text {
        id: stateText
        text: qsTr("empty")
    }
}
