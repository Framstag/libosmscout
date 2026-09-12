## Why

The historic section of `stylesheets/map.ost` covers only a handful of `historic=*` values (castle, monument, memorial, ruins, wreck, manor, archaeological_site, battlefield). The OSM wiki documents ~62 `historic=*` values, and taginfo shows many additional documented values with relevant usage (>= 150 objects) that are not discouraged. Without type definitions these features are not importable, searchable, or renderable, even though they represent common historic objects (churches, towers, mines, stones, crosses, ships, vehicles, etc.).

## What Changes

- Add new `TYPE` definitions in the historic section of `stylesheets/map.ost` for documented `historic=*` values with taginfo usage >= 150 that are not discouraged/deprecated.
- Element types (NODE/AREA/WAY) follow the OSM wiki [Key:historic](https://wiki.openstreetmap.org/wiki/Key:historic) element table for each value (e.g. `anchor`, `milestone`, `wayside_cross` are node-only; `aqueduct`, `castle_wall` are way+area; `hollow_way`, `road`, `roman_road` are way-only).
- Add `_building` variant types for building-like values (e.g. `historic_church_building`) so areas tagged with both `historic=*` and `building=*` render as buildings instead of matching the base node/area type.
- Add style definitions in `stylesheets/include/historic.oss` for the new types: area fills, labels, way rendering, and a dedicated vector symbol per node-capable type where an obvious pictogram can be drawn (castle, church, mosque, tower, anchor, cannon, mine, cross, etc.).
- No existing type or style definitions are removed or renamed.

## Capabilities

### New Capabilities
- `historic-type-definitions`: Import-time feature types and rendering styles for documented `historic=*` values missing from `stylesheets/map.ost`, following the pattern of the existing `leisure-type-definitions`, `tourism-type-definitions`, `natural-type-definitions`, `shop-type-definitions`, and `amenity-type-definitions` capabilities.

### Modified Capabilities
<!-- None: no existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — historic section: new `TYPE` definitions (65 base types + 21 `_building` variants).
- `stylesheets/include/historic.oss` — new style rules: area fills, labels, way rendering, 72 new `SYMBOL` definitions and per-type `NODE.ICON` rules.
- No C++ code changes: the `TypeConfig` is derived from the stylesheet at import time.
- Other stylesheets referencing historic types (`standard.oss`, `cycle.oss`, `winter-sports.oss`) are unaffected but may benefit from the new types.
