import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.1
import FileIO 1.0


Rectangle {
    property alias source: styleFile.source

    property MapControl map

    width:  300
    height: 200

    ToolBar {
        id: toolBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        RowLayout {
            ToolButton {
                id: save
                text: "Save"
                action: Action {
                    onTriggered: {
                        styleFile.write()
                        map.reloadStyle()
                    }
                }
            }
        }
    }

    TextArea {
        Accessible.name: "document"
        id: textArea
        frameVisible: false
        anchors.top: toolBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        //baseUrl: "qrc:/"
        font.family: "Courier"
        textFormat: Qt.RichText
    }

    FileIO {
        id: styleFile
        onError: console.log(msg)
        onSourceChanged: {
            target=textArea
            read()
            textArea.cursorPosition=0
            textArea.forceActiveFocus()
        }
    }
}
