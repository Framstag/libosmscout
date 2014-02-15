import QtQuick 2.2
import QtQuick.Layouts 1.1

Item {
    id: dialog

    anchors.fill: parent
    visible: false
    opacity: 0

    PropertyAnimation {
        id: openAnim
        target: dialog
        property: "opacity"
        duration: 200
        from: 0
        to: 1
        easing.type: Easing.InOutQuad
    }

    PropertyAnimation {
        id: closeAnim
        target: dialog
        property: "opacity"
        duration: 200
        from: 1
        to: 0
        easing.type: Easing.InOutQuad

        onStopped: {
            dialog.destroy()
        }
    }

    function open() {
        visible = true
        openAnim.start()
    }

    function close() {
        openAnim.stop()
        closeAnim.start()
    }

    Rectangle {
        id: overlay

        z: -1
        anchors.fill: parent

        color: "#000000"
        opacity: 0.2

        MouseArea {
            anchors.fill: parent

            onClicked: {
                close()
            }
        }
    }
}
