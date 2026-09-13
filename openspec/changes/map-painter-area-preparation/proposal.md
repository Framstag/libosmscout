## Why

Area preparation is the largest single step of a rendered frame at city zoom levels. Measured
with the Cairo backend on the Dortmund database at zoom 15 (304 tiles, 17629 areas loaded per
tile), the painter's area preparation step consumes about 27% of frame CPU (4.43 ms per tile)
and about 93% of the frame's heap allocations, and the same step costs the same under the Qt
and no-op backends (4.22 / 4.48 ms), so it is backend-independent work that every map
application pays. The step does work for every area loaded into the view, while only about
4% of those areas (725 of 17629) end up prepared for drawing, and the recent type-set
expansion (671 -> 1527 types) increased the number of loaded area objects per view. Reducing
this step is the first step towards keeping pan and zoom frames responsive in the z14-z17
band, where maps are most often viewed and navigated.

## What Changes

- Area preparation SHALL avoid heap work that grows with the number of loaded areas rather
  than with the number of areas that are actually prepared for drawing.
- The decision whether an area ring takes part in the frame (its styling and its visibility)
  SHALL be made before the ring's geometry is transformed for that frame.
- Rings that do not take part in the frame SHALL NOT contribute transformed geometry to the
  frame's coordinate data.
- Rings that are required as clipping regions of a drawn area SHALL keep receiving valid
  geometry even when they are not drawn themselves.
- The set of prepared areas, the draw order of prepared areas, and the rendered output SHALL
  be unchanged, including for areas whose ordering criteria compare equal.
- No rendering output change: identical geometry, styling, label placement and draw order.

## Capabilities

### New Capabilities

- `map-painter-area-preparation`: contract for the painter's area preparation step - heap work
  relative to loaded versus prepared areas, the order of the styling/visibility decision
  against geometry transformation, geometry for clipping rings, and an unchanged prepared
  area set and draw order.

### Modified Capabilities

<!-- None: the prepared-area storage contract is defined by map-painter-frame-buffers, which
     this change keeps unchanged. -->

## Impact

Affected modules and files:

- `libosmscout-map`:
  - `include/osmscoutmap/MapPainter.h` - state used while preparing areas for a frame.
  - `src/osmscoutmap/MapPainter.cpp` - `ProcessAreas`, `PrepareArea` and `PrepareAreaRing`,
    the sites that decide styling and visibility and that transform ring geometry.
- Backends (`libosmscout-map-cairo`, `libosmscout-map-qt`, `libosmscout-map-skia`,
  `libosmscout-map-svg`, `libosmscout-map-agg`, `libosmscout-map-gdi`,
  `libosmscout-map-directx`, `libosmscout-map-iosx`, Android JNI painter): no source change
  expected - area preparation is backend-independent (measured identical under noop, Cairo
  and Qt) and the prepared-area accessors are unchanged.
- `Tests`: `Tests/src/PerformanceTest.cpp` (before/after measurement of the step),
  the existing `MapPainter*Test` suite and `Tests/src/MapPainterFrameBuffersTest.cpp`
  (prepared area set and draw order), plus a new unit test for the preparation contract.
- `TODO.md`: the `MapPainter::ProcessAreas` / per-area scratch allocation finding is resolved
  by this change.

No database format, style sheet, import or public API impact. The change is internal to the
map painter and its prepared-data handling; the painter's protected accessors and the
`MapPainter` subclass contract stay as they are.
