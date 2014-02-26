import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.1

Rectangle {
    id: mainWindow
    objectName: "main"
    color: "white"

    Rectangle {
        id: mainRow
        anchors.fill: parent

        Grid {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: parent.width*0.6
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
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: parent.width*0.4
            map: map1
            source: map1.stylesheetFilename
        }
    }
}
