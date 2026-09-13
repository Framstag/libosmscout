# Proposal: Add missing railway type and style definitions

## Why

`stylesheets/map.ost` defines only 17 of the documented `railway=*` values. Values with significant usage — `railway=switch` (14.7%), `railway=signal` (6.1%), `railway=buffer_stop` (3.3%), `railway=milestone` (3.0%), `railway=stop` (1.9%), `railway=construction`, `railway=proposed`, and others — have no type definition, so they are not importable, searchable, or renderable. The OSM wiki [Key:railway](https://wiki.openstreetmap.org/wiki/Key:railway) and [taginfo](https://taginfo.openstreetmap.org/keys/railway#values) document these values; the stylesheet should cover them like it covers `historic=*`, `amenity=*`, and other keys.

## What Changes

- Add 31 new `TYPE` definitions to `stylesheets/map.ost` for documented `railway=*` values with usage >= 0.02% that are not discouraged:
  - Tracks (WAY): `construction`, `proposed`, `miniature`
  - Stops (NODE): `stop`, `tram_crossing`, `tram_level_crossing`, `train_station_entrance`, `service_station` (NODE AREA)
  - Infrastructure (NODE): `switch`, `signal`, `buffer_stop`, `milestone`, `railway_crossing`, `derail`, `signal_box` (NODE AREA), `junction`, `yard` (NODE AREA), `radio`, `vacancy_detection`, `phone`, `spur_junction`, `rail_brake`, `defect_detector`, `power_supply`, `owner_change`, `crossover`, `platform_marker`, `loading_ramp` (NODE WAY AREA)
  - Ways/Areas: `platform_edge` (WAY), `workshop` (AREA), `ventilation_shaft` (NODE WAY)
- Element types follow the OSM wiki element table for each value.
- Add style definitions to `stylesheets/include/railway.oss` where visualisation is obvious:
  - 5 new symbols: `railway_switch`, `railway_signal`, `railway_buffer_stop`, `railway_milestone`, `railway_stop`
  - NODE.ICON rules reusing existing symbols for `tram_crossing`, `tram_level_crossing`, `railway_crossing`, `train_station_entrance`
  - WAY styles for `construction`, `proposed`, `miniature`, `platform_edge`; AREA style for `yard`
  - Building-area styles for `workshop` and `signal_box` (prevent shadowing generic building types)
- Redesign `railway_station`, `railway_halt`, `railway_tram_stop` symbols to be visually distinct (were identical solid squares); fix `railway_signal` orientation.
- Excluded values: `facility`, `site` (deprecated), `yes` (do not use), `razed`, `dismantled` (controversial, deprecation proposal), values below 0.02% usage (`roundhouse`, `wash`, `traverser`, `water_crane`, ...). `railway=platform` already covered by `public_transport_platform`.

## Capabilities

### New Capabilities
- `railway-type-definitions`: Import-time OSM feature types for documented `railway=*` values missing from `stylesheets/map.ost`, plus corresponding style definitions in `stylesheets/include/railway.oss`.

### Modified Capabilities
<!-- None. No existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — 31 new `TYPE` definitions in the railway section
- `stylesheets/include/railway.oss` — new symbols, NODE.ICON/WAY/AREA style rules, redesigned station/halt/tram_stop symbols, fixed signal symbol
- No C++ code, no API changes, no build system changes
- Validated by `CheckStyleSheet-*.oss` tests (load `map.ost` + stylesheet with `--warning-as-error`) and `SymbolsAll` demo (renders all symbols)
