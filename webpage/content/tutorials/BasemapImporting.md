---
date: "2017-06-04T15:40:00+02:00"
title:  "Basemap Importing"
description: "How to create a basemap database"
weight: 2

menu:
  main:
    Parent: "tutorials"
    Weight: 2
---
        
## Setup

For the basemap import you need the folling data:

* Shapefile containing world-wide OSm coastlines. You can download the 
shapefile from [*openstreemapdata.com*](http://openstreetmapdata.com/data/coastlines).
Choose the WGS84 format. Unzip the resulting file and copy the *.shp file
as coastlines.shp into your maps directory.

Create a directory `world` in your maps directory.

## Calling the basemap importer

Call he basemap importer:

```bash
  $ ../BasemapImport/src/BasemapImport --destinationDirectory world --coastlines coastlines.shp
```

The Basemap importer will create a water.idx file in the world directory.
