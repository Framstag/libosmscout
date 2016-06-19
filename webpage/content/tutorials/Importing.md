---
date: "2016-05-29T19:40:58+02:00"
title:  "Importing an *.osm or *.pbf file"
description: "How to import a *.osm or *.pbf file to create a database"
weight: 1

menu:
  main:
    Parent: "tutorials"
    Weight: 1
---
        
## Setup

All examples and tutorials expect the OSM export files and resulting
databases to be placed in the *maps/* sub directory.

Say, you are using the
[*nordrhein-westfalen.osm.pbf*](http://download.geofabrik.de/europe/germany/nordrhein-westfalen-latest.osm.pbf)
file from [Geofabrik](http://www.geofabrik.de/).

Download the file and rename it to nordrhein-westfalen.osm.pbf and place it
in the maps sub directory:

```bash
  $ ls maps/
  nordrhein-westfalen.osm.pbf
```

In the following text `nordrhein-westfalen` is the paceholder for the 
base name (without the suffix) of your import data file.

The examples and the `build.sh` in general assume that the Importer tool can be
called as `../Import/src/Import` (from the maps sub directory). Depending on the
actual build process the resulting executable file name and its path may differ.

## Using the bash script

Under Unix-like systems you can use the `build.sh` bash script
in the *maps/* sub directory. Go into this directory and call the
script from there as follows:

```bash
  $ ./build.sh nordrhein-westfalen.osm.pbf
```
The advantanges of the script are:

* it sources `nordrhein-westfalen.opt` if available and uses the variable
`options` as defined in this file to call the importer. This way you do not
need to pass a long list of command line arguments every time.
* it also sources `default.opt`for the same reason, if `nordrhein-westfalen.opt`
was not found.
* It passes the output of the importer to the output but also to the file
`nordrhein-westfalen.txt` in the same directory. So you can later take a look
at this file for information you missed during the import.

## Calling the importer by hand

You can also call the importer by hand. The minimum command line would be

```bash
  $ .../Import/src/Import  --typefile ../stylesheets/map.ost --destinationDirectory nordrhein-westfalen nordrhein-westfalen.osm.pbf
```

## Resulting database

The importer uses a number of import steps (currently ver 20) to generate a
custom database consisting of several data and index files.

Each step print itself with a header showing its running number, its name,
a short description of the task of the step, the dependend files and the
files generated. `-` announces a task, `%` prints the progress in relation
to the current task, `WW` introduces warnings and `!!` introduces errors.

```
...
+ Step #3 - CoordDataGenerator...
   Module description: Generate coord data file
   Module requires file 'rawcoords.dat'
   Module provides debugging file 'coord.dat'
 - Searching for duplicate coordinates...
   Searching for coordinates with page id >= 0
   % 27.42 (15078926/54994116)
   % 69.74 (38352393/54994116)
   Sorting coordinates
   Detect duplicates
   Loaded 54994116 coords (54994116/54994116)
   Found 358622 duplicate cordinates
 - Storing coordinates...
   Search for coordinates with page id >= -1844674407370
   % 91.30 (50207143/54994116)
   Sorting coordinates
   Write coordinates
   Loaded 54994116 coords (54994116/54994116)
 - Writing 2855797 index entries to disk...
   => 66.044s, RSS 2.8 GiB, VM 4.8 GiB
...
```


At the end of the import process the file size of the (major) resulting files
is dumped:

```
...
     Overall 833.727s, RSS 3.6 GiB, VM 7.8 GiB
+ Summary...
   Mandatory files:
   File areaarea.idx: 5.9 MiB
   File areanode.idx: 3.4 MiB
   File areas.dat: 189.5 MiB
   File areaway.idx: 2.7 MiB
   File bounding.dat: 14 B
   File intersections.dat: 27.8 MiB
   File intersections.idx: 8.6 MiB
   File location.idx: 17.9 MiB
   File nodes.dat: 11.7 MiB
   File router.dat: 92.5 MiB
   File router.idx: 8.6 MiB
   File router2.dat: 1.2 KiB
   File types.dat: 24.9 KiB
   File water.idx: 496.2 KiB
   File ways.dat: 62.7 MiB
   => 431.8 MiB
   Optional files:
   File areasopt.dat: 1.9 MiB
   File textloc.dat: 2.4 MiB
   File textother.dat: 2.5 MiB
   File textpoi.dat: 1.3 MiB
   File textregion.dat: 107.6 KiB
   File waysopt.dat: 2.9 MiB
   => 11.1 MiB
   Import OK!
```

## Optimizing the import process

TODO

## Optimizing the resulting database

TODO
