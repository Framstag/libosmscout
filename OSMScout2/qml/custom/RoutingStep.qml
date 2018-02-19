import QtQuick 2.2

import net.sf.libosmscout.map 1.0

Item {
    id: item

    anchors.right: parent.right;
    anchors.left: parent.left;
    height: Math.max(text.implicitHeight+5, icon.height)

    RouteStepIcon{
        id: icon
        stepType: type
        width: 12 * Theme.dpi/25.4
        height: width

        Component.onCompleted: {
            console.log("width: "+width);
        }
    }

    Text {
        id: text

        y: 2
        x: 2
        anchors.left: icon.right
        width: parent.width - 4 - icon.width
        text: description
        font.pixelSize: Theme.textFontSize
        wrapMode: Text.Wrap
    }

    Rectangle {
        x: 2
        y: parent.height-2
        width: parent.width-4
        height: 1
        color: "lightgrey"
    }
}
