import QtQuick 2.12
import QtQuick.Controls 2.2

Dialog {
    id: dialog

    /*!
      The property holds the margins from the dialog's dismissArea.
      */
    property real edgeMargins: units.gu(2)

    /*!
      The property controls whether the dialog is modal or not. Modal dialogs block
      event propagation to items under dismissArea, when non-modal ones let these
      events passed to these items. In addition, non-modal dialogs do not dim the
      dismissArea.

      The default value is true.
      */
    modal: true

    /*!
      Grab focus when Dialog is shown
      */
    focus: true

    /*!
      The question to the user.
      \qmlproperty string text
     */
    property string text

    property real minimumWidth: units.gu(40)
    property real minimumHeight: Math.max(160, units.gu(20))
    property real contentSpacing: units.gu(1)

    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - (height + header.height + footer.height)) / 2)
    width: Math.max(Math.round(Math.min(parent.width, parent.height) / 3 * 2), dialog.minimumWidth)
    readonly property real h: contentsColumn.height + units.gu(6) +
                              dialog.header.height + dialog.footer.height
    height: Math.max(Math.min(h, Math.round(parent.height / 4 * 3)), dialog.minimumHeight)

    Rectangle {
        id: background
        color: "transparent"
        anchors.fill: parent
        anchors.margins: dialog.edgeMargins
    }

    Flickable {
        anchors.fill: parent
        anchors.leftMargin: dialog.edgeMargins
        contentWidth: contentsColumn.width
        contentHeight: contentsColumn.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            visible: (parent.visibleArea.heightRatio < 1.0)
        }

        Column {
            id: contentsColumn
            spacing: dialog.contentSpacing
            width: background.width
            onWidthChanged: updateChildrenWidths();

            Text {
                horizontalAlignment: Text.AlignHCenter
                text: dialog.text
                font.pixelSize: units.fs("large")
                wrapMode: Text.Wrap
            }

            onChildrenChanged: updateChildrenWidths()

            function updateChildrenWidths() {
                for (var i = 0; i < children.length; i++) {
                    children[i].width = contentsColumn.width;
                }
            }
        }
    }
}
