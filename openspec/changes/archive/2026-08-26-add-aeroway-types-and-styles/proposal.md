# Proposal: Add missing aeroway type and style definitions

## Why

`stylesheets/map.ost` defines only 7 of the documented `aeroway=*` values. Values with significant usage — `aeroway=navigationaid` (21.5%), `aeroway=parking_position` (11.6%), `aeroway=hangar` (6.9%), `aeroway=holding_position` (3.8%), `aeroway=taxilane` (1.7%), `aeroway=jet_bridge` (1.2%), `aeroway=windsock` (1.0%), `aeroway=airstrip` (0.5%), and others — have no type definition, so they are not importable, searchable, or renderable. The OSM wiki [Key:aeroway](https://wiki.openstreetmap.org/wiki/Key:aeroway) and [taginfo](https://taginfo.openstreetmap.org/keys/aeroway#values) document these values; the stylesheet should cover them like it covers `railway=*`, `historic=*`, `amenity=*`, and other keys.

## What Changes

- Add 15 new `TYPE` definitions to `stylesheets/map.ost` for documented `aeroway=*` values with usage >= 0.05% that are not discouraged:
  - NODE: `navigationaid`, `windsock`, `threshold`, `aircraft_crossing`
  - NODE WAY: `parking_position`, `holding_position`
  - NODE AREA: `hangar`, `airstrip`, `tower`, `heliport`, `fuel`
  - WAY: `taxilane`, `jet_bridge`, `stopway`, `model_runway`
- Element types follow the OSM wiki element table for each value (cross-checked against the individual `Tag:aeroway=*` pages).
- Add style definitions to `stylesheets/include/aeroway.oss` where visualisation is obvious:
  - New symbols: `aeroway_windsock`, `aeroway_heliport`, `aeroway_fuel`, `aeroway_tower`, `aeroway_navigationaid`, `aeroway_aircraft_crossing`, `aeroway_threshold`, `aeroway_holding_position`, `aeroway_parking_position`
  - NODE.ICON rules at veryClose zoom for the new node types
  - WAY styles for `taxilane`, `jet_bridge`, `stopway`, `model_runway` (reusing the runway/taxiway color)
  - AREA styles for `hangar`, `airstrip`, `heliport`, `tower`, `fuel` (building-like areas get building styles to prevent shadowing generic `building` rendering)
- Excluded values: `marking` (draft proposal — discouraged), `aerodrome_marking`, `communication`, `shelter` (undocumented), `control_center`, `highway_strip`, `spaceport`, `launchpad`, `launch_complex`, `arresting_gear`, `model_taxiway` (below 0.05% usage), `control_tower` (deprecated in favor of `tower`).

## Capabilities

### New Capabilities
- `aeroway-type-definitions`: Import-time OSM feature types for documented `aeroway=*` values missing from `stylesheets/map.ost`, plus corresponding style definitions in `stylesheets/include/aeroway.oss`.

### Modified Capabilities
<!-- None. No existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — 15 new `TYPE` definitions in the air transport section
- `stylesheets/include/aeroway.oss` — new symbols, NODE.ICON/WAY/AREA style rules
- `stylesheets/standard.oss` — ORDER WAYS may need the new way types (`taxilane`, `jet_bridge`, `stopway`, `model_runway`) added to the aeroway group
- No C++ code, no API changes, no build system changes
- Validated by `CheckStyleSheet-*.oss` tests (load `map.ost` + stylesheet with `--warning-as-error`) and `SymbolsAll` demo (renders all symbols)
