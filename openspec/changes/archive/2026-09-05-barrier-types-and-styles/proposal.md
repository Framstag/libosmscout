## Why

The barrier section of `stylesheets/map.ost` is missing many documented `barrier=*` values that have relevant usage in OSM (>= 0.01% per taginfo) and are not discouraged. Without type definitions these features are not importable or renderable, even though they represent common barriers (kerbs, guard rails, swing gates, chains, jersey barriers, turnstiles, etc.).

## What Changes

- Add new `TYPE` definitions in the barrier section of `stylesheets/map.ost` for documented `barrier=*` values with taginfo usage >= 0.01% that are not discouraged/deprecated.
- Element types (NODE/AREA/WAY) follow the OSM wiki [Key:barrier](https://wiki.openstreetmap.org/wiki/Key:barrier) element table and the individual tag pages for each value (e.g. `guard_rail` and `handrail` are way-only, `swing_gate` and `turnstile` are node-only, `kerb` and `chain` are node + way, `avalanche_protection` is node + way + area).
- Node-only barrier types follow the existing convention of the barrier section: defined with `IGNORE` (like `barrier_gate`, `barrier_lift_gate`, `barrier_stile`), so matching objects do not mismatch with other types.
- Add style definitions in `stylesheets/include/man_made.oss` for the new linear barrier types where visualisation is obvious, reusing existing patterns (fence/wall line rendering).
- No existing type or style definitions are removed or renamed.

## Capabilities

### New Capabilities
- `barrier-type-definitions`: Import-time feature types and rendering styles for documented `barrier=*` values missing from `stylesheets/map.ost`, following the pattern of the existing `leisure-type-definitions`, `natural-type-definitions`, `aeroway-type-definitions`, and `amenity-type-definitions` capabilities.

### Modified Capabilities
<!-- None: no existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — barrier section: new `TYPE` definitions.
- `stylesheets/include/man_made.oss` — new style rules for the added linear barrier types.
- No C++ code changes: the `TypeConfig` is derived from the stylesheet at import time.
- Other stylesheets referencing barrier types (`standard.oss`, `cycle.oss`, `winter-sports.oss`) are unaffected but may benefit from the new types.
