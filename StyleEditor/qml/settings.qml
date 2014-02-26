import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1

ColumnLayout {
    id: dialogFrame
    anchors.margins: 10
    anchors.fill: parent

    GridLayout {
        columns: 2
        anchors.top: parent.top

        Text {
            Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
            Layout.minimumWidth: width

            text: "DPI"
        }

        SpinBox {
            Layout.alignment: Qt.AlignLeft
            Layout.minimumWidth: 80

            focus: true
            minimumValue: 1
            maximumValue: 400
            suffix: " DPI"
            horizontalAlignment: Qt.AlignRight
        }

        Text {
            Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
            Layout.minimumWidth: width

            text: "Bla blu blub"
        }

        SpinBox {
            Layout.alignment: Qt.AlignLeft
            Layout.minimumWidth: 80
            Layout.fillWidth: true

            focus: true
            minimumValue: 1
            maximumValue: 400
        }
    }

    RowLayout {
        id: buttons
        width: parent.width
        anchors.bottom: parent.bottom

        Rectangle {
            Layout.fillWidth: true
        }

        Button {
            text: "Cancel"
        }

        Button {
            text: "OK"
        }
    }
}
