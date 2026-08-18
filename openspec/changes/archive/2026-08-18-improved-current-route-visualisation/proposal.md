# Proposal: improved-current-route-visualisation

## What Changes

The active route is currently rendered as a single flat red polyline that (a) is drawn *under* bridges and other ways with a positive OSM `layer` value, and (b) blends into red primary roads because it has no outline/casing.

Root causes found in the core rendering pipeline:

1. **Draw order**: `MapPainter::AfterPreprocessing` sorts `wayData` with `WayData::operator<` (layer → zIndex → style priority → way priority). Route segments are pushed to `wayData` with a hardcoded `layer=0` (`MapPainter::ProcessRoutes`), so any way with `layer>0` (bridges, elevated roads) sorts after the route and is painted on top of it.
2. **Single line style**: `ProcessRoutes` uses only `lineStyles.front()` — the code comment says "slots for routes are not supported yet". The style infrastructure (`StyleConfig::GetRouteLineStyles`) already returns a sorted vector of all matching line styles, but the route renderer discards all but the first. This makes a cased route (outline + fill) impossible via stylesheet alone.
3. **Style**: `stylesheets/include/route.oss` defines `_route` as flat `#ff000088` (red, 53% alpha) at 1.5mm. Primary roads are `#ec4044` red, so the route is hard to distinguish on them.

This change:

- Draws the route above all other ways (including bridges) by giving the `_route` type the layer feature and having callers set a high layer value (`MapPainter::routeLayer`) on the route way; the renderer stays generic and honors the layer via `CalculateLineLayer`.
- Adds route slot support so multiple line styles can be applied to one route.
- Adds a white casing + red fill style for the active route so it stands out on any road color, including red primary roads.

## Capabilities

### New Capabilities

- `route-rendering`: Core route draw order and multi-style (slot) rendering in the `MapPainter` pipeline. Covers the guarantee that the active route is painted above all map ways, and that a route can be rendered with multiple stacked line styles (casing).

### Modified Capabilities

- `route-visualization`: The `_route` WAY style changes from a flat red line to a cased line (white outline + red fill). The existing requirement "Route type definitions in stylesheet" (red color, 1.5mm display width) is updated to describe the cased style.

## Impact

- `libosmscout-map/src/osmscoutmap/MapPainter.cpp` — `ProcessRoutes`: honor the layer of route segments via `CalculateLineLayer`; iterate all `lineStyles` (slots) instead of `front()`.
- `libosmscout/src/osmscout/TypeConfig.cpp` — add the layer feature to the internal `_route` type so callers can stack the route via a layer value.
- `libosmscout-map/include/osmscoutmap/MapPainter.h` — possibly a named constant for the route layer value.
- `stylesheets/include/route.oss` — add `WAY#outline` casing rule + fill rule for `_route`, following the existing casing pattern used for roads (`WAY#outline` with lower priority + `WAY` fill).
- All `MapPainter`-derived backends (AGG, Cairo, Qt, SVG, DirectX, GDI, iOSX) benefit automatically — they share the base `DrawWays`/`ProcessRoutes` pipeline. No per-backend changes expected.
- `libosmscout-map-opengl` — out of scope: `MapPainterOpenGL` is a standalone renderer with no route support today.
- Tests — no existing test exercises `MapData::routes`; a render test verifying route-above-bridge draw order and cased style output should be added.
