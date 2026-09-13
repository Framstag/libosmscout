## Why

The leisure section of `stylesheets/map.ost` is missing many documented `leisure=*` values that have relevant usage in OSM (>= 0.01% per taginfo) and are not discouraged. Without type definitions these features are not importable, searchable, or renderable, even though they represent common leisure facilities (picnic tables, fitness centres, dog parks, sports halls, etc.).

## What Changes

- Add new `TYPE` definitions in the leisure section of `stylesheets/map.ost` for documented `leisure=*` values with taginfo usage >= 0.01% that are not discouraged/deprecated.
- Element types (NODE/AREA/WAY) follow the OSM wiki [Key:leisure](https://wiki.openstreetmap.org/wiki/Key:leisure) element table for each value (e.g. `bleachers` and `swimming_area` are area-only).
- Extend `leisure_slipway` to also accept WAY elements, matching the wiki (node + way).
- Add style definitions in `stylesheets/include/leisure.oss` for the new types where visualisation is obvious, reusing existing patterns (pitch fill, water fill, park fill, label rules).
- No existing type or style definitions are removed or renamed.

## Capabilities

### New Capabilities
- `leisure-type-definitions`: Import-time feature types and rendering styles for documented `leisure=*` values missing from `stylesheets/map.ost`, following the pattern of the existing `tourism-type-definitions`, `natural-type-definitions`, `shop-type-definitions`, and `amenity-type-definitions` capabilities.

### Modified Capabilities
<!-- None: no existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — leisure section: new `TYPE` definitions, `leisure_slipway` element extension.
- `stylesheets/include/leisure.oss` — new style rules for the added types.
- No C++ code changes: the `TypeConfig` is derived from the stylesheet at import time.
- Other stylesheets referencing leisure types (`standard.oss`, `public-transport.oss`, `roads.oss`, `winter-sports.oss`) are unaffected but may benefit from the new types.
