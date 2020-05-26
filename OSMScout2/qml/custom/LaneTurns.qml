
import QtQuick 2.0

Rectangle{
    id: laneTurnsComponent

    color: "transparent"
    height: 60

    property var laneTurns: ["", "through", "through;right", "right"]
    property var suggestedLaneFrom: 1
    property var suggestedLaneTo: 2
    property var maxWidth: height * 4

    property var iconWidth: Math.min(height * 0.60, maxWidth / laneTurns.length)
    property var iconOjects: []
    property var separatorObjects: []

    Component.onCompleted: {
        console.log("onCompleted");
        updateLane();
    }

    Component {
        id: iconComponent
        LaneTurnIcon{}
    }
    Component {
        id: separatorComponent
        Rectangle{
            height: parent.height;
            width: 2
            color: "black"
            opacity: 0.3
        }
    }

    onLaneTurnsChanged: {
        updateLane();
    }
    onSuggestedLaneFromChanged: {
        updateLane();
    }
    onSuggestedLaneToChanged: {
        updateLane();
    }

    function updateLane() {
        if (iconComponent.status == Component.Error) {
            // Error Handling
            console.log("Error loading component:", iconComponent.errorString());
            return
        }
        if (iconComponent.status != Component.Ready) {
            console.log("Component is not ready:", iconComponent.errorString());
            return
        }

        console.log("update");
        for (var i in iconOjects){
            iconOjects[i].destroy();
        }
        for (var i in separatorObjects){
            separatorObjects[i].destroy();
        }
        iconOjects = [];
        separatorObjects = [];

        var parentObj = laneTurnsComponent;
        parentObj.width = 0;
        for (var i in laneTurns){
            var turn = laneTurns[i];
            console.log("turn " + i + ": " + turn + " <" + suggestedLaneFrom + ", " + suggestedLaneTo + ">");
            var icon = iconComponent.createObject(parentObj,
                                              {
                                                  id: "turnIcon" + i,
                                                  turnType: turn,
                                                  height: parentObj.height,
                                                  width: iconWidth,
                                                  outline: ( i < suggestedLaneFrom || i > suggestedLaneTo)
                                              });
            if (icon == null) {
                // Error Handling
                console.log("Error creating icon object");
                return;
            }


            if (i==0){
                icon.anchors.left = parentObj.left;
            }else{
                console.log("iconOjects.length: " + iconOjects.length + " i: "+i);
                var prev = iconOjects[i-1];
                icon.anchors.left = prev.right;

                var separator = separatorComponent.createObject(parentObj);
                separator.anchors.left = prev.right;
                separatorObjects.push(separator);
            }

            iconOjects.push(icon);
            parentObj.width += icon.width;
        }
    }

}
