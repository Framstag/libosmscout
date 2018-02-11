import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.1
import FileIO 1.0
import "custom"


Rectangle {
    property alias source: styleFile.source
    property alias selectedText: textArea.selectedText
    property alias searchText: searchText.text

    property MapControl map

    width:  300
    height: 200

    function selectText(index, length){
        textArea.cursorPosition=index;
        textArea.select(index, index+length)
        textArea.forceActiveFocus()
    }

    function findNext(text) {
        if(text.length>0){
            var foundIndex = textArea.getText(0, textArea.length).indexOf(text, textArea.cursorPosition);
            if(foundIndex>0){
                selectText(foundIndex, text.length);
            } else {
                foundIndex = textArea.getText(0, textArea.length).indexOf(text, 0);
                if(foundIndex>0){
                    selectText(foundIndex, text.length);
                }
            }
        }
    }

    function findPrevious(text) {
        if(text.length>0){
            var foundIndex = textArea.getText(0, textArea.length).lastIndexOf(text, textArea.cursorPosition-text.length-1);
            if(foundIndex>0){
                selectText(foundIndex, text.length);
            } else {
                foundIndex = textArea.getText(0, textArea.length).lastIndexOf(text, textArea.length-1);
                if(foundIndex>0){
                    selectText(foundIndex, text.length);
                }
            }
        }
    }

    function goTo(l,c) {
        textArea.cursorPosition = styleFile.lineOffset(l-1) + c - 1;
    }

    function checkForErrors() {
        if(map.stylesheetHasErrors){
            goTo(map.stylesheetErrorLine, map.stylesheetErrorColumn)
            textArea.height = textPanel.height - 24;
            statusText.remove(0, statusText.length)
            statusText.append("Line "+ map.stylesheetErrorLine + " column " + map.stylesheetErrorColumn + " " + map.stylesheetErrorDescription)
            statusText.visible = true
        } else {
            statusText.visible = false
            textArea.height = textPanel.height;
        }
    }

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
                        checkForErrors()
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
                        checkForErrors()
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
                        checkForErrors()
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

                onAccepted: {
                    findNext(text)
                }
            }
            ToolButton {
                id: previous
                action: Action {
                    text: "<"
                    shortcut: StandardKey.FindPrevious
                    onTriggered: {
                        findPrevious(searchText.text)
                    }
                }
            }
            ToolButton {
                id: next
                action: Action {
                    text: ">"
                    shortcut: StandardKey.FindNext
                    onTriggered: {
                        findNext(searchText.text)
                    }
                }
            }
        }
    }

    Column {
        id: textPanel
        anchors.top: toolBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        TextArea {
            Accessible.name: "document"
            id: textArea
            frameVisible: false
            height: parent.height
            width: parent.width
            font.family: "Courier"
            textFormat: Qt.RichText
        }
        TextArea {
            id: statusText
            visible: false
            frameVisible: false
            width: parent.width
            height: 20
            textColor: "#ff0000"
            font.family: "Courier"
            font.bold: true
            textFormat: Qt.RichText
        }
    }

    FileIO {
        id: styleFile
        onError: console.log(msg)
        target: textArea
        onSourceChanged: {
            read()
            textArea.cursorPosition=0
            textArea.forceActiveFocus()
        }

        Component.onCompleted: {
            read();
        }
    }

}
