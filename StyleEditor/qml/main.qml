import QtQuick 2.2
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1

import net.sf.libosmscout.map 1.0

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

        Menu {
            id: styleFlagMenu
            title: "Style flags"

            Component {
                id: menuAction
                Action {
                    checkable: true
                    onCheckedChanged: {
                        if (styleFlagsModel.ready) {
                            styleFlagsModel.setFlag(text, checked);
                        }
                    }
                }
            }

            StyleFlagsModel {
                id: styleFlagsModel
                property bool ready: false
                onModelReset: {
                    ready = false;
                    while (styleFlagMenu.count > 0) {
                        var action = styleFlagMenu.actionAt(0);
                        styleFlagMenu.removeAction(action);
                    }

                    for (var row = 0; row < styleFlagsModel.rowCount(); row++) {
                        var index = styleFlagsModel.index(row, 0);
                        var key = styleFlagsModel.data(index, StyleFlagsModel.KeyRole);
                        var value = styleFlagsModel.data(index, StyleFlagsModel.ValueRole);
                        console.log("flag " + key+ ": " + value);
                        var action = menuAction.createObject(styleFlagMenu, { text: key, checked: value });
                        styleFlagMenu.addAction(action);
                    }

                    ready = true;
                    console.log("reset");
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
            SplitView.preferredWidth: parent.width*0.6
            SplitView.minimumWidth: 200
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
            SplitView.preferredWidth: parent.width*0.4
            map: map1
            source: map1.stylesheetFilename
        }
    }
}
