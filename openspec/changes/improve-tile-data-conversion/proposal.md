# Proposal

## Why

Every pan and zoom step converts the objects of the loaded tiles into the `MapData` of one render
job. That conversion builds a deduplication structure per object kind, inserts every object of every
tile - including the many objects that neighbouring tiles share and the objects a prefill carries
over from parent tiles - and then copies the result into new vectors. Its cost therefore follows the
objects of all loaded tiles, not the distinct objects of the view, it is paid once per database per
rebuild, and nothing reports the step when it gets slow. A viewport that loads more objects, or a
database stack with more databases, pays it more often without anything noticing.

## What Changes

- The conversion of a set of loaded tiles into the `MapData` of one render job becomes a step whose
  cost follows the distinct objects the view needs, not the duplicate insertions of the tiles that
  carry them.
- The order of the objects in the resulting `MapData` becomes explicit, documented and reproducible,
  instead of depending on the iteration order of an internal container.
- The conversion offers one contract for all of its callers, so that the API surface matches what is
  actually used (a removal of an entry point no caller uses is a **BREAKING** API change; see
  design.md for the decision and its alternatives).
- Every phase of the conversion reports itself when it exceeds a threshold, so a regression on a user
  device is visible in the log.
- A test fails when the conversion becomes slower than the recorded baseline, and a second test pins
  the structural cost of the conversion with an exact assertion that does not depend on timing.

## Capabilities

### New Capabilities

- `tile-data-conversion`: the contract for turning the object data of a set of loaded tiles into the
  `MapData` of one render job - that each object appears once, that the resulting order is defined
  and stable, that the cost of the step follows the distinct objects rather than the duplicate
  insertions, that its phases are observable, and that a slower conversion fails a test.

### Modified Capabilities

None. The painter-side capabilities (`map-painter-frame-buffers`, `map-painter-way-culling`,
`map-painter-area-preparation`) describe the step that consumes `MapData`; this change produces it and
their requirements are unchanged.

## Impact

- `libosmscout-map/src/osmscoutmap/MapService.cpp` - both `AddTileDataToMapData` overloads.
- `libosmscout-map/include/osmscoutmap/MapService.h` - the public conversion API.
- `libosmscout-map/include/osmscoutmap/DataTileCache.h` - `TileData` has to let the conversion consume
  its stored objects in the order they were loaded.
- `Tests/src/TileDataConversionTest.cpp` (new) - uniqueness, order and structural cost.
- `Tests/src/TileDataConversionPerformanceTest.cpp` (new) - the speed assertion against a baseline.
- `Tests/CMakeLists.txt`, `Tests/meson.build` - both new test targets, in both build systems.
- `Tests/src/PerformanceTest.cpp` - the existing end-to-end driver used for the before/after number.
- Call sites, expected unchanged in behaviour: `libosmscout-client-qt/src/osmscoutclientqt/MapRenderer.cpp`,
  `.../DBLoadJob.cpp`, `.../IconLookup.cpp`, `libosmscout-client-java/src/OSMScoutClient.cpp`,
  `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp`, `Demos/include/DrawMap.h`, `Demos/src/ResourceConsumption*.cpp`,
  `Demos/src/RoutingAnimation.cpp`, `Apple/OSMScoutOSX/OSMScoutOSX/OSMScout.mm`,
  `libosmscout-extern/src/matlab/libosmscoutmx.cpp`, `Tests/src/LaneEvaluationCompare.cpp`.
- Not affected: the map database format, the import pipeline, the stylesheets and `.ost` files.
