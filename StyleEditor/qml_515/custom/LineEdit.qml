import QtQuick 2.12

Item {
    id: lineEdit

    property color defaultBackgroundColor: "white"

    property color backgroundColor: defaultBackgroundColor
    property color focusColor: "lightgrey"
    property color selectedFocusColor: "lightblue"

    property alias validator: input.validator
    property alias text: input.text
    property alias horizontalAlignment: input.horizontalAlignment
    property alias radius: background.radius

    signal accepted

    height: input.implicitHeight + units.gu(0.5)

    Rectangle {
        id: background
        anchors.fill: parent

        color: backgroundColor
        border.color: focusColor
        border.width: units.dp(1)
    }

    TextInput {
        id: input
        width: parent.width - units.gu(0.5)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: units.fs("large")
        clip: true

        onFocusChanged: {
            background.border.color = focus ? selectedFocusColor : focusColor
        }

        onAccepted: {
            lineEdit.accepted()
        }
    }
}
