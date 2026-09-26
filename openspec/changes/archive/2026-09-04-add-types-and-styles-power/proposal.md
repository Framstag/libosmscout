## Why

`map.ost` defines only 6 of the 32 `power` values documented on the OSM wiki (Key:power). Values like `catenary_mast`, `portal`, `transformer`, `switch`, and `plant` each exceed 0.01% of all `power`-tagged objects on taginfo yet have no type or style definition, so they render as generic/unstyled objects or are dropped entirely.

## What Changes

- Add new type definitions to `map.ost` for the missing `power` values documented in the OSM wiki with relevant usage (>0.01% on taginfo) and not discouraged: `catenary_mast`, `portal`, `transformer`, `cable`, `switch`, `plant`, `terminal`, `heliostat`, `insulator`, `circuit`, `connection`, `catenary_portal`, `compensator`, `inverter`, `switchgear`, `converter`
- Assign correct object types (NODE / WAY / AREA) per the OSM wiki for each new value
- Add corresponding style definitions in `include/power.oss`, reusing existing type/style definitions for similar types (e.g. `power_tower`, `power_pole`, `power_line`, `power_sub_station`, `power_generator`) where visualization is obvious
- Exclude `cable_distribution_cabinet` (discouraged on the wiki) and `cable_distribution` (not documented on the wiki)
- Resolve the existing TODO comment for `power_plant` in `map.ost`

## Capabilities

### New Capabilities
- `power-type-definitions`: Type definitions in `map.ost` for all relevant `power` values, with correct object types (NODE/WAY/AREA) and matching style definitions in `include/power.oss`

### Modified Capabilities
<!-- No existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — new type definitions in the power section (lines ~6647–6672)
- `stylesheets/include/power.oss` — new style definitions for the added types
- `stylesheets/basemap.ost` — only if a power type must also be visible at basemap zoom levels (to be determined during design)
- No C++ code, API, or dependency changes; purely stylesheet data
- Rendering behavior: previously unstyled `power` objects gain styles; no existing type/style is removed or renamed
