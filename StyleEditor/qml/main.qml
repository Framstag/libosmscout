import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1

ApplicationWindow {
    id: mainWindow
    color: "white"
    title: "OSMScout style editor"
    visible: true
    width: 1200
    height: 800

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem { text: "Open..." }
            MenuItem {
                text: "Close"
                action: Action {
                    onTriggered: {
                        mainWindow.close();
                    }
                }
            }
        }

        Menu {
            title: "Edit"
            MenuItem { text: "Cut" }
            MenuItem { text: "Copy" }
            MenuItem { text: "Paste" }
            MenuSeparator { }
            MenuItem {
                text: "Find"
                action: Action {
                    shortcut: StandardKey.Find
                    onTriggered: {
                        var text = textEditor.selectedText;
                        if(text!==""){
                            textEditor.searchText = text;
                        } else {
                            text = textEditor.searchText;
                        }

                        textEditor.findNext(text);
                    }
                }
            }
        }
    }

    SplitView {
        id: mainRow
        anchors.fill: parent
        orientation: Qt.Horizontal


        Grid {
            Layout.fillWidth: true
            width: parent.width*0.6
            Layout.minimumWidth: 200
            columnSpacing: 6
            rowSpacing: 6
            columns: 1
            rows: 1

            MapControl {
                id: map1
                width: (parent.width-parent.columnSpacing*(parent.columns-1))*(1/parent.columns)
                height: (parent.height-parent.columnSpacing*(parent.rows-1))*(1/parent.rows)
            }
            /*  MapControl {
                id: map2
                width: (parent.width-parent.columnSpacing*(parent.columns-1))*(1/parent.columns)
                height: (parent.height-parent.columnSpacing*(parent.rows-1))*(1/parent.rows)
            }
            MapControl {
                id: map3
                width: (parent.width-parent.columnSpacing*(parent.columns-1))*(1/parent.columns)
                height: (parent.height-parent.columnSpacing*(parent.rows-1))*(1/parent.rows)
            }
            MapControl {
                id: map4
                width: (parent.width-parent.columnSpacing*(parent.columns-1))*(1/parent.columns)
                height: (parent.height-parent.columnSpacing*(parent.rows-1))*(1/parent.rows)
            }*/
        }

        TextEditor {
            id: textEditor
            width: parent.width*0.4
            map: map1
            source: map1.stylesheetFilename
        }
    }
}
