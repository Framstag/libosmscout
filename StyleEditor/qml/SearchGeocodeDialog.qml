import QtQuick 2.2
import QtQuick.Layouts 1.1

import "custom"

MapDialog {
    id: dialog

    property double lat
    property double lon

    signal showCoordinates(double lat, double lon)

    onLatChanged: {
        var stringLat =(Math.round(lat*100000)/100000).toLocaleString()

        latInput.text= stringLat
    }

    onLonChanged: {
        var stringLon =(Math.round(lon*100000)/100000).toLocaleString()

        lonInput.text = stringLon
    }

    Rectangle {
        id: content
        width: mainFrame.implicitWidth+20
        height: mainFrame.implicitHeight+20

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 10

        ColumnLayout {
            id: mainFrame
            anchors.margins: 10
            anchors.fill: parent

            GridLayout {
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.bottom: buttonRow.top;

                columns: 2

                Text {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                    text: "Latitude:"
                }

                LineEdit {
                    id: latInput
                    width: 80

                    horizontalAlignment: TextInput.AlignRight
                    validator: DoubleValidator {
                        bottom: -90.0
                        top: 90.0
                        decimals: 5
                        notation: DoubleValidator.StandardNotation
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                    text: "Longitude:"
                }

                LineEdit {
                    id: lonInput
                    width: 80

                    horizontalAlignment: TextInput.AlignRight
                    validator: DoubleValidator {
                        bottom: -180.0
                        top: 180.0
                        decimals: 5
                        notation: DoubleValidator.StandardNotation
                    }
                }
            }

            RowLayout {
                id: buttonRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: 10

                Item {
                    Layout.fillWidth: true
                }

                DialogActionButton {
                    id: ok
                    width: 25
                    height: 25
                    text: "OK"

                    onClicked: {
                        if (latInput.text.length >0 && lonInput.text.length > 0) {
                            var locale = Qt.locale()

                            var numberLat = Number.fromLocaleString(locale, latInput.text)
                            var numberLon = Number.fromLocaleString(locale, lonInput.text)

                            showCoordinates(numberLat, numberLon)
                        }

                        dialog.destroy()
                    }
                }
            }
        }
    }
}
