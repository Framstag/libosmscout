# TextMetricsAll — Cross-Backend Text Metrics Comparison Tool

## Why

Text metrics calculation differs across rendering backends. Bounding box values for the same text, font, and font parameters diverge — visible for example in way shield placement. There is currently no way to compare the metrics produced by each backend against a ground truth, making such bugs hard to spot and fix.

## What Changes

- Add a public text measurement API to the map rendering layer so external tools can obtain label dimensions and per-glyph bounding boxes from any backend.
- Add a manually invoked test application that renders given text with each available backend, draws the text and its bounding boxes onto a stored canvas, and dumps the measured values to the terminal for comparison.
- Provide an independent reference measurement (ground truth) that the backend values are compared against, so differences in implementation become visible.
- Scope: Linux backends only (Cairo, AGG, Qt, Skia, SVG). OpenGL is excluded — it uses a separate text pipeline. Windows/macOS backends (GDI, DirectX, iOSX) are out of scope for this change.

## Capabilities

### New Capabilities
- `text-metrics-api`: Public measurement API on the map painter base class, returning label dimensions and per-glyph bounding boxes for a given text, font, and font size.
- `text-metrics-tool`: Manually invoked comparison application that renders text with each backend, draws bounding boxes on a stored canvas, dumps measured values to the terminal, and compares them against an independent reference.

### Modified Capabilities
- `glyph-bounding-box`: The existing Skia glyph bounding box spec is superseded by the new measurement API contract. Its requirements are folded into `text-metrics-api`.

## Impact

- `libosmscout-map/include/osmscoutmap/MapPainter.h` — new virtual measurement method on the base painter class.
- `libosmscout-map-cairo/` — implement measurement API (Cairo).
- `libosmscout-map-agg/` — implement measurement API (AGG).
- `libosmscout-map-qt/` — implement measurement API (Qt).
- `libosmscout-map-skia/` — implement measurement API (Skia).
- `libosmscout-map-svg/` — implement measurement API (SVG).
- `Demos/CMakeLists.txt` — new demo target for the comparison tool.
- `Demos/src/TextMetricsAll.cpp` — new tool source.
- `Demos/include/` — shared tool infrastructure (argument parsing, output helpers).
- Dependency: FreeType (already a project dependency, used by AGG and OpenGL backends) for the reference measurement.
- No changes to existing rendering behavior; the measurement API is additive.
