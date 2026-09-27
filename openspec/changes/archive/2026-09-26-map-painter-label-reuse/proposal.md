## Why

The label stage is now the cost of a rendered frame. Measured on the Dortmund database with
the Cairo backend, a zoom 16 view of 16 tiles spends 10344 of its 10519 heap allocations per
tile-frame (98%) and 52% of its frame time in the label steps
(`PrepareNodeLabels`, `DrawLabels`, `CalculateWayShields`, `PrepareAreaLabels`,
`DrawWayContourLabels`), while the earlier painter changes reduced area preparation to a
constant allocation count per frame. Every frame measures every registered label again and
rebuilds the label overlap state of the frame, although a label's measurement depends only on
its text, font, font size and wrapping, and those are stable while a view is panned, zoomed
or redrawn. The label steps are therefore the largest remaining per-frame allocation source
and the largest remaining frame-time component in the z16-z17 band, where maps are most often
navigated.

## What Changes

- The measurement of a registered label SHALL be reused across the frames of a view whenever
  the measurement inputs are unchanged, instead of being performed again for every frame.
- The per-glyph representation a backend derives from a measured label SHALL be reused when
  that label takes part in more than one frame.
- The scratch storage of the label stage SHALL be reused instead of being allocated per
  object, per label and per frame: the per-object label list, the per-label element, mask and
  canvas storage, the frame's label overlap state, and the geometry that path labels are
  placed on.
- The label set, the label placement, the label draw order and the rendered output SHALL be
  unchanged, including for equal-comparing labels.
- The results of the text measurement API SHALL be unchanged.
- Reused measurements and glyphs SHALL be observable and invalidatable by the painter, so that
  a backend whose measurement depends on its current drawing target can discard them.
- No rendering output change: identical geometry, styling, label placement and draw order.

## Capabilities

### New Capabilities

- `map-painter-label-reuse`: contract for the label stage of the map painter - reuse of label
  measurement and per-glyph data across frames, reuse of the label stage's scratch storage,
  the conditions under which reuse is allowed, and an unchanged label set, placement, draw
  order, rendered output and measurement result.

### Modified Capabilities

<!-- None: no existing capability's requirements change. text-metrics-api, label-layout,
     label-registration, label-drawing, label-rendering, glyph-bounding-box and
     shield-label-rendering keep their requirements; the reuse of measurements has to keep
     their results identical and is covered by regression tasks. -->

## Impact

Affected modules:

- `libosmscout-map` (core rendering, `MapPainter` and the label layouter):
  - `include/osmscoutmap/MapPainter.h` - the label stage's reusable scratch storage, the
    text measurement entry point used by the tools.
  - `src/osmscoutmap/MapPainter.cpp` - `LayoutPointLabels`, `RegisterPointWayLabel`,
    `DrawWayContourLabel`, `CalculateWayShieldLabels`, `CalculateWayShields`,
    `PrepareAreaLabels`, `PrepareNodeLabels`, `PrepareRouteLabels`, `DrawLabels`.
  - `include/osmscoutmap/LabelLayouter.h` - `LayoutJob`, `Label`, `RegisterLabel`,
    `RegisterContourLabel`, `ProcessLabel`, `ProcessLabelInstance`,
    `ProcessLabelContourLabel`, `ProcessLabels`, `Layout`, `DrawLabels`.
  - `include/osmscoutmap/LabelLayouterHelper.h` and
    `src/osmscoutmap/LabelLayouterHelper.cpp` - `ScreenRectMask` and `ScreenMask`, which hold
    the per-label mask and the frame's overlap state.
- Backends that measure text and derive glyphs from a measured label: `libosmscout-map-cairo`,
  `libosmscout-map-qt`, `libosmscout-map-skia`, `libosmscout-map-svg`, `libosmscout-map-agg`,
  `libosmscout-map-gdi`, `libosmscout-map-directx`. The Cairo backend is the one whose
  measurement depends on its current drawing target, because a drawing call installs a new
  target per frame.
- `Tests`: `Tests/src/PerformanceTest.cpp` (before/after measurement of the label steps),
  `Tests/src/MapPainterRouteTest.cpp`, `Tests/src/MapPainterShieldTest.cpp`,
  `Tests/src/MapPainterShieldQtTest.cpp` and the text metric tests (label placement, shield
  geometry and measurement regressions), plus a new unit test for the reuse and allocation
  requirements.
- `TODO.md`: the label scratch-vector finding is resolved by this change.

Public API impact: none intended. The painter's label stage is internal state, the painter's
protected hooks and the text measurement API keep their signatures, and the backends keep
implementing the same measurement and glyph hooks. In-repo consumers are unaffected.

No database format, style sheet, import or build system impact.
