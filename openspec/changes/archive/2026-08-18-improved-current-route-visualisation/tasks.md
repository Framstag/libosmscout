# Tasks: improved-current-route-visualisation

## 1. Core renderer — route draw order

- [x] 1.1 Add a named constant for the route layer value (`static constexpr int8_t routeLayer = 100;` in `libosmscout-map/include/osmscoutmap/MapPainter.h`), documented as "above all OSM layers" (spec: `route-rendering` "Route drawn above all map ways", design: D1)
- [x] 1.2 Give the internal `_route` type the layer feature (`TypeConfig::Initialize`, `libosmscout/src/osmscout/TypeConfig.cpp`) and have route renderers set `LayerFeatureValue` with `MapPainter::routeLayer` on the route way (`OverlayObject` in the Qt client, JNI client). `CalculateWayPaths` and `ProcessRoutes` then honor the layer via `CalculateLineLayer` — the renderer stays generic and needs no type-name check (spec: `route-rendering` "Route drawn above all map ways", design: D1)

## 2. Core renderer — route slots

- [x] 2.1 Restructure `FlushRouteData` in `ProcessRoutes` to iterate all `lineStyles` (not just `front()`), computing `CalculateLineColor`/`CalculateLineWith` per style and pushing one `WayData` per transSegment per style (spec: `route-rendering` "Route supports multiple line styles", design: D2)
- [x] 2.2 Verify sidecar/offset logic (lineOffset, `OffsetRel::sidecar`, connecting segments) still uses the first line style and behaves as before for existing sidecar stylesheets (design: D2)

## 3. Stylesheet — cased route

- [x] 3.1 In `stylesheets/include/route.oss`, replace the single `[TYPE _route] WAY` rule with a cased pair following the roads.oss pattern: `WAY#outline` (white, displayWidth 2.2mm, width 8m, priority 99) + `WAY` (fill `@routeColor`, displayWidth 1.5mm, width 6m, priority 100) (spec: `route-visualization` "Route type definitions in stylesheet", design: D3)

## 4. Tests

- [x] 4.1 Add a Catch2 unit test asserting `WayData::operator<` places a route-layer way after a bridge-layer way (layer 1) and after a normal way (layer 0) (spec: `route-rendering` "Route drawn above all map ways")
- [x] 4.2 Add a Catch2 test (`Tests/src/MapPainterRouteTest.cpp`, Cairo backend) rendering a `MapData` with a route over a bridge way, asserting route pixels are painted on top (spec: `route-rendering` "Route drawn above all map ways")
- [x] 4.3 Verify cased route style renders outline below fill: the same render test asserts the white outline is visible around the red fill (spec: `route-visualization` "Route type definitions in stylesheet")

## 5. Validation

- [x] 5.1 Build and run the test suite: `cmake --build build && cd build && ctest -j 2 --output-on-failure` — 100/107 pass; the 7 failures are pre-existing `PerformanceTest-opengl-*` (missing shader files in this environment, unrelated to this change)
- [x] 5.2 Visual check: the render test verifies at pixel level that the route renders above a bridge with a visible white casing; `CheckStyleSheet-standard.oss` confirms the updated `route.oss` loads. A GUI check in OSMScout2 requires a display and remains a manual follow-up
