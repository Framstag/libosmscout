import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

Item {
    id: dialog

    property bool fullscreen: false

    property alias label : title.text
    property alias content : content.children

    anchors.fill: parent
    visible: false
    opacity: 0

    signal opened
    signal closed

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
        if (fullscreen) {
            content.Layout.preferredHeight = overlay.height-5*Theme.vertSpace-title.height
        }

        visible = true

        dialog.opened()

        openAnim.start()
    }

    function close() {
        dialog.closed()

        openAnim.stop()
        closeAnim.start()
    }

    Rectangle {
        id: overlay

        z: -2
        anchors.fill: parent

        color: "#000000"
        opacity: 0.2

        MouseArea {
            anchors.fill: parent

            onClicked: {
                if (mouse.x<mainLayout.x || mouse.x>mainLayout.x+mainLayout.width ||
                    mouse.y<mainLayout.y || mouse.y>mainLayout.y+mainLayout.height) {
                  close()
                }
            }
        }
    }

    Rectangle {
        id: background
        z: -1

        border.color: "black"
        color: "white"

        x: mainLayout.x-Theme.horizSpace
        y: mainLayout.y-Theme.vertSpace
        width: mainLayout.width+2*Theme.horizSpace
        height: mainLayout.height+2*Theme.vertSpace
    }

    ColumnLayout {
        id: mainLayout

        x: (parent.width-width)/2
        y: 2*Theme.vertSpace
        Layout.minimumWidth: 4*Theme.horizSpace
        Layout.minimumHeight: 4*Theme.vertSpace
        Layout.maximumWidth: parent.width-4*Theme.horizSpace
        Layout.maximumHeight: parent.height-4*Theme.vertSpace

        spacing: Theme.vertSpace

        Text {
            id: title

            Layout.fillWidth: true

            color: "black"
            font.bold: true
            font.pixelSize: Theme.textFontSize
            horizontalAlignment: Text.AlignHCenter
        }

        ColumnLayout {
            id: content

            Keys.onPressed: {
                if (event.key === Qt.Key_Escape) {
                    close()
                }
            }
        }
    }

    onOpened: {
        content.focus = true
    }
}

