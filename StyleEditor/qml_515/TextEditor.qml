import QtQuick 2.12
import QtQuick.Controls 2.2
import DocumentHandler 1.0

import "custom"

Item {
    id: textEditor
    property color baseColor: "#f2f2f2"
    property color backgroundColor: "white"
    property alias source: document.source
    property MapControl map

    property alias modified: document.modified
    property alias fileName: document.fileName
    property alias fileType: document.fileType

    function load() {
        document.load();
    }
    function save() {
        document.save();
    }
    function activeFocus() {
        textArea.forceActiveFocus();
    }

    Connections {
        target: map
        // check for style errors
        function onStylesheetHasErrorsChanged() {
            if(map.stylesheetHasErrors) {
                var t = findToken(map.stylesheetErrorLine, map.stylesheetErrorColumn);
                selectText(t.start, t.end-t.start+1);
                textArea.forceActiveFocus();
                setStatusText("Line "+ map.stylesheetErrorLine + " column " + map.stylesheetErrorColumn + " " + map.stylesheetErrorDescription);
            } else {
                clearStatusText();
            }
        }
    }

    // shortcuts
    Shortcut {
        sequence: StandardKey.Save
        onActivated: document.save()
    }
    Shortcut {
        sequence: StandardKey.Copy
        onActivated: textArea.copy()
    }
    Shortcut {
        sequence: StandardKey.Cut
        onActivated: textArea.cut()
    }
    Shortcut {
        sequence: StandardKey.Paste
        onActivated: textArea.paste()
    }
    Shortcut {
        sequence: "Ctrl+F"
        onActivated: {
            searchText.forceActiveFocus();
        }
    }
    Shortcut {
        enabled: searchText.text.length > 0
        sequence: "F3"
        onActivated: {
            findNext(searchText.text);
            textArea.forceActiveFocus();
        }
    }
    Shortcut {
        enabled: searchText.text.length > 0
        sequence: "Shift+F3"
        onActivated: {
            findPrevious(searchText.text);
            textArea.forceActiveFocus();
        }
    }
    Shortcut {
        enabled: textArea.focus == true
        sequence: StandardKey.MoveToNextPage
        onActivated: {
            var p = textArea.cursorPosition;
            var c = 0;
            for (;;) {
                p = textArea.text.indexOf("\n", p + 1);
                if (p > 0) {
                    if (++c > textArea.visibleLineCount) {
                        ++p; // go to 1st char of next line
                        break;
                    }
                } else {
                    p = textArea.text.length; // go to end
                    break;
                }
            }
            textArea.cursorPosition = p;
        }
    }
    Shortcut {
        enabled: textArea.focus == true
        sequence: StandardKey.MoveToPreviousPage
        onActivated: {
            var p = textArea.cursorPosition;
            var c = 0;
            for (;;) {
                p = textArea.text.lastIndexOf("\n", p - 1);
                if (p > 0) {
                    if (++c > textArea.visibleLineCount + 1) {
                        ++p; // go to 1st char of next line
                        break;
                    }
                } else {
                    p = 0; // go to begin
                    break;
                }
            }
            textArea.cursorPosition = p;
        }
    }

    DocumentHandler {
        id: document
        document: textArea.textDocument
        cursorPosition: textArea.cursorPosition
        selectionStart: textArea.selectionStart
        selectionEnd: textArea.selectionEnd
        indentString: "  "

        onSourceChanged: {
            clearStatusText();
            load();
            textArea.cursorPosition=0
            textArea.forceActiveFocus()
        }
        onLoaded: function(text, format) {
            clearStatusText();
            textArea.textFormat = format;
            textArea.text = text;
            textArea.cursorPosition = 0;
            textArea.forceActiveFocus();
        }
        onSaved: {
            clearStatusText();
        }

        onError: function(message) {
            setStatusText(message);
        }

        Component.onCompleted: {
            load();
        }
    }

    // the text editor
    Item {
        width: parent.width
        anchors.top: header.bottom
        anchors.bottom: footer.top

        // line numbers
        Rectangle {
            id: lineColumn
            color: baseColor
            anchors.left: parent.left
            width: units.gu(6)
            height: parent.height

            Rectangle {
                height: parent.height
                anchors.right: parent.right
                width: units.dp(1)
                color: "#ddd"
            }

            Column {
                width: parent.width
                y: -flickable.contentY // glue to the scroll view

                Repeater {
                    id: lineNumberRepeater
                    model: textArea.lineCount

                    delegate: Text {
                        id: text
                        color: "#666"
                        font: textArea.font
                        width: lineColumn.width
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        height: textArea.rowHeight
                        text: index+1
                    }
                }
            }
        }

        // the scroll view
        Flickable {
            id: flickable
            flickableDirection: Flickable.VerticalFlick
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: lineColumn.right
            anchors.right: parent.right
            maximumFlickVelocity: textArea.font.pixelSize * 50

            TextArea.flickable: TextArea {
                id: textArea
                anchors.fill: parent
                textFormat: Qt.PlainText
                wrapMode: TextArea.NoWrap
                font.family: "Courier"
                font.pixelSize: units.fs("medium")
                selectByMouse: true
                persistentSelection: true
                leftPadding: units.gu(1)
                rightPadding: units.gu(2)
                topPadding: 0
                bottomPadding: 0
                background: Rectangle {
                    color: backgroundColor
                }

                property int visibleLineCount: flickable.visibleArea.heightRatio * textArea.lineCount - 1
                property real rowHeight: height / textArea.lineCount

                MouseArea {
                    acceptedButtons: Qt.RightButton
                    anchors.fill: parent
                    onClicked: {
                        contextMenu.x = mouseX;
                        contextMenu.y = mouseY;
                        contextMenu.open();
                    }

                    Menu {
                        id: contextMenu

                        MenuItem {
                            text: qsTr("Copy")
                            enabled: textArea.selectedText
                            onTriggered: textArea.copy()
                        }
                        MenuItem {
                            text: qsTr("Cut")
                            enabled: textArea.selectedText
                            onTriggered: textArea.cut()
                        }
                        MenuItem {
                            text: qsTr("Paste")
                            enabled: textArea.canPaste
                            onTriggered: textArea.paste()
                        }
                    }
                }

                Keys.onTabPressed: {
                    if (selectionStart === selectionEnd) {
                        textArea.insert(cursorPosition, document.indentString);
                    } else {
                        var start = textArea.selectionStart;
                        textArea.select(start, start + document.tabSelectedText());
                    }
                }
                Keys.onBacktabPressed: {
                    var start = textArea.selectionStart;
                    textArea.select(start, start + document.backtabSelectedText());
                }
            }

            ScrollBar.vertical: ScrollBar {
                width: units.gu(1.5)
                policy: ScrollBar.AlwaysOn
            }
            ScrollBar.horizontal: ScrollBar {
                width: units.gu(1.5)
                policy: ScrollBar.AsNeeded
            }
        }
    }

    // the header tool bar
    Rectangle {
        id: header
        width: parent.width
        height: units.gu(5)
        color: baseColor

        Row {
            id: fileRow
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: saveButton.width
                height: saveButton.height
                //color: "transparent"
                //border.width: units.dp(2)
                //border.color: saveButton.enabled ? "red" : "transparent"
                color: saveButton.enabled ? "#ffd" : "transparent"
                ToolButton {
                    id: saveButton
                    height: units.gu(5)
                    anchors.centerIn: parent
                    enabled: document.modified
                    icon.source: "qrc:/images/save.svg"
                    onClicked: {
                        if (document.save()) {
                            map.reloadStyle()
                        }
                    }
                }
            }
            ToolButton {
                id: openButton
                height: units.gu(5)
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "qrc:/images/load.svg"
                onClicked: {
                    if (document.modified)
                        reloadDialog.open();
                    else
                        reloadStyle();
                }
            }
            ToolButton {
                id: renderButton
                height: units.gu(5)
                width: units.gu(5)
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "qrc:/images/render.svg"
                onClicked: {
                    if (document.saveTmp()) {
                        map.reloadTmpStyle()
                    }
                }
            }
            ToolSeparator {
                height: units.gu(5)
            }
            ToolButton {
                id: copyButton
                height: units.gu(5)
                width: units.gu(5)
                anchors.verticalCenter: parent.verticalCenter
                enabled: textArea.selectedText
                onClicked: textArea.copy()
                icon.source: "qrc:/images/copy.svg"
            }
            ToolButton {
                id: cutButton
                height: units.gu(5)
                width: units.gu(5)
                anchors.verticalCenter: parent.verticalCenter
                enabled: textArea.selectedText
                onClicked: textArea.cut()
                icon.source: "qrc:/images/cut.svg"
            }
            ToolButton {
                id: pasteButton
                height: units.gu(5)
                width: units.gu(5)
                anchors.verticalCenter: parent.verticalCenter
                enabled: textArea.canPaste
                onClicked: textArea.paste()
                icon.source: "qrc:/images/paste.svg"
            }
            ToolSeparator {
                height: units.gu(5)
            }
            // TextField {
            //     id: searchText
            //     anchors.verticalCenter: parent.verticalCenter
            //     width: units.gu(16)
            //     height: units.gu(3.5)
            //     font.pixelSize: units.fs("small")
            //     placeholderText: "Search ..."
            //     maximumLength: 255
            //     onAccepted: {
            //         findNext(searchText.text);
            //         searchText.forceActiveFocus(); // keep focus
            //     }
            // }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: searchText.width + units.gu(1)
                height: units.gu(3.5)
                border.width: searchText.focus ? units.dp(2) : units.dp(1)
                border.color: "black"
                color: searchText.focus ? "#ffd" : "white"
                TextInput {
                    id: searchText
                    anchors.centerIn: parent
                    width: units.gu(15)
                    font.pixelSize: units.fs("small")
                    maximumLength: 255
                    clip: true
                    onAccepted: {
                        findNext(searchText.text);
                        searchText.forceActiveFocus(); // keep focus
                    }
                }
            }
            ToolButton {
                id: searchNext
                height: units.gu(5)
                width: units.gu(5)
                anchors.verticalCenter: parent.verticalCenter
                enabled: searchText.text.length > 0
                icon.source: "qrc:/images/next.svg"
                onClicked: {
                    findNext(searchText.text);
                    textArea.forceActiveFocus();
                }
            }
            ToolButton {
                id: searchPrevious
                height: units.gu(5)
                width: units.gu(5)
                anchors.verticalCenter: parent.verticalCenter
                enabled: searchText.text.length > 0
                icon.source: "qrc:/images/previous.svg"
                onClicked: {
                    findPrevious(searchText.text);
                    textArea.forceActiveFocus();
                }
            }
        }

        Rectangle {
            width: parent.width
            anchors.bottom: parent.bottom
            height: units.dp(1)
            color: "#ddd"
        }
    }

    // the footer tool bar
    Rectangle {
        id: footer
        width: parent.width
        height: units.gu(4)
        anchors.bottom: parent.bottom
        color: baseColor

        Text {
            id: statusText
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            width: parent.width - analyser.width - units.gu(1)
            clip: true
            leftPadding: units.gu(1)
            color: "#ff0000"
            font.pixelSize: units.fs("medium")
        }

        CheckBox {
            id: analyser
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            checked: document.styleAnalyserEnabled
            onClicked: {
                if (checked)
                    document.styleAnalyserEnabled = true;
                else
                    document.styleAnalyserEnabled = false;
            }
        }

        Rectangle {
            width: parent.width
            anchors.top: parent.top
            height: units.dp(1)
            color: "#ddd"
        }
    }

    function clearStatusText() {
        statusText.text = "";
    }
    function setStatusText(text) {
        statusText.text = text;
    }

    function findToken(lno, cno) {
        var p = 0;
        var c = 1;
        while (c < lno) {
            p = textArea.text.indexOf('\n', p);
            if (p < 0)
                return {"start": 0, "end": 0};
            ++p;
            ++c;
        }
        var ret = {"start": (p + cno - 1), "end": textArea.text.length - 1};
        ['\n', ';', ' '].forEach((e) => {
            let ee = textArea.text.indexOf(e, ret.start) - 1;
            if (ee >= ret.start && ee < ret.end)
                ret.end = ee;
        });
        return ret;
    }

    function selectText(index, length){
        textArea.cursorPosition=index;
        textArea.select(index, index+length);
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

    function reloadStyle() {
        var savedTextPosition = textArea.cursorPosition;
        document.load()
        map.reloadStyle()
        textArea.cursorPosition = savedTextPosition
    }

    DialogAction {
        id: reloadDialog
        actionName: qsTr("Reload")
        title: qsTr("Style file is not saved !")
        text: qsTr("Reload file anyway ?")

        Component.onCompleted: {
            reloadDialog.reply.connect(function(accepted){
                reloadDialog.visible = false;
                if (accepted) {
                    reloadStyle();
                }
            })
        }
    }

}
