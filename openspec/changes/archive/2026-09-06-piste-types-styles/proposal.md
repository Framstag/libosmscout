## Why

`map.ost` defines only 3 piste types (`piste_downhill_easy`, `piste_downhill_intermediate`, `piste_downhill_advanced`). The OSM wiki Pistes page documents 14 `piste:type` values (downhill, nordic, skitour, hike, sled, sleigh, ice_skate, snow_park, playground, ski_jump, fatbike, snowkite, snowmobile, halfpipe), and taginfo shows additional values with significant usage (`connection` 3052, `snowshoe` 428, plus semicolon combinations like `nordic;hike` 1102). All of these currently have no type or style definition, so they render as generic/unstyled objects or are dropped entirely.

## What Changes

- Add new type definitions to `map.ost` for the missing `piste:type` values documented on the OSM wiki Pistes page: `downhill` (with difficulty variants `novice`, `expert`, `freeride`, `extreme` and a generic fallback), `nordic`, `skitour`, `hike`, `sled`, `sleigh`, `ice_skate`, `snow_park`, `playground`, `ski_jump`, `fatbike`, `snowkite`, `snowmobile`, and `man_made=piste:halfpipe`
- Add taginfo-driven types for values with >=150 uses that are not discouraged: `connection` (approved status), `snowshoe`, and the semicolon combinations `nordic;hike`, `hike;fatbike`, `hike;snowshoe`, `nordic;hike;snowshoe`, `nordic;fatbike`
- Assign correct object types (NODE / WAY / AREA) per the OSM wiki for each new value
- Add corresponding style definitions in `include/piste.oss`, reusing existing type/style definitions for similar types (e.g. `piste_downhill_easy`) and following the color scheme of OpenSnowMap where visualization is obvious
- Extend the piste GROUP priority line in `winter-sports.oss` so the new types render at the same priority as the existing piste types
- Exclude `piste:type=yes` (undocumented/undefined, discouraged), values below 150 taginfo uses without wiki documentation (`snowkite` is included because it is documented on the wiki), and `piste:type=snowmobile` is included per the wiki despite 112 uses

## Capabilities

### New Capabilities
- `piste-type-definitions`: Type definitions in `map.ost` for all relevant `piste:type` values, with correct object types (NODE/WAY/AREA) and matching style definitions in `include/piste.oss`

### Modified Capabilities
<!-- No existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — new type definitions in the winter sports section (after `piste_downhill_advanced`, lines ~771–781)
- `stylesheets/include/piste.oss` — new style definitions and symbols for the added types
- `stylesheets/winter-sports.oss` — piste GROUP priority line extended with the new types
- No C++ code, API, or dependency changes; purely stylesheet data
- Rendering behavior: previously unstyled `piste:*` objects gain styles; no existing type or style is removed or renamed
