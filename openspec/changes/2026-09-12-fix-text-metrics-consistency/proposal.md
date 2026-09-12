## Why

Text measurement is inconsistent in two places the `fix-text-metrics-shields` change did not reach:

1. The SVG map backend, when built without Pango, measures labels with a character-count approximation (`AverageCharacterWidth`) instead of real font metrics. Measured label width at 30 px font is ~340 px against ~216 px of actual ink — roughly 57% too wide. This breaks the `text-metrics-api` contract that all backends report consistent measurements, and makes SVG shields/labels come out oversized on any build without Pango.
2. The `TextMetricsAll` demo's FreeType reference reports `face->size->metrics.height` (the font box) as the label/line height, while the ink-semantics contract used by the map backends expects ink height. The demo's reference label height therefore disagrees with the backends (35 vs 23 at 30 px), so the demo is not a valid reference for the behavior it is supposed to verify.

Both are pre-existing, not initiated by `fix-text-metrics-shields`. Process as a separate change.

## What Changes

- The SVG map backend reports text metrics that are consistent whether it is built with or without Pango; the non-Pango variant no longer diverges from real font metrics.
- The `TextMetricsAll` demo's FreeType reference reports ink height, consistent with the ink-semantics contract verified by the text metrics tests.
- Both behaviors are covered by automated comparisons so drift is caught by the test suite.

## Capabilities

### New Capabilities
- `text-metrics-consistency`: Consistency of text measurement between the SVG backend's Pango and non-Pango variants, and between the `TextMetricsAll` reference and the map backends' ink height semantics.

### Modified Capabilities
<!-- No existing spec-level behavior changes. -->

## Impact

- `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp` — non-Pango `Layout()`, `ToGlyphs()`, `GlyphBoundingBox()`; accurate measurement replaces `AverageCharacterWidth` approximation. The Pango path is already correct and must not regress.
- `libosmscout-map-svg/CMakeLists.txt` + `libosmscout-map-svg/meson.build` — dependency adjustments if the non-Pango measurement path needs font-metric access (e.g. FreeType), mirroring the Cairo backend's non-Pango path.
- `libosmscout-map-svg/include/osmscoutmapsvg/MapPainterSVG.h` — any support declarations for the new measurement path.
- `Demos/src/TextMetricsAll.cpp` (+ `Demos/include/TextMetricsAll.h`) — FreeType reference reports ink height instead of font box height.
- `Tests/src/TextMetricsCairoTest.cpp` (or a sibling SVG text metrics test) — comparison coverage for the non-Pango SVG path and reference height semantics.
