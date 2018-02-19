import QtQuick 2.0

Image{
    id: routeStepIcon

    property string stepType: 'unknown'
    property string unknownTypeIcon: 'information'

    /*
     * This is mapping libosmscout route step types and step icons.
     */
    property variant iconMapping: {
        'information': 'information',

        'start': 'start',
        'drive-along': 'drive-along',
        'target': 'target',

        'turn': 'turn',
        'turn-sharp-left': 'turn-sharp-left',
        'turn-left': 'turn-left',
        'turn-slightly-left': 'turn-slightly-left',
        'continue-straight-on': 'continue-straight-on',
        'turn-slightly-right': 'turn-slightly-right',
        'turn-right': 'turn-right',
        'turn-sharp-right': 'turn-sharp-right',

        'enter-roundabout': 'enter-roundabout',
        'leave-roundabout': 'leave-roundabout',

        'enter-motorway': 'enter-motorway',
        'change-motorway': 'change-motorway',
        'leave-motorway': 'leave-motorway',

        'name-change': 'information'
    }

    function iconUrl(icon){
        return '../../pics/routestep/' + icon + '.svg';
    }

    function typeIcon(type){
      if (typeof iconMapping[type] === 'undefined'){
          console.log("Can't find icon for type " + type);
          return iconUrl(unknownTypeIcon);
      }
      return iconUrl(iconMapping[type]);
    }

    source: typeIcon(stepType)

    fillMode: Image.PreserveAspectFit
    horizontalAlignment: Image.AlignHCenter
    verticalAlignment: Image.AlignVCenter

    sourceSize.width: width
    sourceSize.height: height

}
