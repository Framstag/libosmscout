# Design: improved-current-route-visualisation

## Context

The active route is rendered by `MapPainter::ProcessRoutes`, which pushes route segments into the shared `wayData` list. `MapPainter::AfterPreprocessing` sorts `wayData` with `WayData::operator<` (layer → zIndex → style priority → way priority). Route segments are hardcoded to `layer=0`, so bridges (`layer>0`) paint over them.

`ProcessRoutes` also uses only `lineStyles.front()` — the code comment says "slots for routes are not supported yet". `StyleConfig::GetRouteLineStyles` already returns a sorted vector of all matching line styles, but the renderer discards all but the first.

The base `MapPainter` pipeline is shared by AGG, Cairo, Qt, SVG, DirectX, GDI, and iOSX backends. `MapPainterOpenGL` is a standalone renderer with no route support and is out of scope.

See proposal.md for motivation, specs/ for requirements.

## Goals / Non-Goals

**Goals:**
- Route always painted above all map ways, including bridges and tunnels.
- Route renderable with multiple stacked line styles (casing) via stylesheet.
- Cased route style (white outline + red fill) visible on red primary roads.
- No per-backend changes — all MapPainter-derived backends benefit from base-pipeline changes.

**Non-Goals:**
- Route support in the OpenGL renderer (standalone, no route handling today).
- Changes to route labels, start/end markers, or sidecar route behavior.
- Animated or dashed route effects.

## Decisions

### D1: Route stacking via a layer feature value, renderer stays generic

Set the layer of route segments to a high value instead of `0`. The existing `wayData.sort()` then places all route segments after every map way (OSM layer values are typically -5..5).

The `MapPainter` is generic regarding concrete types: it must not know the `_route` type name or any other concrete type from `map.ost` / programmatic type definitions. Instead, the layer comes from the way data itself:

- The internal `_route` type (registered in `TypeConfig::Initialize`) now carries the layer feature (`LayerFeature`).
- Callers that render the active route as an overlay (`OverlayObject` in the Qt client, the JNI client) set a `LayerFeatureValue` with `MapPainter::routeLayer` (100) on the route way.
- `CalculateLineLayer` then returns 100 for the route way, so both paths need no type check:
  - `CalculateWayPaths` (the active route is added to `MapData::poiWays` as a `_route` way by OSMScout2's `MapRenderer` and JavaScout's JNI): the layer value rides in the way's feature buffer.
  - `ProcessRoutes` (DB route relations from `data.routes`): uses `CalculateLineLayer` uniformly, so a caller-supplied layer value on a route buffer is honored and DB route relations keep their own layer (0 unless tagged).
- Constant: `static constexpr int8_t routeLayer = 100;` in `MapPainter.h`, documented as "above all OSM layers". It is a suggested value for callers, not a renderer special case.

Chosen over a dedicated `DrawRoutes` render step: minimal, no new render-step plumbing across backends.
Alternative considered: renderer-side type-name check (`GetName()=="_route"`, precedent in `OSMScoutClient.cpp` / `OverlayObject.cpp`) — rejected because it couples the generic renderer to a concrete type name. `TypeInfo::IsRoute()` was also considered and rejected — it identifies OSM route relations (`route_bicycle` etc.), not the internal `_route` type.

### D2: Route slots — iterate all line styles

Restructure `ProcessRoutes` so each route segment produces one `WayData` entry per matching line style, instead of only `lineStyles.front()`:

- Keep the existing segment-building loop (transSegments, sidecar offsets, connecting segments) driven by the first line style — casing styles use `OffsetRel::base`, so sidecar/offset behavior is unaffected.
- In `FlushRouteData`, iterate all `lineStyles`; for each, compute `CalculateLineColor`/`CalculateLineWith` and push one `WayData` per transSegment.
- Paint order between outline and fill is handled by the existing `wayData.sort()`: the outline rule gets a lower style priority than the fill, so it is drawn first (below).
- The color-collapse check (`memberWay->second.colors`) keeps using the first style's color, preserving existing sidecar-route collapse behavior.

### D3: Cased route style in route.oss

Follow the existing road casing pattern (`WAY#outline` with lower priority + `WAY` fill, see roads.oss):

```
[TYPE _route] WAY#outline { color: #ffffff; displayWidth: 2.2mm; width: 8m; priority: 99; }
[TYPE _route] WAY { color: @routeColor; displayWidth: 1.5mm; width: 6m; priority: 100; }
```

- White outline provides contrast on any road color, including red primary roads (`#ec4044`).
- Fill keeps `@routeColor` (`#ff000088`) and the existing 1.5mm display width.
- Outline is 0.7mm wider than the fill so the casing is visible.

## Risks / Trade-offs

- **High layer value**: if OSM data ever used `layer > 100` (unrealistic; typical range -5..5), the route would paint under it. Acceptable.
- **Caller responsibility**: stacking is now enforced by the caller setting the layer feature value on the route way; the renderer no longer special-cases route types. Both current callers (Qt `OverlayObject` default, JavaScout JNI) set `MapPainter::routeLayer`; a future caller that forgets gets the route on layer 0 again.
- **Slots + sidecar routes**: multiple styles on sidecar routes could interact with the sidecar offset logic. Mitigated: sidecar decisions use the first style; casing styles use base offset. Existing sidecar stylesheets (cycle.oss) define a single ROUTE rule per type, so behavior is unchanged there.
- **Performance**: two styles per route = two draw calls per segment instead of one. Negligible for typical route lengths.
- **Color collapse**: with slots, two styles sharing a color on the same way would both draw (collapse is per first-style color). Not an issue for the casing use case (outline and fill differ).
- **Style change affects all apps**: the cased route style applies to every MapPainter-derived backend and app (OSMScout2, JavaScout, demos). Visual change is intentional and consistent with the roads casing pattern.
