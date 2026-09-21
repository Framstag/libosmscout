## Why

Every area loaded for a view is prepared for drawing, whether or not it lies inside the view. Measured
on the Dortmund database with the Cairo backend at zoom 15, a single-tile view at the city centre loads
17629 areas (17650 rings) and only 726 of those rings (4.1 %) take part in the frame, yet all 17650 rings
are passed through style resolution before any of them is discarded as off-view. The area preparation
step therefore spends most of its work on objects that cannot contribute a pixel, and its cost grows with
the data of the loaded tiles rather than with what the view shows - the wrong scaling for pan and zoom,
where views are redrawn most often.

## What Changes

- The painter SHALL reject an area before its per-ring preparation when that area provably cannot
  contribute a pixel to the current view.
- The rejection SHALL be conservative: it SHALL NOT discard an area that the existing per-ring visibility
  decision would have kept, for the current stylesheet and zoom level.
- The rejection SHALL take precedence over per-ring style resolution and over the per-ring visibility
  decision, so that neither is performed for a rejected area.
- The work of area preparation SHALL scale with the areas that become visible in the view instead of with
  the areas that are loaded for it.
- The prepared area entries, the prepared clipping geometry, the draw order, the prepared ways and the
  rendered output SHALL be unchanged.
- No rendering output change: identical geometry, styling, label placement and draw order.

## Capabilities

### New Capabilities

- `map-painter-area-culling`: contract for the painter's early rejection of areas that cannot be visible -
  conservativeness of the early decision, what that decision precedes, the resulting scaling of area
  preparation work, and the unchanged prepared entries, clipping geometry and rendered output.

### Modified Capabilities

<!-- None. No existing capability's requirements change: area-clippings and the label/rendering
     contracts keep their requirements and are covered by regression tasks. -->

## Impact

Affected modules:

- `libosmscout-map` (core rendering, `MapPainter`):
  - `src/osmscoutmap/MapPainter.cpp` - `ProcessAreas` (the per-area preparation loop over `areas` and
    `poiAreas`), `PrepareAreaRing` (the existing per-ring style resolution and visibility decision, and
    the assertion that the early tolerance covers the per-ring tolerance), `IsVisibleArea` (the
    visibility test the early decision is built on).
- `libosmscout-map` (style configuration, `StyleConfig`):
  - `include/osmscoutmap/StyleConfig.h` and `src/osmscoutmap/StyleConfig.cpp` - the per-level bound of
    the area border width, derived from the loaded stylesheet during the existing per-level
    postprocessing of the area style lookup tables, plus the public accessor for it.
- `Tests`:
  - a new unit test covering the capability's requirements (conservativeness, unchanged prepared areas,
    prepared ways and clipping geometry, work that does not scale with the loaded area count),
    registered in `Tests/CMakeLists.txt` and `Tests/meson.build`.
  - `src/PerformanceTest.cpp` for the before/after measurement of the area preparation step.
- `TODO.md`: the area preparation early-out that no stylesheet reaches, the millimetre border width
  used as a pixel offset in the per-ring visibility decision, and the OpenGL backend's per-data-load
  reprocessing of loaded areas, all recorded as pre-existing findings of this work.

**Public API**: one addition - a documented accessor for the bound of 2.1 on `StyleConfig`, next to the
other per-level style queries. Everything else is internal state of the painter.

No database format, style sheet syntax, import or build system impact.
