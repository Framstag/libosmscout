import QtQuick 2.0

Image{
    id: laneTurnIcon

    property string turnType: 'unknown'
    property string unknownTypeIcon: 'empty'
    property int roundaboutExit: -1
    property bool outline: false

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
        'through': 'through',
        'through;right': 'through_right',
        'through;slight_right': 'through_right',
        'through;sharp_right': 'through_right',
        'right': 'right',
        'slight_right': 'right',
        'merge_to_right': 'right'
    }

    function iconUrl(icon){
        return '../../pics/laneturn/' + icon + (outline ? '_outline' : '') + '.svg';
    }

    function typeIcon(type){
      if (typeof iconMapping[type] === 'undefined'){
          console.log("Can't find icon for type " + type);
          return iconUrl(unknownTypeIcon);
      }
      return iconUrl(iconMapping[type]);
    }

    source: typeIcon(turnType)

    fillMode: Image.PreserveAspectFit
    horizontalAlignment: Image.AlignHCenter
    verticalAlignment: Image.AlignVCenter

    sourceSize.width: width
    sourceSize.height: height

}
