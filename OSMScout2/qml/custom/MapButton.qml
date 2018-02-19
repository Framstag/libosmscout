import QtQuick 2.2
import QtGraphicalEffects 1.0

import net.sf.libosmscout.map 1.0

Rectangle {
  id: mapButton
  
  property color defaultColor: "white"
  property color hoverColor: Qt.darker(defaultColor, 1.1)
  property string label

  property alias font: mapButtonLabel.font

  signal clicked
  
  width: Math.max(Theme.mapButtonWidth, mapButtonLabel.width + Theme.horizSpace*2)
  height: Theme.mapButtonHeight
  color: defaultColor
  border.color: "grey"
  border.width: 1
  opacity: 0.8
  
  
  MouseArea {
    id: mapButtonMouseArea
    anchors.fill: parent
    
    hoverEnabled: true
    onEntered: {
      parent.color = hoverColor
    }
    
    onExited:  {
      parent.color = defaultColor
    }
    
    onClicked: {
      parent.clicked()
    }
  }
  
  Text {
    id: mapButtonLabel
    anchors.centerIn: parent
    color: "black"
    font.pixelSize: Theme.mapButtonFontSize
    text: label
  }
  
  scale: mapButtonMouseArea.pressed ? 1.2 : 1.0
  
  Behavior on scale {
    NumberAnimation { 
      duration: 55
    }
  }
}
