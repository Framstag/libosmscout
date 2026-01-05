import QtQuick 2.2

import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 1.4
import QtQuick.Window 2.0

import QtPositioning 5.2

import net.sf.libosmscout.map 1.0

import "custom"

MapDialog {
    id: voiceDownloadDialog
    label: "Voice Downloader"
    fullscreen: parent.width<500 || parent.height<600


    AvailableVoicesModel {
        id: availableVoices
    }

    content : ColumnLayout {
        spacing: Theme.mapButtonSpace

        Component.onCompleted: {
            console.log("content dimensions: "+width+"x"+height)
        }

        Text {
            id: disclaimerText
            width: parent.width
            Layout.minimumWidth: Math.min(voiceDownloadDialog.width * 0.8, 500)

            text: "Voice samples were created as part of <a href=\"https://community.kde.org/Marble/VoiceOfMarble\">VoiceOfMarble</a> project. " +
                  "Licensed under terms of <a href=\"https://creativecommons.org/licenses/by-sa/3.0/\">CC BY-SA 3.0</a> license."
            onLinkActivated: Qt.openUrlExternally(link)

            font.pixelSize: Theme.textFontSize
            wrapMode: Text.WordWrap
            //horizontalAlignment: Text.AlignHCenter
        }
        TableView {
            id: downloadsTable

            Layout.minimumWidth: Math.min(voiceDownloadDialog.width * 0.8, 500)
            Layout.minimumHeight: Math.min(voiceDownloadDialog.height * 0.5, 400)
            height: 400

            model: availableVoices

            style: TableViewStyle{
                backgroundColor: "#f0f0f0"
                highlightedTextColor : "#ffffff"
                textColor: "#000000"
                //frame: Rectangle {color: "blue"}
                handle: Rectangle {color: "blue"}
            }

            TableViewColumn {
                role: "state"
                title: "State"
                width: 100
                delegate: Text {
                    text: model.state == AvailableVoicesModel.Downloaded ? "Downloaded" :
                        (model.state == AvailableVoicesModel.Downloading? "Downloading...": "Not downloaded")
                }
            }

            TableViewColumn {
                role: "lang"
                title: "Language"
                width: 200
            }
            TableViewColumn {
                role: "name"
                title: "Name"
            }

            function getData(row, role){
                return availableVoices.data(availableVoices.index(row, 0), role);
            }

            onDoubleClicked: {

                var state = getData(row, AvailableVoicesModel.StateRole);
                var indexObj = availableVoices.index(row, 0);
                console.log("Selected: " + row+ " state: "+ state);
                if (state == AvailableVoicesModel.Downloaded || state == AvailableVoicesModel.Downloading){
                    console.log("remove: " + row);
                    availableVoices.remove(indexObj);
                } else {
                    console.log("download: " + row);
                    availableVoices.download(indexObj);
                }

            }
        }

        Text {
            //Layout.fillWidth: true
            //width: treeView.width

            text: "Double click on some voice for downloading or removing it."
            font.pixelSize: Theme.textFontSize
            horizontalAlignment: Text.AlignHCenter
        }

    }
}
