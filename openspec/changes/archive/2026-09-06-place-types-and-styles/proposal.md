## Why

The place section of `stylesheets/map.ost` only defined 18 of the documented `place=*` values (continent, country, state, region, county, city tiers, town, village, hamlet, suburb, locality, island, islet, peninsula). The OSM wiki [Key:place](https://wiki.openstreetmap.org/wiki/Key:place) documents 33 values; 17 documented values with real-world usage were missing, so those features were not importable or renderable. Three documented values remain missing after the initial work.

## What Changes

- Add 17 new `TYPE` definitions in the place section of `stylesheets/map.ost` for documented `place=*` values that were missing: `province`, `district`, `subdistrict`, `municipality`, `isolated_dwelling`, `farm`, `allotments`, `borough`, `quarter`, `neighbourhood`, `city_block`, `plot`, `square`, `archipelago`, `polder`, `sea`, `ocean`. Element types follow the OSM wiki [Key:place](https://wiki.openstreetmap.org/wiki/Key:place) element table and the individual tag pages (e.g. `municipality`, `borough`, `quarter`, `neighbourhood`, `plot`, `isolated_dwelling` are node + area + relation; `archipelago` is area + relation; `ocean` is node-only; the rest are node + area).
- Add style definitions in `stylesheets/include/place.oss` for the new types: label priority constants and NODE.TEXT/AREA.TEXT rules at the appropriate magnification levels, following the existing patterns of the place style module.
- Add the 3 remaining documented values to `stylesheets/map.ost`: `cadastral_community` (16,636 objects per taginfo, status "in use", Czech Republic), `subcounty` (48 objects, status "in use", Kenya), `building_complex` (40 objects, experimental). Element types per the individual tag pages.
- No existing type or style definitions are removed or renamed.

## Capabilities

### New Capabilities
- `place-type-definitions`: Import-time feature types and rendering styles for documented `place=*` values missing from `stylesheets/map.ost`, following the pattern of the existing `man-made-type-definitions`, `landuse-type-definitions`, `barrier-type-definitions`, and `waterway-type-definitions` capabilities.

### Modified Capabilities
<!-- None: no existing spec-level behavior changes. -->

## Impact

- `stylesheets/map.ost` — place section: 20 new `TYPE` definitions (`place_province`, `place_district`, `place_subdistrict`, `place_municipality`, `place_isolated_dwelling`, `place_farm`, `place_allotments`, `place_borough`, `place_quarter`, `place_neighbourhood`, `place_city_block`, `place_plot`, `place_square`, `place_archipelago`, `place_polder`, `place_sea`, `place_ocean`, `place_cadastral_community`, `place_subcounty`, `place_building_complex`).
- `stylesheets/include/place.oss` — label priority constants and label rules for the new types.
- No C++ code changes: the `TypeConfig` is derived from the stylesheet at import time.
- Other stylesheets referencing place types (`standard.oss`, `cycle.oss`, `winter-sports.oss`) are unaffected but may benefit from the new types.
