import QtQuick 2.12

Rectangle {
    id: container

    property Flickable flickableArea

    x: flickableArea.x+flickableArea.width - units.gu(0.5)
    y: flickableArea.y
    width: units.gu(0.5)
    height: flickableArea.height

    color: "#00000000"

    Rectangle {
        id: indicator
        x: 0
        y: flickableArea.visibleArea.yPosition * container.height
        width: parent.width
        height: flickableArea.visibleArea.heightRatio * container.height
        color: "black"
        opacity: height==parent.height ? 0 : 0.3
    }
}

