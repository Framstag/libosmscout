import QtQuick 2.0

Image{
    id: laneTurnIcon

    property string turnType: 'unknown'
    property bool suggested: false
    property string suggestedTurn: 'unknown'
    property string unknownTypeIcon: 'empty'
    property int roundaboutExit: -1

    /*
     * This is mapping libosmscout route step types and step icons.
     */
    property variant iconMapping: {
        '': 'empty',
        'left': 'left',
        'slight_left': 'left',
        'merge_to_left': 'left',

        'through;left': 'through_left',
        'through;slight_left': 'through_left',
        'through;sharp_left': 'through_left',

        'through;left-through': 'through_left-through',
        'through;slight_left-through': 'through_left-through',
        'through;sharp_left-through': 'through_left-through',

        'through;left-left': 'through_left-left',
        'through;slight_left-slight_left': 'through_left-left',
        'through;sharp_left-sharp_left': 'through_left-left',

        'through': 'through',

        'through;right': 'through_right',
        'through;slight_right': 'through_right',
        'through;sharp_right': 'through_right',

        'through;right-through': 'through_right-through',
        'through;slight_right-through': 'through_right-through',
        'through;sharp_right-through': 'through_right-through',

        'through;right-right': 'through_right-right',
        'through;slight_right-slight_right': 'through_right-right',
        'through;sharp_right-sharp_right': 'through_right-right',

        'right': 'right',
        'slight_right': 'right',
        'merge_to_right': 'right'
    }

    function iconUrl(icon){
        return '../../pics/laneturn/' + icon + (suggested ? '' : '_outline') + '.svg';
    }

    function typeIcon(type){
      console.log("turn icon " + turnType + " " + (suggested ? ("suggested: " + suggestedTurn) : "not-suggested"))
      var icon = iconMapping[turnType];
      if (typeof icon === 'undefined'){
          console.log("Can't find icon for type " + turnType);
          return iconUrl(unknownTypeIcon);
      }
      if (suggested) {
          console.log("mapping: " + turnType + '-' + suggestedTurn)
          var suggestedTurnIcon = iconMapping[turnType + '-' + suggestedTurn];
          if (typeof suggestedTurnIcon !== 'undefined'){
              icon = suggestedTurnIcon;
          }
      }

      return iconUrl(icon);
    }

    source: typeIcon()

    fillMode: Image.PreserveAspectFit
    horizontalAlignment: Image.AlignHCenter
    verticalAlignment: Image.AlignVCenter

    sourceSize.width: width
    sourceSize.height: height

}
