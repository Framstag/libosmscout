# Fix Text Metrics and Shield Label Rendering Inconsistencies

## Why

Shield labels (road number badges) are rendered differently by each map backend — most visibly, in the Cairo backend the label text is not centered within the shield borders. The `TextMetricsAll` demo confirms the underlying cause: text measurement results differ substantially between backends, and the Cairo backend (both with and without Pango) returns per-glyph bounding boxes that do not reflect the actual glyphs (constant font-box dimensions instead of per-glyph ink extents). Broken metrics propagate into label layouting and shield drawing, so measurement fixes and shield-drawing alignment fixes must land together.

## What Changes

- Cairo and SVG backends return correct, per-glyph ink-based bounding boxes from the text measurement API (currently they return a constant font-level rectangle for every glyph when Pango is enabled).
- Cairo backend (both text paths: with Pango and with plain Cairo) measures label dimensions consistently with the other backends.
- All backends position label text so the measured label rectangle corresponds to the actually drawn text — the drawn ink must coincide with the label rectangle used for layouting and shield geometry.
- Shield labels are drawn with consistent, symmetric padding around the text in all backends, so the text is visually centered inside the shield background and border in Cairo, Qt, and the other backends alike.

## Capabilities

### New Capabilities
- `shield-label-rendering`: Cross-backend rendering of shield labels (background, border, text) with consistent, symmetric geometry so the label text is centered within the shield.

### Modified Capabilities
- `text-metrics-api`: The measurement consistency requirement is extended to hold for both Cairo text stack variants (with and without Pango), and a new requirement fixes the semantics of the returned label dimensions (visual/ink extents, not font-box extents).

## Impact

- `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` — text measurement/layout (Pango path and plain-Cairo path), `GlyphBoundingBox()`, `Layout()`, `DrawLabel()` shield branch
- `libosmscout-map-cairo/include/osmscoutmapcairo/MapPainterCairo.h` — internal label/font struct fields if needed
- `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp` — shares the same Pango glyph-extents code shape as the Cairo Pango path
- `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp` — shield drawing branch (align padding with other backends)
- `libosmscout-map/src/osmscoutmap/MapPainter.cpp` + `include/osmscoutmap/MapPainter.h` — shared `TextMetrics`/`MeasureLabel` helper; documentation of label dimension semantics
- `libosmscout-map/include/osmscoutmap/LabelLayouter.h` — label rectangle placement (centering) semantics, if measurement semantics change require it
- `Demos/src/TextMetricsAll.cpp` — used as verification harness; may gain a cross-backend comparison summary output
- Tests: `Tests/src/TextMetricsTest.cpp`, `Tests/src/TextMetricsQtTest.cpp`, `Tests/src/TextMetricsReferenceTest.cpp` — new/adjusted test cases; test data unchanged
- No public API signature changes expected; `TextMetrics` struct field meanings are clarified (behavioral contract, not signature change)