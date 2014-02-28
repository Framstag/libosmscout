import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.1
import FileIO 1.0
import "custom"


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
                id: render
                action: Action {
                    text: "Render"
                    shortcut: StandardKey.Refresh
                    onTriggered: {
                        styleFile.writeTmp()
                        map.reloadTmpStyle()
                    }
                }
            }
            ToolButton {
                id: load
                action: Action {
                    text: "Load"
                    shortcut: StandardKey.Load
                    onTriggered: {
                        var savedTextPosition = textArea.cursorPosition
                        styleFile.read()
                        map.reloadStyle()
                        textArea.cursorPosition = savedTextPosition
                    }
                }
            }
            ToolButton {
                id: save
                action: Action {
                    text: "Save"
                    shortcut: StandardKey.Save
                    onTriggered: {
                        styleFile.write()
                        map.reloadStyle()
                    }
                }
            }
            Item {
                Layout.minimumWidth: 10
                //Layout.preferredWidth: parent.width
                Layout.maximumWidth: Number.POSITIVE_INFINITY
                Layout.fillWidth: true
                Rectangle {
                    width: 10
                }
            }
            LineEdit {
                id: searchText
                Layout.maximumWidth: 120
                Layout.preferredWidth: 120
                Layout.minimumWidth: 40
                radius: 4

                onEditingFinished: {
                    var foundIndex = textArea.getText(0, textArea.length).indexOf(text, textArea.cursorPosition);
                    if(foundIndex>0){
                        textArea.cursorPosition=foundIndex;
                        textArea.select(foundIndex, foundIndex+text.length)
                        textArea.forceActiveFocus()
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
