
# OSMScout2 

This is demo application that shows basic capabilities of osmscout library.

## Build

You can find detailed instruction how to build libraries and demo applications
on project webpage:
http://libosmscout.sourceforge.net/documentation/source/

## Configure renderer

Currently, Qt API (libosmscout-client-qt) supports two rendering methods: 

 * **Plane** rendrering. Complete canvas is rendered at once, there are no artefacts
  on tile boundaries, there is no image scaling issues. It's support for multiple 
  maps (databases) is limited. This method is used when `DBThread` instance is 
  initialized by `DBThread::InitializePlaneInstance` method.
 * **Tile** rendering. Canvas is composite from tiles that are cached in memory
  and it may be rendered separately. It combine tiles rendered from offline 
  map (database) and some online source. It has better support for multiple databases.
  This method is used when `DBThread` instance is initialized 
  by `DBThread::InitializeTiledInstance` method.

### Configure online tile source

Online tile providers are loaded on application startup 
from `resources/online-tile-providers.json`. It is JSON array of objects that
describing tile sources. Only providers with `256px` width tiles 
are supported currently.

Example for OSM Mapnik source:
```
[
  {
    "id": "mapnik",
    "name": "OSM Mapnik",
    "servers": [
      "https://a.tile.openstreetmap.org/%1/%2/%3.png",
      "https://b.tile.openstreetmap.org/%1/%2/%3.png",
      "https://c.tile.openstreetmap.org/%1/%2/%3.png"
    ],
    "maximumZoomLevel": 19, 
    "copyright": "© OpenStreetMap contributors"
  }
]
```

## Offline maps

### Build custom map database

OSMScout project provides `Import` tool, that can be used for converting raw OSM
data to internal database format. Detailed instructions are available on project
website: http://libosmscout.sourceforge.net/tutorials/Importing/

## Download prepared map database

We (osmscout developers) provide prepared map databases for some regions, 
you can download it directly from OSMScout2 application. Map providers are loaded
on application startup from `resources/map-providers.json` file. 
It is JSON Array with objects describing map providers:

```
[
  {
    "uri": "https://osmscout.karry.cz",
    "listUri": "https://osmscout.karry.cz/latest.php?fromVersion=%1&toVersion=%2&locale=%3",
    "name": "karry.cz"
  }
]
```

Server should provide list of available maps in JSON format on `listUri` address.
This address may have three arguments: what database versions should be listed 
(`fromVersion`, `toVersion`) and client's `locale` for localise names.

Map list sample:
```
[
 {
   "version" : 10,
   "timestamp" : 1480801927,
   "name" : "Czech Republic",
   "directory" : "europe/czech-republic-10-20161203",
   "size" : 622036876,
   "map" : "europe/czech-republic"
 },
 {
   "dir" : "europe",
   "name" : "Europe"
 }
]
```
Map should be available on provider's server/CDN (`uri` in provider object) 
with relative path `directory` from map object. 
So, `europe/czech-republic` map from previous sample can be downloaded from: 
https://osmscout.karry.cz/europe/czech-republic-10-20161203

You can find server scripts (in PHP) in separate git repository: 
https://github.com/Karry/osmscout-scripts/tree/master/web

