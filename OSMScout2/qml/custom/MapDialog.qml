import QtQuick 2.2
import QtQuick.Layouts 1.1

import net.sf.libosmscout.map 1.0

Item {
    id: dialog

    property bool fullscreen: false

    property alias label : label.text
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
            content.Layout.maximumHeight = overlay.height-5*Theme.vertSpace-title.height
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

        z: -1
        anchors.fill: parent

        color: "#000000"
        opacity: 0.2

        MouseArea {
            anchors.fill: parent

            onClicked: {
                if (mouse.x<frame.x || mouse.x>frame.x+frame.width ||
                    mouse.y<frame.y || mouse.y>frame.y+frame.height) {
                  close()
                }
            }
        }
    }

    Rectangle {
        id: frame

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: Theme.vertSpace
        anchors.bottomMargin: Theme.vertSpace

        border.color: "black"
        color: "white"

        width: mainLayout.width+2*Theme.horizSpace
        height: mainLayout.height+2*Theme.vertSpace

        ColumnLayout {
            id: mainLayout

            x: Theme.horizSpace
            y: Theme.vertSpace

            spacing: Theme.vertSpace

            Rectangle {
                id: title

                Layout.fillWidth: true

                height: label.implicitHeight

                Text {
                    id: label

                    width: parent.width

                    color: "black"
                    font.bold: true
                    //font.pixelSize: font.pixelSize*1.3
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            ColumnLayout {
                id: content

                Layout.fillWidth: true
                Layout.fillHeight: true

                focus: true

                /*
                Keys.onPressed: {
                    if (event.key === Qt.Key_Escape) {
                        close()
                    }
                }*/
            }
        }
    }
}

