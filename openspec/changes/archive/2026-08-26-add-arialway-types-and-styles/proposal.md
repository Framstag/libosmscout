## Why

`stylesheets/map.ost` defines only three `aerialway=*` types (`gondola`, `chair_lift`, `drag_lift`), folding several documented values into them. Four documented, widely used values are missing entirely: `pylon` (~144k uses), `station` (~36k uses), `zip_line` (~2.8k uses), and `goods` (~2.8k uses). These features are therefore not importable, searchable, or renderable in libosmscout.

## What Changes

- Add `aerialway_pylon` type (NODE) to `stylesheets/map.ost`
- Add `aerialway_station` type (NODE AREA) to `stylesheets/map.ost`
- Add `aerialway_zip_line` type (WAY) to `stylesheets/map.ost`
- Add `aerialway_goods` type (WAY) to `stylesheets/map.ost`
- Add style definitions for the new types in `stylesheets/include/aerialway.oss` (way rendering, node symbols, labels)
- Register new way types in the aerialway `ORDER WAYS` groups of `standard.oss`, `winter-sports.oss`, `cycle.oss`, and `public-transport.oss`
- No existing types are removed or renamed; no breaking changes

## Capabilities

### New Capabilities
- `aerialway-type-definitions`: Import-time feature types for documented `aerialway=*` values missing from `map.ost`, with element types per the OSM wiki, plus corresponding style definitions where visualisation is obvious

### Modified Capabilities
<!-- None: no existing spec-level behavior changes -->

## Impact

- `stylesheets/map.ost` — new TYPE definitions in the Aerialway section
- `stylesheets/include/aerialway.oss` — new STYLE/SYMBOL definitions
- `stylesheets/standard.oss`, `stylesheets/winter-sports.oss`, `stylesheets/cycle.oss`, `stylesheets/public-transport.oss` — ORDER WAYS groups
- Database `TypeConfig` gains four new types; import pipeline picks them up automatically
- No C++ code, API, or dependency changes
