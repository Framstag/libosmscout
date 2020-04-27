---
date: "2017-10-15T11:00:00+02:00"
title: "Qt QML API"
description: "Description of the Qt QML API"
weight: 14

menu:
  main:
    Parent: "documentation"
    Weight: 14
---

Part of this project is **libosmscout-client-qt** library that aims to provide 
simple API for applications using user interface in [QML](http://doc.qt.io/qt-5/qmlapplications.html).

This page show usage of basic QML components that may helps you create your own application 
with mapping capabilities, like displaying map, search places, routing... Following examples
are using [Silica UI components](https://sailfishos.org/develop/docs/silica/), but code 
for other UI components (like [QtQuick components](https://wiki.qt.io/Qt_Quick_Components)), 
will be very similar.  

## Initialisation

Before using library from QML it is necessary to initialise it from C++.

```c++
OSMScoutQt::RegisterQmlTypes();

bool success=OSMScoutQt::NewInstance()
     .WithBasemapLookupDirectory(basemapDir)
     .WithStyleSheetDirectory(stylesheetDir)
     .WithStyleSheetFile(stylesheetFileName)
     .WithIconDirectory(iconDirectory)
     .WithMapLookupDirectories(mapLookupDirectories)
     .WithOnlineTileProviders(":/resources/online-tile-providers.json")
     .WithMapProviders(":/resources/map-providers.json")
     .Init();

if (!success){
  // terminate program, or just report error - something is really bad
  
}

// now it is possible to create QML window that is using the library

// release resources

OSMScoutQt::FreeInstance();
```
For detailed description see [OSMScoutQtBuilder class](/api-doc/html/classOSMScoutQtBuilder.html)
and practival usage [OSMScout2 demo source code](https://github.com/Framstag/libosmscout/blob/master/OSMScout2/src/OSMScout.cpp).

To make components awailable in QML, you have to import it: 
```qml
import net.sf.libosmscout.map 1.0
``` 

*Note: when you are writting application for Sailfish OS (SFOS) and do you plan to distribute
it via Harbour (official SFOS store), [it is neccessary](https://harbour.jolla.com/faq#QML_API) 
to change component uri to match the name of the application: 
`OSMScoutQt::RegisterQmlTypes("harbour.<app-name>.map", 1, 0);` Imported uri will be different
then, but another code will be working without change.*

## Displaying map

When you want to display a map, it is very simple. Just use Map UI component:

```qml
Map {
  id: map
  renderingType: "tiled" // plane (default) or tiled
}
```
For all signals and slots, see [MapWidget](/api-doc/html/classMapWidget.html).
You can choose one of two rendering types:

 * **plane** (default) - complete scene is re-rendered on every change of the view.
   Component may display partial (unfinished) rendering result, when rendering 
   is not fast enough - it provides more responsive experience. 
   This kind of rendering reduce rendering errors on tile boundary, 
   but it don't support combination of online tiles with offline rendering (yet). 
   It fully supports map rotating, you can rotate map to device direction obtained from compass sensor.
 * **tiled** - map is combined from tiles downloaded from some online web service 
   ([openstreetmap.org](https://openstreetmap.org) for example) and tiles rendered offline
   by library. This allows to render detailed map for whole world and keep benefits 
   of offline rendering for areas covered by downloaded databases. It supports map rotating,
   but labels are rotated too. Rendering may be more responsive, due to tile caching in memory.
 
### User input

Map component react on user touches with moving (or mouse drag) and
zooming (multitouch gesture, double-click or scrolling of mouse wheel). Actions on simple 
click, or "holding" should be defined by application.
```qml
Map{
    onTap: {
        console.log("tap: " + screenX + "x" + screenY + " @ " + 
                    lat + " " + lon + " (map center "+ 
                    map.view.lat + " " + map.view.lon + ")");
    }
    onLongTap: {
        console.log("long tap: " + screenX + "x" + screenY + " @ " + 
                    lat + " " + lon + " (map center " + 
                    map.view.lat + " " + map.view.lon + ")");
    }
}
```

### Display custom marks and ways

Map component supports to display few kinds of overlay objects.

#### Current position mark

Just allow it by `showCurrentPosition` property 
and update current position by `locationChanged` slot:

```qml
Map{
    id: map
    showCurrentPosition: true

    PositionSource {
        id: positionSource
        active: true
        onPositionChanged: {
            map.locationChanged(
                      position.latitudeValid && position.longitudeValid,
                      position.coordinate.latitude, position.coordinate.longitude,
                      position.horizontalAccuracyValid, position.horizontalAccuracy);
        }
    }
}
```
It will display small green dot (or red when position is not valid) with semitransparent 
circle representing horizontal accuracy:

<a href="/images/qt-current-position.png"><img src="/images/qt-current-position.png" width="460" alt="Current position mark"/></a>
  
#### Place mark

You can custom place mark by `addPositionMark` and remove it by `removePositionMark`.

```qml
Map{
  id: map
  property int markId: 0;
  onTap: {
      map.addPositionMark(markId++, lat, lon);
      if (markId>=10){
          map.removePositionMark(markId-10);
      }
  }
}
```

  It will display red circle around position:
  
  <a href="/images/qt-place-mark.png"><img src="/images/qt-place-mark.png" width="460" alt="Place mark"/></a>
  
#### Custom way, area or node

This overlay type is used for displaying routing result for example, but it may be used 
for displaying any custom object with any style defined in [stylesheet](/documentation/stylesheet/).

```qml
Map{
  property var overlayWay: map.createOverlayWay("_route");
  onTap: {
    overlayWay.addPoint(lat, lon);
    map.addOverlayObject(0, overlayWay);
  }
}
```

  <a href="/images/qt-custom-way.png"><img src="/images/qt-custom-way.png" width="460" alt="Place mark"/></a>

Overlay types that don't exists in database, should be defined on library startup 
by `OSMScoutQtBuilder::AddCustomPoiType` method.

### Custom map overlays

For some kind of applications, custom map objects don't cover all requirements. 
For example when you want to display hill shades, traffic intensity or some kinds of heatmap, 
you can create own QML component with [`MapOverlay`](/api-doc/html/classMapOverlay.html) 
as base class and implement `void paint(QPainter *painter)` method to access full Qt rendering api.

*Note: `paint` method is called in context of UI thread, execute CPU intensive tasks in 
this thread will cause UI lags!*

You can take inspiration in [`TiledMapOverlay`](/api-doc/html/classTiledMapOverlay.html) 
class that provides simple overlay based on online tile services. 
For example pre-rendered hill shades.

```qml
Map {
  id: map
  
  TiledMapOverlay {
      anchors.fill: parent
      view: map.view
      enabled: true
      opacity: 0.6
      // If you intend to use tiles from OpenMapSurfer services in your own applications please contact us.
      // https://korona.geog.uni-heidelberg.de/contact.html
      provider: {
            "id": "ASTER_GDEM",
            "name": "Hillshade",
            "servers": [
              "https://korona.geog.uni-heidelberg.de/tiles/asterh/x=%2&y=%3&z=%1"
            ],
            "maximumZoomLevel": 18,
            "copyright": "© IAT, METI, NASA, NOAA",
          }
  }
}
```

  <a href="/images/qt-hill-shades.png"><img src="/images/qt-hill-shades.png" width="460" alt="OSM Scout map with OpenMapSurfer hill shades overlay"/></a>

### QML overlay

There is also possible to use QML Canvas type for overlay.
It is easy for proof of concept and may be suitable for some usecases.

```qml
Map {
  id: map

  Canvas {
    id: mapCanvas
    anchors.fill: parent

    Connections {
      target: map
      onViewChanged: {
          mapCanvas.requestPaint();
      }
    }

    onPaint: {
      try{
        var ctx = getContext("2d");

        ctx.clearRect(0, 0, mapCanvas.width, mapCanvas.height);

        ctx.lineWidth = 4;
        ctx.strokeStyle = "blue";
        ctx.fillStyle = Qt.rgba(0, 0, 0.8, 0.2);

        var p1 = map.screenPosition(50.4253, 14.5602);
        var p2 = map.screenPosition(50.4099, 14.5816);
        var p3 = map.screenPosition(50.4304, 14.6022);

        ctx.beginPath();
        ctx.moveTo(p1.x, p1.y);
        ctx.lineTo(p2.x, p2.y);
        ctx.lineTo(p3.x, p3.y);
        ctx.closePath();
        ctx.fill();

        ctx.stroke();
      }catch(e){
        console.log("error: "+e);
      }
    }
  }
}
```

<a href="/images/qt-canvas-overlay.png"><img src="/images/qt-canvas-overlay.png" width="460" alt="OSM Scout map with QML Canvas overlay"/></a>

## Location info

`LocationInfoModel` provides description for place on map based on nearest POI object 
or address point. It it usual Qt model that may be presented by some `ListView`.  

```qml
LocationInfoModel {
    id: locationInfoModel
    Component.onCompleted: {
        locationInfoModel.setLocation(50.08923, 14.49837);
    }
}
SilicaListView {
    id: locationInfoView
    model: locationInfoModel
    delegate: Column{
        Label {
            id: entryPoi
            text: poi
            visible: poi != ""
        }
        Label {
            id: entryAddress
            text: address
            visible: address != ""
        }
    }
}
```
Possible result:

  <a href="/images/qt-place-descriptin.png"><img src="/images/qt-place-descriptin.png" width="460" alt="Place description"/></a>

Detailed description in [LocationInfoModel class](/api-doc/html/classLocationInfoModel.html).

## Search place

`LocationListModel` provides search in location (address) index and "fulltext" index of named objects.
It is usual Qt model that may be presented by some `ListView`.

```qml
LocationListModel {
  id: suggestionModel
  Component.onCompleted: {
    suggestionModel.setPattern("Praha");
  }
}
SilicaListView {
  id: suggestionView
  model: suggestionModel
  delegate: Label {
    text: label
  }
}
```

Detailed description in [LocationListModel class](/api-doc/html/classLocationListModel.html).

## Search POI

`NearPOIModel` provide lookup of database objects (not limited to POI) by given coordinates and types.
It is usual Qt model that may be presented by some `ListView`.

```qml
NearPOIModel {
  id: poiModel
  lat: 50.0923
  lon: 14.4827
  maxDistance: 1000
  types: ["amenity_restaurant", "amenity_restaurant_building"]
}
SilicaListView {
  id: suggestionView
  model: poiModel
  delegate: Label {
    text: label
  }
}
```

Detailed description in [NearPOIModel class](/api-doc/html/classNearPOIModel.html).

## Routing

```qml
RoutingListModel{
  id: route
  Component.onCompleted: {
    var from=route.locationEntryFromPosition(50.0920, 14.4936);
    var to=route.locationEntryFromPosition(49.1949, 16.6345);
    route.setStartAndTarget(from, to);
  }
  onComputingChanged: {
    if (route.count>0){
        console.log("route length: " + route.length + " m");
        console.log("route commands: " + route.count);
        map.addOverlayObject(0,route.routeWay);
    }
  }
}
Map{
  id: map
}  
```

  <a href="/images/qt-route.png"><img src="/images/qt-route.png" width="460" alt="Computed route"/></a>

Detailed description in [RoutingListModel class](/api-doc/html/classRoutingListModel.html).


## Behind the scene

Client Qt library consists from three layers: 

<a href="/images/QtApi.svg"><img src="/images/QtApi.png" width="800" alt="Qt API structure"/></a>

QML components provides just limited set of capabilities now. But you may create your own data models
or UI components easily. Just dive into source code and don't worry to ask us on [mailing list](https://sourceforge.net/p/libosmscout/mailman/). 

