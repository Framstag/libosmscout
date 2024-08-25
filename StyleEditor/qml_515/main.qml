import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1

import net.sf.libosmscout.map 1.0

import "custom"

ApplicationWindow {
    id: mainWindow
    color: "white"
    title: "OSMScout style editor"
    visible: true
    minimumWidth: units.gu(100)
    minimumHeight: units.gu(75)
    width: units.gu(150)
    height: units.gu(100)

    Units {
        id: units
    }

    SplitView {
        id: mainRow
        anchors.fill: parent
        orientation: Qt.Horizontal

        Item {
            SplitView.preferredWidth: parent.width * 0.50
            SplitView.minimumWidth: units.gu(25)
            height: parent.height

            Rectangle {
                id: menuBar
                width: parent.width
                height: units.gu(5)

                MenuBar {
                    anchors.fill: parent
                    Menu {
                        title: "File"
                        MenuItem {
                            text: "Close"
                            action: Action {
                                onTriggered: {
                                    mainWindow.close();
                                }
                            }
                        }
                        MenuItem {
                            text: "About"
                            action: Action {
                                onTriggered: {
                                    about.open();
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

                Rectangle {
                    width: parent.width
                    anchors.bottom: parent.bottom
                    height: units.dp(1)
                    color: "#ddd"
                }
            }

            Grid {
                width: parent.width
                anchors.top: menuBar.bottom
                anchors.bottom: parent.bottom
                Layout.fillWidth: true
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
        }

        TextEditor {
            id: textEditor
            SplitView.preferredWidth: parent.width * 0.50
            SplitView.minimumWidth: units.gu(72)
            map: map1
            source: map1.stylesheetFilename;
        }
    }

    AboutDialog {
        id: about
    }

    DialogAction {
        id: quitDialog
        actionName: qsTr("Quit")
        title: qsTr("Style file is not saved !")
        text: qsTr("Quit anyway ?")

        Component.onCompleted: {
            quitDialog.reply.connect(function(accepted){
                quitDialog.visible = false;
                if (accepted) {
                    // HACK: Qt.quit() is unusable from qt6
                    checkSaved = false;
                    mainWindow.close();
                }
            })
        }
    }

    property bool checkSaved: true

    onClosing: function(event) {
        if (checkSaved && textEditor.modified) {
            event.accepted = false;
            quitDialog.open();
        } else {
            event.accepted = true;
        }
    }
}
