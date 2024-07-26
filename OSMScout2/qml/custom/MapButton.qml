import QtQuick 2.7
import QtQuick.Controls 2.7
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

import net.sf.libosmscout.map 1.0

Button {
  id: mapButton

    display: AbstractButton.TextOnly

    font.pixelSize: Theme.mapButtonFontSize
    //font: mapButtonLabel.font

    background: Rectangle {
        opacity: 0.3
        border.color: black
        border.width: 1
        color: parent.checked ? "#ff0000" :
            (parent.down ? "#555555" :
                (parent.hovered ? "#aaaaaa" : "#ffffff"))
    }

    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredWidth: Theme.mapButtonHeight
    Layout.preferredHeight: Theme.mapButtonHeight

    scale: down ? 1.2 : 1.0

    Behavior on scale {
        NumberAnimation {
            duration: 55
        }
    }
}
