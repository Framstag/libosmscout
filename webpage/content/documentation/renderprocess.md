---
date: "2016-08-10T19:48:00+02:00"
title: "Render process"
description: "From Database to map: Description of the render process"
weight: 12

menu:
  main:
    Parent: "documentation"
    Weight: 12
---

The following chapters give an overview starting from data loading via the
`Database` to the rendered map using the `MapPainter`.

This initial description will be incomplete, descriptions and chapters
will be added on demand.

## Index and data files

On the lowest level the databse consists of a number of data and index files,
in some cases a file contains both an index and the actual data. For a detailed
description of the individual files see
[Database files]({{< relref "documentation/database.md" >}}).

For each file an corresponding C++ class exists that wraps access to these
files.

For rendering mainly the files that contain information regarding nodes, ways, 
areas and water/sea/coastline are relevant. In the following these are
summerized as "map data".

## The `Database`

The `Database` class wraps most of the above files, controlling initialisation,
deinitialsation and access to these files. `Database` is mainly a convinience
wrapper to the low level data and index classes.

## Characteristics of map data as retrieved the Database

The database allows you to retrieve map data for each object type (node, way,
area) individually and let you control which data types are retrieved.

Database allows you to retrieve data in a given rectangular area (as seen
from the coordinates. On ground and depending of the used projection the area
is not rectangular at all).

For the returned data the following statements are valid:

* If you select data for a given data type (like "city" or "motorway") only
  objects of these types are returned.
* You always get all objects of this type in the given bounding box, though
  for areas the returned data is filtered by its size.
* If the object is a way or an area you will even get all objects that intersect
  the given bounding box.
* The database internally makes heavy use of tiles. So while you get all in the
  given bounding box, you may get more data, that is outside of the given
  bounding box. Depending on the data type the effective bounding box may
  even be different.
* In result if you select data for disjunct bounding boxes you must still be
  prepared hat both results sets shared the same data, since internally the
  effective bounding boxes may still interleave.
* You get all the data that is in or intersecting the bounding box based on its
  covered area. Note though that during rendering attributes of the actual
  visual representation may be bigger than the actual covered area. This
  especially holds for labels.
* So if you make use of such "visualisation is bigger than the actual covered
  area" effects and want to make sure that the rendering is stable if the 
  bounding box changes, you liekely will have to fetch data for a bigger
  bounding box than the by the rendeirng covered area. The size of this
  bounding box depends on the "size" of the used effects and not on the bounding
  box of the covered area. It may even not be valid to assume that these boxes
  are linked by some constant factor ("covered bounding box area * 1.5").  
* OpenStreetMap itself uses a 3x3 tile grid for this. Which means that the
  bounding box of the loaded data is 3 times as big as the covered area that is
  later on rendered.
* Also if you make use of such effects, make sure that clipping is not activated
  later on too soon in the rendeirng pipeline as this may result in negativing
  the effect expected by loading a bigger area than axctually getting rendered
  later on.

## The `MapService`

To be written...

## The `MapPainter`

To be written...
