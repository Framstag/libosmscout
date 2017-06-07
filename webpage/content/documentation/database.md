---
date: "2016-05-29T19:50:42+02:00"
title: "Database files"
description: "Description of the indivudal database files and their function"
weight: 8

menu:
  main:
    Parent: "documentation"
    Weight: 8
---

This document list the files that are processed as part of an import.
For each file its content is described. Later version of this document
should also describe the internal format of each of these files. The
text in the brackets after the files name give information if this file
is required during import only ("import only") or are also part of the
official export data ("export").

## Metadata

types.dat (export)
: Mapping of internal type ids to type names. Mapping of internal tag ids to
  tag names

bounding.dat (export)
: Holds the bounding rectangle of the preprocessed data

## Imported raw object data

rawnodes.dat (import only)
: Raw dump of all nodes in import data

rawnode.idx (import only)
: Index by numeric id over all nodes in 'rawnodes.dat'. Required
  for resolving nodes during way/area and relation preprocessing.

rawways.dat (import only)
: Raw dump of all ways and areas in import data

rawway.idx (import only)
: Index by numeric id over all ways/areas in 'rawways.dat'.
  Required for resolving nodes during relation preprocessing.

rawrels.dat (import only)
: Raw dump of all relations in import data

rawrel.idx (import only)
: Index by numeric id over all relations in 'rawrels.dat'.
  Required for resolving nodes during relation preprocessing.

rawcoastline.dat (import only)
: List of coastlines as parsed from import file

rawturnrestr.dat (import only)
: List of restrictions as parsed from rawrels.dat

## Temporary data files created during import

coord.dat (import only)
: map of node ids to coordinates 

turnrestr.dat (import only)
: List of restrictions modified by the Way data generator
  because of way merges.

wayblack.dat (import only)
: List of way ids that are already part of a outer member
  of a multipolygon region. This ids will not be part of
  area.idx, because they would then be drawn twice. Once as way
  and once as part of the relation.

nodes.tmp, relarea.tmp, wayarea.tmp, wayway.tmp (import only)
: Conversion of rawnodes.dat, rawways.dat and rawrels.dat to
  the export data structures.

areas.tmp, ways.tmp (import only)
: New version of *.tmp after nodes ids have been reduced to
  only those nodes that are referenced at least twice. 

turnrestr.dat (import only)
: Converted version of turn restrictions. Used during creation
  of routing specific export files.
 
wayareablack.dat (import only)
: List of areas ids that get blacklisted during relation
  parsing.

distribution.dat (import only)
: Contains information regarding number of nodes, ways and areas
  per type. USed during import for triggering import variants (depending
  on data size => low memeory fallbacks).

## Debugging information

nodes.idmap (debug only)
: Maps node ids to node file offset. Only used for debugging
  using DumpData.

areas.idmap (debug only)
: Maps ways and relations ids to area file offset. Only used
  for debugging using DumpData.

ways.idmap (debug only)
: Maps way ids to way file offset. Only used for debugging
  using DumpData.

## Exported object data

nodes.dat (export)
: All nodes that have a matching type during preprocessing.

areas.dat (export)
: All areas that have a matching type during preprocessing.

ways.dat (export)
: All ways that have a matching type during preprocessing.

## Object in area index

areanode.idx (export)
: Area index returning all nodes in a given bounding box
  that are of a given list of types and.

areaarea.idx (export)
: Area index returning all areas in a given bounding box
  that are of a given list of types and have at least
  a given size.

areaway.idx (export)
: Area index returning all ways in a given bounding box
  that are of a given list of types.

## Find object by name index

location.idx (export)
: Holds the location index.

location.txt (debug only)
: Dump of the internal location index

## Find objects by name (fultext search) index

Additional optional index file using maria ful text engine library

textloc.dat (export, optional)
: Index file holding names of locations

textpoi.dat (export, optional)
: Index file holding POI names

textregion.dat (export, optional)
: Index file holding names of regions

textother.dat (export, optional)
: Index file holding other objects having a name

## Is tile water or land index

water.idx (export)
: Index that for a given region returns tiles that are either
  of ground type 'water' or of ground type 'land' or ground type
  'coast'. Tiles of type coast also hold information about
  coastline within the tile and land regions within the tile.

## Optimized data for faster rendering in low zoom

areasopt.dat (export)
: Combined index and data file for returning (in the map.ost
  marked) areas in low zoom. Nodes are reduced
  to minimize the amount of data to load and render.

waysopt.dat (export)
: Combined index and data file for returning (in the map.ost
  marked) ways in low zoom. Ways are merged and nodes are reduced
  to minimize the amount of data to load and render.

## Routing

(if you create vehicle-specific routing data - which is the
default - actual file names follow the pattern routeXXX.*,
where XXX is the vehicle name)

route.dat (export)
: Contains the route graph.

route2.dat (export)
: Contains additional data for the route graph.

route.idx (export)
: Contains the index over the route graph to load individual
  routing nodes by id.

intersections.dat (export)
: Contains information about which objects (areas and ways) 
  are part of a routing relevant (route node at intersection point)
  intersections.
