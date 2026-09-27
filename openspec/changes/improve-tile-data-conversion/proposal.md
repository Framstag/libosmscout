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
  cost follows the objects the tiles hold and does not grow with the tiles that repeat them.
- The order of the objects in the resulting `MapData` becomes defined and reproducible for a given tile
  list, instead of depending on the iteration order of an internal container.
- The conversion offers one contract for all of its callers. Whether the entry point that no caller in
  the repository uses stays is a separate decision (design.md, D3).
- Every phase of the conversion reports itself when it exceeds a threshold, the threshold is settable,
  and the value the previous implementation warned at stays the default.
- A test fails when the conversion becomes slower than the conversion it replaces, over tile sets that
  differ in how often their tiles repeat the same object, and a second test pins the cost that timing
  cannot pin.

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

- `libosmscout-map/src/osmscoutmap/MapService.cpp` - both `AddTileDataToMapData` overloads, the
  seen-offset table of a source data file, and the phase measurement.
- `libosmscout-map/include/osmscoutmap/MapService.h` - the public conversion API: the documented
  contract of both entry points and the settable threshold of the phase report (additive).
- `Tests/src/TileDataConversionTest.cpp` (new) - uniqueness, order, the two offset spaces, duplicates,
  the restricted entry point, the phase report and the allocation bound.
- `Tests/src/TileDataConversionPerformanceTest.cpp` (new) - the comparison against the conversion this
  change replaces.
- `Tests/src/TestAllocationCounter.cpp` (new) and `Tests/include/TestAllocationCounter.h` (new) - the
  counting allocator the allocation assertions use, shared by the tests, disabled when the binary is
  built with AddressSanitizer.
- `Tests/CMakeLists.txt`, `Tests/meson.build` - both new test targets, in both build systems.
- `Tests/src/PerformanceTest.cpp` - the existing end-to-end driver used for the before/after number;
  unchanged in the change itself.
- Call sites, expected unchanged in behaviour: `libosmscout-client-qt/src/osmscoutclientqt/MapRenderer.cpp`,
  `.../DBLoadJob.cpp`, `.../IconLookup.cpp`, `libosmscout-client-java/src/OSMScoutClient.cpp`,
  `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp`, `Demos/include/DrawMap.h`, `Demos/src/ResourceConsumption*.cpp`,
  `Demos/src/RoutingAnimation.cpp`, `Apple/OSMScoutOSX/OSMScoutOSX/OSMScout.mm`,
  `libosmscout-extern/src/matlab/libosmscoutmx.cpp`, `Tests/src/LaneEvaluationCompare.cpp`.
- Not affected: the map database format, the import pipeline, the stylesheets and `.ost` files.
