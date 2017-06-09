---
date: "2016-08-28T15:25:58+02:00"
title:  "Performance optimizations"
description: "How to optimize import and rendering"
weight: 5

menu:
  main:
    Parent: "tutorials"
    Weight: 5
---

## General notes

Note that Libosmscout uses an importer that creates a database from the original
OSM data. This importer should be run on desktop or server hardware. It is neither
designed to run nor is it necessary to run it on the mobile. The hardware
requirements for the importer and the on-mobile libraries (map rendering,
routing and address/POI lookup) should thus be viewed separately.

## My development system

My current system has a octa-core Core7 CPU and 16GB main memory. From time to
time I will make sure that even big imports like germany or France will work.
I try to keep the default configuration, so that it will work on less powerful
systems, by may fail to do so.

## Import

### Minimum system requirements

You should at least have 4GB of main memory to work with smaller imports.
It may be possible that a smaller import works with less memory,
but this is neither tested nor garanteed. Import will also very likely be very
smal in this case.

It may though that the import will not work or will be very slow with the default
configuration on systems with only little memory. In this case to not panic,
just try to reduce resource consumption by reducing default values.
        
### Import optimization guidelines

* Most import steps will just use one CPU. So a multi processor machine -
  while useful in general - will not increase import performance as much
  as you might expect. Future versions of the import may make better use
  of multi core machines - at least for some import steps.
* Each import step has its own memory constraints depending on import data size
  and structure. In general import has to be configured to work best on *your*
  system with *your* data. Currently the importer is by default tuned to smoothly
  import larger pbf files on systems with medium memory (4GB main memory).
  This should make most of your imports run smooth, too, but there is no
  garantee.
* See the command line options of `Import` for a list of options.
* Take a look at each import module source code to see which options are used
  where.
* Do not change options at random.
* The import is fastest if each step runs with a minimum of iterations and by
  using all physical memory available (of course if data set is very small not
  all physical memory might be needed).
* If the systems must use swap it will be much slower than using mutiple
  iterations for getting the work done.
* If the import runs into swap, try to reduce memory consumption by reducing
  block sizes and thus increasing iterations. This will make the importer run
  slower than on systems with more memory but still makes it way faster than if
  running into swap.
* On big imports on a 32 bit system, using memory mapped file access for all
  intermediate data files might might result in out of memory because memory
  mapping big files and doing many allocation pushes the process over the 4GB
  addressable memory limit. In this case switch of memory mapping for data and
  index files in this case to stay below 4GB.
  
## Rendering  

The rendering process consists basically of two parts:

* Loading of the data to be rendered. The Loading of data is mainly IO bound
  but it also consists of decoding of compressed data into memory, it is thus
  also CPU bound.
* Rendering of the data. The rendering itself again consists of a number of
  steps, which either prepare data for rendering or do the actual drawing. The
  rendering is CPU (sorting of data, label layouting, ...), FPU bound
  (calculation of coordinates on screen using some projection) and finally GPU
  bound (actual rendering).

For performance analysis and improvements it is mandatory which part is the
problematic one and which actual resource boundaries it has.

### Minimum system requirements

Libosmscout runs on modern iPhone mobiles, Ubuntu and Sailfish phones and
also on the RaspberryPi. 1GB of main memory should be enough, possibly even less.
More memory will allow more caching of data, making the data loading -
and thus rendering - faster.

You should have at least 500MB of "disk"-space, for bigger maps more. Country
imports may result in 1-2GB disk space needed.

### Use of memory mapped files

Libosmscout uses memory mapped files using `mmap` under Unix
and under Windows. Note that depending on the OS for `mmap`
to work main memory of the sum of the size of the individual open files
may be required. If memory mapped files cannot be activated
the library falls back to normal file access automatically.

Using memory mapped files will increase data loading performance.

### DatabaseParameter-Options

AreaAreaIndexCacheSize, AreaNodeIndexCacheSize
: Caches for the some of the indexes. If you do not give this caches enough
  memory, pages of indexes must be repeately loaded from disk. If you have
  enough memory, give this caches enough memory so that they can completely
  (or at least most of it) load the index into memory. The indexes will log
  a warning, if assigned cache memory is not enough.

### AreaSearchParameter-Options

MaxAreaLevel
: The area index is implemented as quad-tree. The Index is build upon
  containment in tiles where tile count for each level is dubbled
  in every dimension (1, 4, 16...tiles). An area is indexed by a certain
  tile if it is smaller than the tile area, is bigger than the tile
  area in the next index level and the area is (partially) covered by the
  tile. That means that the index sorts areas by size and position.
  The Database evaluates all tiles covering the area from the top level
  upto the current zoom level plus further `MaxAreaLevel` levels.
  The bigger `MaxAreaLevel` is the more details you will thus see. On
  the other side the more index pages have to be loaded and evaluated,
  the longer the lookup takes. Note also that the index by default is
  generated up to (zoom) level 18. The final zoom level contains
  all data, not indexed higher in the index. So if on zoom level 18
  and using `MaxAreaLevel=0` everything will still be shown.
  
### MapParameter-Options

OptimizeWayNodes, OptimizeAreaNodes
: If set, the drawing engine tries to reduce the number of nodes for
  ways and/or areas. These additional step costs CPU time, but reduces
  the number of points and lines elements the rendering backend needs to draw.
  Use these options, if you have CPU left and drawing is slow.
  This traits CPU for GPU. Play with it.

LineMinWidthPixel
: Minimum width of a line in pixel. If a line is too small it will
 possibly get lost in anti aliasing :-)

AreaMinDimensionMM
: Areas smaller than this will dropped from rendering since they would
  not be (really) visible anyway.

OptimizeErrorToleranceMm
: The higher the value, the more aggresive the above optimization

LabelSpace, PlateLabelSpace, SameLabelSpace
: Space around labels. If big, a lot of labels might get loaded but
  not rendered. If small too many labels might get rendered.

DropNotVisiblePointLabels
: Labels not visible will not be rendered. However if you render
  tiles, labels not being visible might still influence the label
  placing algorithm. Switch on if rendering tiles, else switch off.
  
There are two modes for reducing nodes in way and areas:

fast
: Scan the line for nodes that are either very close or
  are positioned close enough on a line between its preceding and
  following node.

quality
: Uses Douglas Peuker algorithm to simplify the contour.


### Analysing data loading and rendering performance

The MapService logs warnings, in case that data loading operations take too
long.

If you see output like

```
Retrieving all tile data took 0.2 seconds
```

this is a sign, that data loading tool too long for parts or all of the data.
The thresholds for the warnings are currently hardcoded.

#### DebugPerformance switch

If `DebugPerformance` for the `MapPainter` is activated, some measurements for
parts of the rendering are logged. It is recommended to look at the actual code
in `MapPainter.cpp` to understand the logged values.

#### Example debugging output

An example could look like this (Overview of Dortmund at zoom level 11):

```
Draw: [51.35236 N 7.36066 E - 51.58064 N 7.58053 E] 2048x/11 480x800
143.90712 DPI
Paths: 7555/2733/2733/794 (pcs) 0.010/0.023/0.006 (sec)
Areas: 4032/1351/2702 (pcs) 0.026/0.018/0.000 (sec)
Nodes: 663+0/663 (pcs) 0.007/0.000 (sec)
Labels: 85/7/92 (pcs) 0.075 (sec)
```

or like this (same region, but zoom level 16):

```
Draw: [51.51090 N 7.46212 E - 51.51803 N 7.46899 E] 65536x/16 480x800
143.90712 DPI
Paths: 7936/781/781/141 (pcs) 0.014/0.030/0.021 (sec)
Areas: 2130/584/1168 (pcs) 0.007/0.018/0.018 (sec)
Nodes: 2162+0/2162 (pcs) 0.020/0.000 (sec)
Labels: 260/1/261 (pcs) 0.132 (sec)
```

with the logging code at that time:

```c++
if (parameter.IsDebugPerformance()) {
  log.Info()
     << "Paths: "
     << data.ways.size() << "/" << waysSegments << "/" <<  waysDrawn << "/" << waysLabelDrawn << " (pcs) "
     << prepareWaysTimer << "/" << pathsTimer << "/" <<  pathLabelsTimer << " (sec)";

  log.Info()
     << "Areas: "
     << data.areas.size() << "/" << areasSegments << "/" <<  areasDrawn << " (pcs) "
     << prepareAreasTimer << "/" << areasTimer << "/" <<  areaLabelsTimer << " (sec)";

  log.Info()
     << "Nodes: "
     << data.nodes.size() <<"+" << data.poiNodes.size() << "/"  << nodesDrawn << " (pcs) "
     << nodesTimer << "/" << poisTimer << " (sec)";

  log.Info()
     << "Labels: " << labels.size() << "/" <<  overlayLabels.size() << "/" << labelsDrawn << " (pcs) "
     << labelsTimer << " (sec)";
}
```

#### DebugData switch

You can also switch on data debugging.

Example output (Dortmund, zoom level 11):

```
Type|NodeCount|WayCount|AreaCount|Nodes|Labels|Icons
highway_residential 2113 0 2103 10 16995 0 0
railway_rail 830 0 830 0 7433 0 0
highway_secondary 630 0 630 0 6990 630 0
highway_tertiary 321 0 321 0 4369 0 0
highway_unclassified 311 0 311 0 2693 0 0
natural_scrub 242 0 0 242 4248 1 0
highway_motorway 209 0 209 0 1564 418 0
highway_trunk_link 206 0 206 0 2028 206 0
highway_primary 184 0 184 0 2077 368 0
highway_motorway_link 159 0 159 0 1598 159 0
highway_motorway_trunk 149 0 149 0 1100 0 0
waterway_stream 127 0 127 0 1743 0 0
leisure_pitch 110 0 0 110 562 0 0
leisure_common 104 0 0 104 1196 0 0
landuse_residential 99 0 0 99 7633 9 0
leisure_park 97 0 0 97 2766 0 0
wood 82 0 0 82 4685 7 0
landuse_allotments 72 0 0 72 1352 0 0
place_suburb 54 54 0 0 54 54 0
waterway_drain 53 0 53 0 374 0 0
highway_trunk 52 0 52 0 366 104 0
landuse_commercial 48 0 0 48 864 0 0
highway_motorway_junction 46 46 0 0 46 46 0
natural_water 44 0 0 44 1366 1 0
highway_primary_link 40 0 40 0 327 40 0
highway_secondary_link 39 0 39 0 296 39 0
landuse_industrial 37 0 0 37 1109 0 0
landuse_railway 29 0 0 29 1273 0 0
landuse_farmland 24 0 0 24 1206 0 0
landuse_brownfield 23 0 0 23 347 0 0
amenity_school 22 0 0 22 429 0 0
place_town 22 22 0 0 22 0 0
highway_road 21 0 21 0 97 0 0
landuse_basin 20 0 0 20 213 0 0
waterway_river 18 0 18 0 886 0 0
highway_tertiary_link 14 0 14 0 76 0 0
waterway_canal 13 0 13 0 130 0 0
landuse_cemetery 13 0 0 13 291 0 0
waterway_riverbank 11 0 0 11 13596 0 0
natural_peak 10 10 0 0 10 20 10
waterway_dock 8 0 0 8 187 0 0
leisure_nature_reserve 8 0 0 8 4797 1 0
landuse_greenfield 7 0 0 7 132 0 0
power_generator 7 0 0 7 36 0 0
landuse_retail 5 0 0 5 106 1 0
natural_grassland 3 0 0 3 133 0 0
landuse_farm 3 0 0 3 71 0 0
landuse_recreation_ground 3 0 0 3 31 0 0
amenity_hospital 3 0 0 3 69 0 0
place_hamlet 3 3 0 0 3 3 0
landuse_construction 2 0 0 2 16 0 0
landuse_reservoir 2 0 0 2 9 0 0
natural_wetland 2 0 0 2 36 0 0
leisure_track 2 0 0 2 74 0 0
place_islet 2 1 0 1 24 2 0
aeroway_helipad 1 0 0 1 4 0 0
landuse_military 1 0 0 1 7 0 0
landuse_quarry 1 0 0 1 12 0 0
leisure_water_park 1 0 0 1 14 0 0
amenity_grave_yard 1 0 0 1 25 0 0
tourism_attraction 1 0 0 1 6 0 0
tourism_artwork 1 0 0 1 4 0 0
historic_ruins 1 0 0 1 4 0 0
historic_archaeological_site 1 0 0 1 24 0 0
place_village 1 1 0 0 1 1 0
elevation_contour_major 0 0 0 0 0 0 0
elevation_contour_medium 0 0 0 0 0 0 0
_route 0 0 0 0 0 0 0
_tile_land 0 0 0 0 0 0 0
_tile_sea 0 0 0 0 0 0 0
_tile_coast 0 0 0 0 0 0 0
_tile_unknown 0 0 0 0 0 0 0
elevation_contour_minor 0 0 0 0 0 0 0
highway_motorway_primary 0 0 0 0 0 0 0
historic_battlefield 0 0 0 0 0 0 0
waterway_weir 0 0 0 0 0 0 0
natural_fell 0 0 0 0 0 0 0
natural_glacier 0 0 0 0 0 0 0
route_ferry 0 0 0 0 0 0 0
aeroway_aerodrome 0 0 0 0 0 0 0
aeroway_terminal 0 0 0 0 0 0 0
aeroway_runway 0 0 0 0 0 0 0
aeroway_taxiway 0 0 0 0 0 0 0
aeroway_apron 0 0 0 0 0 0 0
natural_land 0 0 0 0 0 0 0
landuse_farmyard 0 0 0 0 0 0 0
landuse_landfill 0 0 0 0 0 0 0
landuse_vineyard 0 0 0 0 0 0 0
natural_beach 0 0 0 0 0 0 0
historic_wreck 0 0 0 0 0 0 0
military_airfield 0 0 0 0 0 0 0
natural_heath 0 0 0 0 0 0 0
tourism_alpine_hut 0 0 0 0 0 0 0
natural_wetland_marsh 0 0 0 0 0 0 0
natural_wetland_tidalflat 0 0 0 0 0 0 0
leisure_golf_course 0 0 0 0 0 0 0
military_danger_area 0 0 0 0 0 0 0
military_range 0 0 0 0 0 0 0
military_naval_base 0 0 0 0 0 0 0
leisure_marina 0 0 0 0 0 0 0
leisure_fishing 0 0 0 0 0 0 0
leisure_ice_rink 0 0 0 0 0 0 0
amenity_bank 0 0 0 0 0 0 0
amenity_cafe 0 0 0 0 0 0 0
amenity_fast_food 0 0 0 0 0 0 0
amenity_fuel 0 0 0 0 0 0 0
amenity_kindergarten 0 0 0 0 0 0 0
amenity_post_office 0 0 0 0 0 0 0
amenity_restaurant 0 0 0 0 0 0 0
amenity_taxi 0 0 0 0 0 0 0
amenity 0 0 0 0 0 0 0
tourism_camp_site 0 0 0 0 0 0 0
tourism_caravan_site 0 0 0 0 0 0 0
tourism_picnic_site 0 0 0 0 0 0 0
tourism_theme_park 0 0 0 0 0 0 0
tourism_zoo 0 0 0 0 0 0 0
tourism_chalet 0 0 0 0 0 0 0
tourism_guest_house 0 0 0 0 0 0 0
tourism_hostel 0 0 0 0 0 0 0
tourism_hotel 0 0 0 0 0 0 0
tourism_information 0 0 0 0 0 0 0
tourism_motel 0 0 0 0 0 0 0
tourism_museum 0 0 0 0 0 0 0
historic_castle 0 0 0 0 0 0 0
historic_monument 0 0 0 0 0 0 0
historic_memorial 0 0 0 0 0 0 0
place_island 0 0 0 0 0 0 0
```

`DebuggingData` gives you some information about what data (number and type of
objects) is actually loaded and passed to the render.

#### Analysis of debugging output

As one can see in above example logs, at higher zoom level libosmscout is loading
nearly the same amount of ways from database, but is rendering by some amount
less of them and less way labels (interesting enough we are a little bit 
slower though).

Similar for areas: Libosmscout is only loading half of the areas and again
rendering only half of them.

And the same for nodes. As one can see, the number of labels in the
second case has drastically increased and rendering them took most of the 
time.

As one can see, labels are rather expensive to render, however the
label detection mechanism seems to be rather cheap.

What to learn:

* Take a look at the data and see if you can reduce the number of
  objects getting rendered. If there 2000-4000 areas, do you really see
  them? If you do not see them, why load them? A high drop between way
  and areas loaded and rendered either means that the objects are
  outside the visible range and clipping is working on them or they are
  possibly just to small to be seen (we are dropping to small areas).
* A good measurement for the map beeing too crowdy is the number of areas per
  display pixel or square millimeter.
* If you have too many labels, reduce them. How many labels should be
  on map, how many can you actually read on a mobile. The labeling
  algorithm tries to drop overlapping labels. So it is possibly that you
  are loading a loot of labels but most of them get dropped and not
  rendered anyway. The first rule of optimization: Just don't do things
  you do not need and then make the (most expensive) rest faster ;-)
* Comparing the rendering times for different zoom levels you can see
  where are the hot spots in your style sheet. In the default style
  sheet I think the problem zone is around level 11-13. These is the
  range where the most objects are rendered. Later on (further zooming in)
  the number of objects decreases again.
* If you switch on `DebugData` in the `MapParameter` you get a more detailed
  list of objects (by type) actually loaded. Move things in the style sheet
  to a higher zoom level if data is loaded but is or should not immediately
  visible on the map. Do not load a football stadium at zoom level where
  it will be barely or not at all visible.
* Note that depending on the region objects might have a very different size.
  This is especially true for the nature itself. In some countries
  beaches are just bigger than in others. Similar for national parks and other
  similar stuff. 
* Note that the OSMScout2 demo uses partial rendering. That means, a map
  is rendered after around 100ms even if not all data is loaded. Try to
  change your code so that you do not need to wait for all data loaded,
  if loading data takes too much time.
* If label rendering is expensive, see how many labels are loaded and how
  many of the are actually rendered. Font size measurements and text
  rendering are in fact the most costly things currently. Each letter
  normally consists of small belzier curved lines and areas. 100 letters
  are likely comparable to render 100 areas or ways.
* If rendering is still too expensive do not render that often. Try to
  cache results of rendering (this is the render to tiles approach, most
  of the map is just copied from some back buffer and not rendered at
  all).
* On zoom and panning do not wait for a complete rerender. Show what you 
  have. Especially on zoom try to use image magnification functions as
  part of GUI Toolkit or OS instead off rerendering everything
  (OSMScout2 does *not* do this yet).
* Try to make the rendering stack render things in background. Make sure
  that response time and some sensible framerate on changes is
  prioritized before "complete image rendered" (like the delayed
  rendering does).

## Profiling

And yet, debug output only gives you rough information. Using a 
profiler give you more detailed information. But the context given by
the debug information might still help interpret the result and traget the
profiler in the right direction.

Make sure that you do profiling on the actual target platform. Performance
relevant behaviour may be very different depending on the actual target system.

Key performance idicator regarding CPU, FPU, GPU disk and memory IO may be
different and thus different parts of the process may feel slughish depending
on the actual device.

