import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 1.4
import QtQuick.Window 2.0

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"

Window {
    id: mainWindow
    objectName: "main"
    title: "OSMScout"
    visible: true
    width: 480
    height: 800

    TreeView {
        anchors.fill: parent

        TableViewColumn {
            title: "Name"
            role: "name"
            width: 200
        }
        TableViewColumn {
            title: "Size"
            role: "size"
            width: 80
        }
        TableViewColumn {
            title: "Description"
            role: "description"
            width: 300
        }
        headerVisible: true

        AvailableMapsModel{
            id: availableMapsModel
        }
        rowDelegate: Rectangle{
            color: styleData.selected ? "#3030ff": (styleData.alternate?"#303030": "#202020")
        }

        itemDelegate: Item {
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: styleData.textColor
                elide: styleData.elideMode
                text: styleData.value
            }
        }

        backgroundVisible: true
        alternatingRowColors: true
        //color: SystemPaletteSingleton.window(true)
        model: availableMapsModel

        style: TreeViewStyle{
            backgroundColor: "#181818"
            //highlightedTextColor : "#ffff00"
            //frame: Rectangle {color: "blue"}
            handle: Rectangle {color: "blue"}
        }
    }
}
