## Why

The painter prepares ways, nodes and labels for every object that is loaded
into a view, whether or not the object can contribute a pixel. Measured on the
Dortmund database with the Cairo backend on a single-tile view at zoom 15
(51.514 7.463 - 51.510 7.470), the view loads 6308 ways and 17629 areas, of
which 335 ways and 726 areas take part in the frame: about 5 % of the loaded
objects are used. The same view spends 22 % of its painter-side frame time
(the noop backend, which isolates painter work from backend drawing) in the
way preparation step `CalculatePaths`, which runs for all 6308 loaded ways, and
the node label step allocates 323 objects per frame for 164 loaded nodes; at
zoom 16 and zoom 17 the node label step is 57 % - 72 % of all frame
allocations with 4815 and 7080 loaded nodes. Across a zoom band (13 - 17) the
way preparation step is 18 % - 44 % of the painter-side frame in every view
measured.

The recently completed area work (`map-painter-area-preparation`,
`map-painter-area-visibility-cull`) established that area preparation must
follow the view and not the loaded data. Ways, nodes and the label stage never
received that treatment: a label is measured and stored as soon as it is
registered, and the decision that the label lies outside the view is taken at
draw time, after the work has been done. The measured steps are
backend-independent - the noop, Cairo and Qt runs report the same work for them
- so every map application on every backend pays for it, and the cost grows
with the loaded tile set rather than with what the view shows, which is the
wrong scaling for pan and zoom, where views are redrawn most often.

## What Changes

- The painter SHALL reject a way before its style resolution, before its shield
  labels are registered and before its geometry is transformed, when that way
  provably cannot contribute a pixel to the current view.
- The painter SHALL reject a loaded point object (node or POI node) before its
  style resolution when neither its icon, its symbol nor its label rectangle
  can intersect the current view.
- The painter SHALL not measure, not store and not lay out a label whose
  rectangle cannot intersect the current view, for every source of labels
  (nodes, areas, way shields, contour labels).
- Every rejection SHALL be conservative: it SHALL NOT discard an object or a
  label that the existing per-object decision would have kept, for the current
  stylesheet, zoom level and map parameters.
- Every rejection SHALL take precedence over the work it removes, so that the
  removed work is not performed for a rejected object or label.
- The work of way preparation, node preparation and the label stage SHALL scale
  with the objects that become visible in the view instead of with the objects
  that are loaded for it.
- The prepared ways and paths, the prepared node labels, the label set, the
  label placement, the draw order and the rendered output SHALL be unchanged.
- No rendering output change: identical geometry, styling, icon and symbol
  placement, label placement and draw order.

## Capabilities

### New Capabilities

- `map-painter-way-culling`: contract for the painter's early rejection of ways
  that cannot be visible - conservativeness of the early decision, what the
  decision precedes (style resolution, shield label registration, geometry
  transformation), the resulting scaling of way preparation and shield work,
  and the unchanged prepared ways, draw order and rendered output.
- `map-painter-point-object-culling`: contract for the painter's early
  rejection of loaded point objects (nodes, POI nodes) whose visual extent
  cannot reach the view - conservativeness with respect to icons, symbols and
  labels, what the decision precedes, the resulting scaling of node
  preparation, and the unchanged prepared label elements and rendered output.
- `map-painter-label-culling`: contract for the volume of the label stage - a
  label whose rectangle cannot intersect the view is not measured, not stored
  and not laid out, for every label source; conservativeness of the decision
  against the existing label rectangle, the resulting scaling of the label
  stage's allocation and time budget, and an unchanged label set, label
  placement, draw order and measurement result.

### Modified Capabilities

<!-- None. label-registration, label-layout, label-drawing, label-rendering,
     shield-label-rendering and glyph-bounding-box keep their requirements:
     which backend hooks exist and what they do does not change, and the
     rejection keeps their results identical. area-clippings and the existing
     painting contracts are covered by regression tasks. -->

## Impact

Affected modules:

- `libosmscout-map` (core rendering, `MapPainter`):
  - `include/osmscoutmap/MapPainter.h` and `src/osmscoutmap/MapPainter.cpp` -
    `CalculatePaths` / `CalculateWayPaths` (the per-way style resolution, line
    metric and transform sites), `CalculateWayShields` /
    `CalculateWayShieldLabels` / `RegisterPointWayLabel` (the way-level shield
    registration), `PrepareNodes` / `PrepareNode` / `LayoutPointLabels` (the
    per-point-object style resolution and label registration), plus the
    visibility decisions they are built on.
- `libosmscout-map` (style configuration, `StyleConfig`):
  - `include/osmscoutmap/StyleConfig.h` and `src/osmscoutmap/StyleConfig.cpp` -
    the per-level visibility bounds (the widest reach a drawn way line can
    have, the widest icon and symbol the level can resolve, and the label
    extent the map parameters allow), derived from the loaded stylesheet and
    the map parameters during the existing per-level postprocessing of the
    style lookup tables, in the same shape as the area border width bound the
    area visibility cull introduces.
- `libosmscout-map` (label stage, `LabelLayouter`):
  - `include/osmscoutmap/LabelLayouter.h` - the registration and measurement
    path, the label element geometry and the existing viewport decision of the
    drawing path, which the rejection is aligned with.
- Backends (`libosmscout-map-cairo`, `libosmscout-map-qt`,
  `libosmscout-map-skia`, `libosmscout-map-svg`, `libosmscout-map-agg`,
  `libosmscout-map-gdi`, `libosmscout-map-directx`, `libosmscout-map-iosx`,
  Android JNI painter): no source change expected - the measured steps are
  backend-independent, and the label layouter's registration path is shared by
  all of them through their unchanged hooks.
- `Tests`: a new unit test per capability (rejection precedes the removed work,
  conservativeness at the tolerance, unchanged prepared ways / label elements /
  label placement) registered in `Tests/CMakeLists.txt` and `Tests/meson.build`,
  `Tests/src/PerformanceTest.cpp` for the before/after measurement of the three
  steps, and the existing `MapPainterShieldTest`, `MapPainterShieldQtTest`,
  `MapPainterRouteTest`, `MapPainterAreaPreparationTest` and frame buffer tests
  as regression guards for shield geometry, label placement and prepared data.
- `TODO.md`: the loader-side over-fetch found during this analysis (a zoom 14
  tile loads 1621 areas for a view whose zoom 15 tile, four times smaller,
  loads 17629; the database load step costs 91.9 ms against a 15.2 ms
  painter-side frame at zoom 15) is recorded there as a pre-existing finding,
  because it is the root of the loaded-versus-visible gap this change culls
  downstream and is not addressed here.

Dependencies and ordering, not part of this change:

- The level bound and the early rejection pattern this change follows are
  introduced by `map-painter-area-visibility-cull`, which is on a branch and
  not merged into `master`; the work here builds on that accessor and pattern.
- `map-painter-label-reuse` (branch) changes the measurement, glyph and scratch
  handling of the same label stage functions (`CalculateWayShields`,
  `RegisterPointWayLabel`); the two changes must be sequenced or rebased against
  each other, because reuse of a measurement and rejection of a label touch the
  same call path.

**Public API**: one addition - a documented per-level visibility bound accessor
on `StyleConfig`, next to the other per-level style queries (the same shape as
the area border width bound accessor). Everything else is internal state of the
painter and of the label layouter.

No database format, style sheet syntax, import or build system impact.
