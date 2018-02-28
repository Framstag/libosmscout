import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 1.4
import QtQuick.Window 2.0

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: mapDownloadDialog
    label: "Map Downloader"
    fullscreen: parent.width<500 || parent.height<600

    MapDownloadsModel{
        id:mapDownloadsModel
    }

    //Layout.maximumWidth: Math.max(mapDownloadDialog.width * 0.9, 800)
    //Layout.maximumHeight: mapDownloadDialog.height * 0.9

    content : ColumnLayout {
        spacing: Theme.mapButtonSpace

        Item{
            width: parent.width
            height: disclaimerText.contentHeight

            Component.onCompleted: {
                console.log("dimensions: "+parent.width+"x"+height)
            }

            Text {
                id: disclaimerText
                width: parent.width
                x:0
                y:0

                text: "Server uses http cookies for collecting downloads statistics.<br/>We promise that application will not send<br/>your location or other sensitive data."
                font.pixelSize: Theme.textFontSize
                wrapMode: Text.WordWrap
                //horizontalAlignment: Text.AlignHCenter
            }
        }

        TreeView {
            id: treeView
            //anchors.fill: parent
            Layout.minimumWidth: Math.min(mapDownloadDialog.width * 0.8, 500)
            Layout.minimumHeight: Math.min(mapDownloadDialog.height * 0.5, 400)

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
            TableViewColumn {
                title: "Last Update"
                role: "time"
                width: 200
            }

            headerVisible: true

            AvailableMapsModel{
                id: availableMapsModel
            }
            rowDelegate: Rectangle{
                color: styleData.selected ? "#5050ff": (styleData.alternate?"#e0e0e0": "#f0f0f0")
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
                backgroundColor: "#f0f0f0"
                highlightedTextColor : "#ffffff"
                textColor: "#000000"
                //frame: Rectangle {color: "blue"}
                handle: Rectangle {color: "blue"}
            }

            onDoubleClicked: {
                if (index) {
                    //console.log(index.parent.row, index.row)
                    var map=availableMapsModel.map(index);
                    if (map){
                        var dir=mapDownloadsModel.suggestedDirectory(map);
                        mapDownloadsModel.downloadMap(map, dir)
                    }
                }
            }
        }

        Text {
            //Layout.fillWidth: true
            //width: treeView.width

            text: "Double click on some map, downloading will start immediately."
            font.pixelSize: Theme.textFontSize
            horizontalAlignment: Text.AlignHCenter
        }

        Text{
            text: "Downloads"
            font.bold: true
        }

        TableView {
            id: downloadsTable

            Layout.minimumWidth: Math.min(mapDownloadDialog.width * 0.8, 500)
            height: 60

            model: mapDownloadsModel

            style: TableViewStyle{
                backgroundColor: "#f0f0f0"
                highlightedTextColor : "#ffffff"
                textColor: "#000000"
                //frame: Rectangle {color: "blue"}
                handle: Rectangle {color: "blue"}
            }

            TableViewColumn {
                role: "mapName"
                title: "Map"
                width: 100
            }
            TableViewColumn {
                //role: "progressRole"
                title: "Progress"
                delegate: Text {
                    text: (model!=null) ? Math.round(model.progressRole * 100)+" %" : "?"
                    //font.pointSize: 20
                }
                width: 30
            }
            TableViewColumn {
                title: "Downloading"
                delegate: Text {
                    text: (model!=null) ? (model.errorString!="" ? model.errorString : model.progressDescription) : ""
                }
                width: 100
            }
            TableViewColumn {
                role: "targetDirectory"
                title: "Target Directory"
                width: 300
            }
        }

    }
}
