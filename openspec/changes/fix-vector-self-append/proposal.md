# Proposal

## Why

The optimizer that turns a projected way or area into a simple geometry — the step that keeps a drawn
polygon from intersecting itself — closes the sequence it is optimizing by appending that sequence's
own first element back onto it. The element is read through a reference into the sequence that is
growing, so what the append stores is only defined while the append does not move the storage. GCC 16
reports the site as a possible use of an uninitialized value in a Release build
(`libosmscout/src/osmscout/util/Transformation.cpp:613`, reproduced with the build's own compile
command), and the same shape appears at a dozen further places in the core library, the importer and
the tests. Where the assumption does not hold, the append reads an element that the growing sequence
has already moved from or released: the geometry loses its closing point or carries an undefined
point. That geometry reaches the importer's low-zoom areas and its water index, and it is the geometry
the map painters draw.

## What Changes

- Every append in the tree that appends an element of the sequence it appends to takes a value that is
  independent of that sequence's storage, so the append is defined for any storage state and any
  implementation.
- The optimized geometry is unchanged for every input: the same points, the same draw flags, the same
  closing point, for ways and for areas.
- A Release build reports no possible-uninitialized-use warning for the optimizer.
- The optimizer's output is covered for the storage state the current code assumes away: a geometry
  whose closing append has to grow the storage produces the same result as one that does not.
- The pattern does not come back unnoticed: the places that append an element of their own sequence
  are checked from the tree rather than by reading them, so a new one is reported.

## Capabilities

### New Capabilities

- `geometry-optimization-integrity`: the contract of the projected-geometry optimizer — that the
  geometry it produces for a way or an area is simple and closed, and that this output does not depend
  on the storage state of the sequence it optimizes.

### Modified Capabilities

None. No existing capability describes the optimizer's output contract; the rendered and imported
output is intended to stay as it is.

## Impact

Affected code:

- `libosmscout/src/osmscout/util/Transformation.cpp` — the closing appends of the optimized
  sequence: the area closing point and the closing point of the cut-off branch, both reached from
  `OptimizeArea` and `OptimizeWay`.
- `libosmscout/include/osmscout/util/Geometry.h` — the closing appends of the ring helpers
  (`AreaIsSimple`, `AreaIsValid`).
- `libosmscout-import/src/osmscoutimport/WaterIndexProcessor.cpp` — the coastline closing append,
  which is guarded by an equality test that compares the points but does not protect the read.
- `BasemapImport/src/BasemapImport.cpp` — the closing append of a coastline that crosses the
  antimeridian.
- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` — the closing append of the
  ring points an area is prepared with.

Tests and build:

- `Tests/src/TransPolygonTest.cpp` — the existing simplicity cases gain the storage-growth case; a new
  case is added only if the existing target cannot carry it.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — only if a new target is introduced for the storage
  check; the existing test registry stays the primary home.
- `scripts/` — the tree check for appends of an element of the same sequence, registered as a test
  next to the existing signature check (both build systems).

Checked and not affected, because each appends an element of a different sequence than the one it
appends to (the receiver and the argument are different objects):
`libosmscout-import/src/osmscoutimport/GenAreaAreaIndex.cpp:298,372` and
`SortWayDat.cpp:219,280` (node buffer from ring/way nodes),
`WaterIndexProcessor.cpp:615` (tile coordinates from cell boundary coordinates),
`GenRelAreaDat.cpp:266` (ring ways from a matched other part) and `DbJson.cpp:412` (file entries).
The optimizer's contract, the projection code, the type definition format, the database format and
the public API stay unchanged; no rendered or imported output is intended to change.
