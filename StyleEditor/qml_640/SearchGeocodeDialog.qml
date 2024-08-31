import QtQuick 2.12
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import "custom"

DialogBase {
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

    ColumnLayout {
        anchors.centerIn: parent

        GridLayout {

            columns: 2

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
                font.pixelSize: units.fs("large")
                text: "Latitude:"
            }

            LineEdit {
                id: latInput
                width: units.gu(20)

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
                font.pixelSize: units.fs("large")
                text: "Longitude:"
            }

            LineEdit {
                id: lonInput
                width: units.gu(20)

                horizontalAlignment: TextInput.AlignRight
                validator: DoubleValidator {
                    bottom: -180.0
                    top: 180.0
                    decimals: 5
                    notation: DoubleValidator.StandardNotation
                }
            }
        }

    }

    footer: Row {
        leftPadding: units.gu(1)
        rightPadding: units.gu(1)
        bottomPadding: units.gu(1)
        spacing: units.gu(1)
        layoutDirection: Qt.RightToLeft

        Button {
            flat: true
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
