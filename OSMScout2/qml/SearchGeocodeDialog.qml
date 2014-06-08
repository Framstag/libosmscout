import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

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

    label: "Goto coordinate..."

    content : ColumnLayout {
        id: mainFrame

        GridLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            anchors.bottom: buttonRow.top;

            columns: 2

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

                text: "Latitude:"
            }

            LineEdit {
                id: latInput

                maximumLength: 10
                width: Theme.numberCharWidth*10

                horizontalAlignment: TextInput.AlignRight
                inputMethodHints: Qt.ImhFormattedNumbersOnly
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

                maximumLength: 10
                width: Theme.numberCharWidth*10

                horizontalAlignment: TextInput.AlignRight
                inputMethodHints: Qt.ImhFormattedNumbersOnly
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
