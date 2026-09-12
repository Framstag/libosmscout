## Why

`map.ost` defines only 13 of the ~50 `waterway` values documented on the OSM wiki (Key:waterway). Values like `rapids`, `flowline`, `tidal_channel`, `pressurised`, `sluice_gate`, and `fuel` each exceed 500 uses on taginfo yet have no type or style definition, so they render as generic/unstyled objects or are dropped entirely.

## What Changes

- Add new type definitions to `map.ost` for the missing `waterway` values documented in the OSM wiki with relevant usage (>=500 on taginfo) and not discouraged: `flowline`, `tidal_channel`, `drystream`, `pressurised`, `derelict_canal`, `drainage_channel`, `link`, `fairway`, `fish_pass`, `fuel`, `water_point`, `sanitary_dump_station`, `access_point`, `milestone`, `rapids`, `sluice_gate`, `floodgate`, `check_dam`, `floating_barrier`, `flow_control`, `stream_end`, `soakhole`
- Assign correct object types (NODE / WAY / AREA) per the OSM wiki for each new value
- Add corresponding style definitions in `include/waterway.oss`, reusing existing type/style definitions for similar types (e.g. `waterway_stream`, `waterway_drain`, `waterway_weir`, `waterway_boatyard`) where visualization is obvious
- Update GROUP priority definitions in `standard.oss`, `cycle.oss`, `winter-sports.oss`, and `public-transport.oss` so the new types render at the correct priority
- Exclude `artificial` (bad import tag, not used in real mapping), `wadi` and `riverbank` (discouraged on the wiki), `brook` (deprecated), and values not documented on the wiki (`yes`, `tile_line`, `connector`, `sign`, `sewer`, `depth`, `oxbow`, `ford`, `pumping_station`)

## Capabilities

### New Capabilities
- `waterway-type-definitions`: Type definitions in `map.ost` for all relevant `waterway` values, with correct object types (NODE/WAY/AREA) and matching style definitions in `include/waterway.oss`

### Modified Capabilities
<!-- No existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — new type definitions in the waterway section (after `waterway_dam`, lines ~359–463)
- `stylesheets/include/waterway.oss` — new style definitions and symbols for the added types
- `stylesheets/standard.oss`, `stylesheets/cycle.oss`, `stylesheets/winter-sports.oss`, `stylesheets/public-transport.oss` — GROUP priority lines extended with the new types
- No C++ code, API, or dependency changes; purely stylesheet data
- Rendering behavior: previously unstyled `waterway` objects gain styles; no existing type/style is removed or renamed
