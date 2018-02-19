import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.2
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

    Rectangle {
        id: textPanel
        anchors.top: toolBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Item {
            anchors.fill: parent
            clip: true
            Rectangle {
                id: lineColumn
                property int rowHeight: textArea.font.pixelSize + 3 // TODO: use dynamic line space
                color: "#f2f2f2"
                width: 50
                height: parent.height

                Rectangle {
                    height: parent.height
                    anchors.right: parent.right
                    width: 1
                    color: "#ddd"
                }
                Column {
                    y: -textArea.flickableItem.contentY + textArea.textMargin // 4
                    width: parent.width

                    // hack for posponing lineNumberRepeater initialisation
                    property bool ready: false
                    Component.onCompleted: {
                        console.log("repeater: Max("+ (textArea.lineCount + 2) +", " + lineColumn.height +" / " + lineColumn.rowHeight +")");
                        ready = true;
                    }
                    Repeater {
                        id: lineNumberRepeater
                        model: parent.ready ? Math.max(textArea.lineCount + 2, (lineColumn.height/lineColumn.rowHeight) ) : (lineColumn.height/lineColumn.rowHeight)

                        delegate: Text {
                            id: text
                            color: "#666"
                            font: textArea.font
                            width: lineColumn.width
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            height: lineColumn.rowHeight
                            renderType: Text.NativeRendering
                            text: index
                        }
                    }
                }
            }

            TextArea {
                Accessible.name: "document"
                id: textArea

                anchors.left: lineColumn.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                frameVisible: false
                font.family: "Courier"
                textFormat: Qt.RichText
                wrapMode: TextEdit.NoWrap

                tabChangesFocus: true

                Component.onCompleted: {
                    cursorPosition = 0;
                    textArea.flickableItem.contentY = 0;
                    textArea.flickableItem.contentX = 0;
                }

                property string newLine: String.fromCharCode(0x2029) // 8233
                property string nbsp: String.fromCharCode(0xa0)

                Keys.onBacktabPressed: {
                    // console.log("backtab " + cursorPosition + ", " +  selectionStart + ", " + selectionEnd);
                    // remove two spaces before each line in selection

                    var originalText=getText(0, textArea.length);
                    var yScroll = textArea.flickableItem.contentY;

                    var blockStart=originalText.lastIndexOf(newLine, selectionStart-1)+1;
                    var blockCursorPosition=cursorPosition-blockStart;
                    var blockSelectionStart=selectionStart-blockStart;
                    var originalSelectionEnd=selectionEnd;

                    var textBefore=originalText.substring(0, blockStart);
                    var textBlock=originalText.substring(blockStart, selectionEnd);
                    var textAfter=originalText.substring(selectionEnd, originalText.length);

                    console.log(selectionStart + " " + blockStart);

                    var updatedBlock="";
                    var removeNextSpaces=2;
                    var sizeChange=0;
                    for (var pos=0; pos<textBlock.length; pos++){
                        var ch = textBlock.substring(pos, pos+1);
                        if (removeNextSpaces>0 && ch===nbsp){
                            removeNextSpaces--;
                            if (blockCursorPosition>=pos){
                                blockCursorPosition--;
                            }
                            if (blockSelectionStart>=pos){
                                blockSelectionStart--;
                            }
                            sizeChange--;
                        }else{
                            updatedBlock += ch;
                            removeNextSpaces=0;
                        }
                        if (ch===newLine){
                            removeNextSpaces=2;
                        }
                    }
                    cursorPosition=0;
                    var updated = (textBefore + updatedBlock + textAfter)
                                    .replace(/</g, "&lt;").replace(/>/g, "&gt;");
                    textArea.text = updated;

                    cursorPosition = blockCursorPosition + blockStart;
                    if (blockCursorPosition==blockSelectionStart){
                        textArea.select(originalSelectionEnd+sizeChange, blockSelectionStart+blockStart);
                    }else{
                        textArea.select(blockSelectionStart+blockStart, originalSelectionEnd+sizeChange);
                    }
                    textArea.flickableItem.contentY = yScroll;
                }
                Keys.onTabPressed: {
                    //console.log("tab " + cursorPosition + ", " +  selectionStart + ", " + selectionEnd);
                    if (selectionStart==selectionEnd){
                        // no selection, add two spaces on cursor position
                        insert(cursorPosition, "&nbsp;&nbsp;");
                    }else{
                        // indent block (each line in selection) by two spaces

                        var originalText=getText(0, textArea.length);
                        var blockStart=originalText.lastIndexOf(newLine, selectionStart)+1;

                        var blockCursorPosition=cursorPosition-blockStart;
                        var blockSelectionStart=selectionStart-blockStart;
                        var originalSelectionEnd=selectionEnd;
                        var yScroll = textArea.flickableItem.contentY;

                        var textBefore=originalText.substring(0, blockStart);
                        var textBlock=originalText.substring(blockStart, selectionEnd);
                        var textAfter=originalText.substring(selectionEnd, originalText.length);

                        var updatedBlock="";
                        var lineStart=true;
                        var sizeChange=0;
                        for (var pos=0; pos<textBlock.length; pos++){
                            if (lineStart){
                                updatedBlock += "&nbsp;&nbsp;"
                                lineStart=false;
                                if (blockCursorPosition>=pos){
                                    blockCursorPosition+=2;
                                }
                                if (blockSelectionStart>=pos){
                                    blockSelectionStart+=2;
                                }
                                sizeChange+=2;
                            }
                            var ch = textBlock.substring(pos, pos+1);
                            updatedBlock += ch;
                            if (ch===newLine){
                                lineStart=true;
                            }
                        }
                        cursorPosition=0;
                        var updated = (textBefore + updatedBlock + textAfter)
                                        .replace(/</g, "&lt;").replace(/>/g, "&gt;");

                        textArea.text = updated;

                        console.log(textArea.length + " = " + updated.length);
                        cursorPosition = blockCursorPosition + blockStart;
                        if (blockCursorPosition==blockSelectionStart){
                            textArea.select(originalSelectionEnd+sizeChange, blockSelectionStart+blockStart);
                        }else{
                            textArea.select(blockSelectionStart+blockStart, originalSelectionEnd+sizeChange);
                        }
                        textArea.flickableItem.contentY = yScroll;
                    }
                }
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
    }

    Dialog {
        id: saveDialog
        property bool isAccepted: false
        property bool isRefused: false
        visible: false
        title: qsTr("Style file is not saved!")
        standardButtons: StandardButton.Save | StandardButton.No | StandardButton.Cancel

        Text{
            text: qsTr("Save stylesheet changes to %1 ?").arg(styleFile.source)
            wrapMode: Text.Wrap
        }

        onAccepted: {
            isAccepted = true;
            visible = false;
            console.log("Save dialog accepted");
            mainWindow.close();
        }
        onNo: {
            isRefused = true;
            visible = false;
            console.log("Save dialog refused");
            mainWindow.close();
        }
        onRejected: {
            visible = false;
            console.log("Save dialog canceled");
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

        function onWindowClosing(event){
            console.log("Save dialog "+saveDialog.isAccepted+" "+saveDialog.isRefused);
            event.accepted = false;
            if (saveDialog.isAccepted){
                if (styleFile.write()){
                    event.accepted = true;
                }
            } else if (saveDialog.isRefused){
                event.accepted = true;
            } else if (isModified()){
                console.log("Stylesheet is not saved, don't accept close event, show saveDialog...");
                saveDialog.visible = true;
            } else {
                event.accepted = true;
            }
        }

        Component.onCompleted: {
            read();
            mainWindow.closing.connect(onWindowClosing)
        }
    }

}
