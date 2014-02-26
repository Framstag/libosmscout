import QtQuick 2.2

Item {
    id: lineEdit

    property color defaultBackgroundColor: "white"

    property color backgroundColor: defaultBackgroundColor
    property color focusColor: "lightgrey"
    property color selectedFocusColor: "lightblue"

    property alias validator: input.validator
    property alias text: input.text
    property alias horizontalAlignment: input.horizontalAlignment

    height: input.implicitHeight+4

    Rectangle {
        id: background
        anchors.fill: parent

        color: backgroundColor
        border.color: focusColor
        border.width: 1
    }

    TextInput {
        id: input
        height: parent.height-4
        width: parent.width-4
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        clip: true

        onFocusChanged: {
            background.border.color = focus ? selectedFocusColor : focusColor
        }
    }
}
