---
date: "2017-06-04T15:40:00+02:00"
title:  "Basemap Importing"
description: "How to create a basemap db"
weight: 2

menu:
  main:
    Parent: "tutorials"
    Weight: 2
---
        
## Setup

For the basemap import you need the folling data:

* Shapefile containing world-wide OSm coastlines. You can download the 
shapefile from [*osmdata.openstreetmap.de*](https://osmdata.openstreetmap.de/data/coastlines.html).
Choose the WGS84 format. Unzip the resulting file and copy the *.shp file
as coastlines.shp into your maps directory.

Create a directory `world` in your maps directory.

## Calling the basemap importer

The map repository pipeline does this automatically: `scripts/mapgen/mapgen-basemap.sh`
imports the pre-filtered planet export with `stylesheets/basemap.ost`, generates the
water index from the coastline shapefile with this tool, and places the result in the
served repository as a version-keyed slot. Use the procedure below for a one-off run
outside the pipeline.

Call he basemap importer:

```bash
  $ ../BasemapImport/src/BasemapImport --destinationDirectory world --coastlines coastlines.shp
```

The Basemap importer will create a water.idx file in the world directory.
