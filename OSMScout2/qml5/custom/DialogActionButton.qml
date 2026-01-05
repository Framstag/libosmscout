import QtQuick 2.2

import net.sf.libosmscout.map 1.0

Rectangle {
  id: dialogActionButton
  
  property color contentColor: "lightblue"
  property color contentHoverColor: Qt.darker(contentColor, 1.1)
  property color borderColor: Qt.darker(contentColor, 1.1)
  property color textColor: "black"

  property alias text: label.text
  
  signal clicked
  
  width: label.implicitWidth+4
  height: label.implicitHeight+4
  color: contentColor
  border.color: borderColor
  border.width: 1

  MouseArea {
    id: mouseArea
    anchors.fill: parent
    
    hoverEnabled: true
    onEntered: {
      parent.color = contentHoverColor
    }
    
    onExited:  {
      parent.color = contentColor
    }
    
    onClicked: {
      parent.clicked()
    }
  }
  
  Text {
    id: label
    font.pixelSize: Theme.textFontSize
    anchors.centerIn: parent
    color: textColor
  }
}
