import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

FocusScope {
    id: control

    property color defaultBackgroundColor: "white"

    property color backgroundColor: defaultBackgroundColor
    property color focusColor: "lightgrey"
    property color selectedFocusColor: "lightblue"

    property alias validator: input.validator
    property alias text: input.text
    property alias horizontalAlignment: input.horizontalAlignment
    property alias maximumLength: input.maximumLength
    property alias inputMethodHints: input.inputMethodHints

    signal accepted()

    height: background.height

    Rectangle {
        id: background

        color: backgroundColor
        border.color: focusColor
        border.width: 1


        anchors.fill: parent

        height: input.implicitHeight+4

        RowLayout {

            anchors.fill: parent

            spacing: 0

            TextInput {
                id: input

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignVCenter

                font.pixelSize: Theme.textFontSize
                selectByMouse: true
                clip: true
                focus: true

                onFocusChanged: {
                    background.border.color = focus ? selectedFocusColor : focusColor
                }

                onAccepted: {
                    control.accepted()
                }

                onTextChanged: {
                    deleteButton.visible = text.length>0
                }
            }

            Image {
                id: deleteButton

                width: input.height
                height: input.height
                visible: false

                source: "qrc:///pics/DeleteText.svg"
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                sourceSize.width: input.height
                sourceSize.height: input.height

                MouseArea {
                    anchors.fill: deleteButton

                    onClicked: {
                        input.text = ""
                    }
                }
            }
        }
    }
}
