import QtQuick 2.12
import QtQuick.Controls 2.2

DialogBase {
    id: dialog

    property string actionName: qsTr("Ok")

    // note: connect the signal reply(bool) to process the response
    signal reply(bool accepted)

    onAccepted: reply(true)

    onRejected: reply(false)

    footer: Row {
        leftPadding: units.gu(1)
        rightPadding: units.gu(1)
        bottomPadding: units.gu(1)
        spacing: units.gu(1)
        layoutDirection: Qt.RightToLeft

        Button {
            flat: true
            text: actionName
            onClicked: {
                dialog.accept();
            }
        }
        Button {
            flat: true
            text: qsTr("Cancel")
            onClicked: {
                dialog.reject();
            }
        }
    }
}
