## Why

The man made section of `stylesheets/map.ost` only defines 3 of the many documented `man_made=*` values (bridge, pier, wastewater_plant). The OSM wiki [Key:man_made](https://wiki.openstreetmap.org/wiki/Key:man_made) documents 195 values; taginfo shows 73 additional documented values with usage >= 0.05% that are not discouraged. Without type definitions these features are not importable or renderable, even though they represent common structures (storage tanks, towers, masts, silos, pipelines, wells, windmills, lighthouses, etc.).

## What Changes

- Add new `TYPE` definitions in the man made section of `stylesheets/map.ost` for documented `man_made=*` values with taginfo usage >= 0.05% that are not discouraged/deprecated.
- Element types (NODE/AREA/WAY) follow the OSM wiki [Key:man_made](https://wiki.openstreetmap.org/wiki/Key:man_made) element table and the individual tag pages for each value (e.g. `cutline`, `pipeline`, `embankment`, `goods_conveyor`, `cooling`, `snow_fence` are way-only; `manhole`, `mast`, `surveillance`, `survey_point`, `utility_pole`, `petroleum_well`, `flagpole`, `chimney`, `beehive`, `charge_point`, `cross`, `adit`, `lighthouse`, `snow_cannon`, `flare`, `beacon`, `telephone_box`, `oil_gas_separator`, `buoy`, `fuel_pump`, `carpet_hanger` are node-only; `clearcut`, `tunnel`, `spoil_heap`, `tailings_pond` are area-only; the rest are node + area, node + way, way + area, or node + way + area as documented).
- Discouraged values (`man_made=yes`, `man_made=lamp`, `man_made=pumping_rig`, `man_made=launch_pad`) and undocumented values (`pond`, `tar_kiln`, `marsh_terrace`, `waterway`, `pole`) are excluded.
- Add style definitions in `stylesheets/include/man_made.oss` for the new types where visualisation is obvious, reusing existing patterns (industrial area fills, pier/dam way rendering, node icons).
- No existing type or style definitions are removed or renamed.

## Capabilities

### New Capabilities
- `man-made-type-definitions`: Import-time feature types and rendering styles for documented `man_made=*` values missing from `stylesheets/map.ost`, following the pattern of the existing `barrier-type-definitions`, `power-type-definitions`, `waterway-type-definitions`, and `amenity-type-definitions` capabilities.

### Modified Capabilities
<!-- None: no existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — man made section: 73 new `TYPE` definitions.
- `stylesheets/include/man_made.oss` — new symbols and style rules for the added types.
- No C++ code changes: the `TypeConfig` is derived from the stylesheet at import time.
- Other stylesheets referencing man made types (`standard.oss`, `cycle.oss`, `winter-sports.oss`) are unaffected but may benefit from the new types.
