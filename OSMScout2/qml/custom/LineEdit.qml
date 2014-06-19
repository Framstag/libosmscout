import QtQuick 2.2

import net.sf.libosmscout.map 1.0

Item {
    id: lineEdit

    property color defaultBackgroundColor: "white"

    property color backgroundColor: defaultBackgroundColor
    property color focusColor: "lightgrey"
    property color selectedFocusColor: "lightblue"

    property alias validator: input.validator
    property alias text: input.text
    property alias horizontalAlignment: input.horizontalAlignment
    property alias maximumLength: input.maximumLength
    property alias inputMethodHints: input.inputMethodHints

    width: input.width+4
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

        anchors.fill: parent
        anchors.margins: 2

        font.pixelSize: Theme.textFontSize
        clip: true

        onFocusChanged: {
            background.border.color = focus ? selectedFocusColor : focusColor
        }
    }
}
