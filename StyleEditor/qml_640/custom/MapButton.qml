import QtQuick 2.12
import Qt5Compat.GraphicalEffects 6.0

Rectangle {
  id: mapButton

  property color defaultColor: "white"
  property color hoverColor: Qt.darker(defaultColor, 1.1)
  property string label

  signal clicked

  width: 25
  height: 25
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
    text: label
  }

  scale: mapButtonMouseArea.pressed ? 1.2 : 1.0

  Behavior on scale {
    NumberAnimation {
      duration: 55
    }
  }
}
