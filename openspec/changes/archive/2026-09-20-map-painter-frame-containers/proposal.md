## Why

`MapPainter` prepares every visible area, way, way path and route label into per-frame
storage before drawing. That storage is torn down and rebuilt on every rendered frame,
and it grows 1:1 with the object count of the viewport, which the recent type-set
expansion (671 -> 1527 types, PR #1782) directly raised. The result is per-frame
allocation churn and a pointer-chasing draw traversal in the hottest client path
(tile rendering on every pan step), with no requirement that the prepared frame data
stays contiguous or that its capacity survives from one frame to the next.

## What Changes

- Prepared frame data for areas, ways, way paths and route labels SHALL be reused
  across frames instead of released and re-allocated per frame.
- The prepared frame data SHALL be stored contiguously so the draw steps traverse it
  without per-object pointer chasing.
- Route labels SHALL keep a valid reference to their prepared way path when the
  prepared data is reused.
- The draw order of prepared areas and ways SHALL be unchanged, including for areas
  whose ordering criteria compare equal.
- **BREAKING** (internal API): the accessors that expose prepared areas and ways to
  backend callbacks SHALL expose the reused contiguous storage instead of the current
  per-frame node-based storage.
- No rendering output change: identical geometry, styling, label placement and
  draw order.

## Capabilities

### New Capabilities

- `map-painter-frame-buffers`: contract for the prepared per-frame data of the map
  painter: reuse across frames, contiguity, stable ordering, path-reference validity
  for route labels, and read access for backend callbacks.

### Modified Capabilities

<!-- None: no existing capability's requirements change. Regression concerns for
     area-clippings, label-rendering and shield-label-rendering are unchanged
     behavior and are covered by verification tasks. -->

## Impact

Affected modules:

- `libosmscout-map` (core rendering, `MapPainter`):
  - `include/osmscoutmap/MapPainter.h` - prepared data members, the way path
    reference type used by route labels, the public prepared-area/prepared-way
    accessors, the area clipping storage nested in prepared areas.
  - `src/osmscoutmap/MapPainter.cpp` - the fill, clear, sort and consume sites of
    the prepared data: `CalculatePaths`, `CalculateWayPaths`, `ProcessAreas`,
    `PrepareAreaRing`, `ProcessRoutes`, `AfterPreprocessing`, `DrawOSMTileGrid`,
    `DrawGroundTiles`, `DrawAreas`, `DrawWays`, `DrawWayDecorations`,
    `DrawWayContourLabels`, `PrepareAreaLabels`, `DrawAreaBorderLabels`,
    `DrawAreaBorderSymbols`, `PrepareRouteLabels`.
- `libosmscout-map-svg`: `src/osmscoutmapsvg/MapPainterSVG.cpp`
  (`AfterPreprocessingCallback`) is the only backend that reads the prepared areas
  and ways directly; it must follow the accessor change.
- `Tests`: `Tests/src/PerformanceTest.cpp` (before/after measurement),
  `Tests/src/MapPainterRouteTest.cpp`, `Tests/src/MapPainterShieldTest.cpp`,
  `Tests/src/MapPainterShieldQtTest.cpp` (draw order and label placement
  regressions), plus a new unit test for the reuse, ordering and path-reference
  requirements.
- `TODO.md`: the `MapPainter` prepared-data finding is resolved by this change.

Public API impact: the prepared-area and prepared-way accessors of `MapPainter` are the
protected hooks a backend (subclass) uses from its post-preprocessing callback, and they
change their return type. In-repo consumers are the SVG backend only; external backends
that subclass `MapPainter` and use these accessors must be updated.

No database, file format, style or import impact.
