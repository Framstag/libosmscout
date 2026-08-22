## Why

`stylesheets/map.ost` defines specific types for most documented `tourism=*` values (hotel, museum, camp_site, …), but several documented and widely used values are missing. Objects carrying these tags are only imported as the generic `tourism` catch-all type, so they lack specific feature extraction (opening hours, phone, website), POI/routingPOI classification, and area rendering.

Missing values with relevant usage per taginfo (>=50, documented in OSM wiki, not discouraged):
- `tourism=camp_pitch` (≈139k)
- `tourism=apartment` (≈90k)
- `tourism=gallery` (≈22k)
- `tourism=wilderness_hut` (≈17k)
- `tourism=trail_riding_station` (≈1.6k)

Discouraged/deprecated values (`tourism=resort`, `tourism=winery`) and undocumented values (`checkpoint`, `cabin`, `wine_cellar`, `hunting_lodge`, `lean_to`, `spa_resort`, `holiday_village`, …) SHALL NOT be added.

## What Changes

- Add import-time types to `stylesheets/map.ost` following the existing tourism conventions (feature set, ADDRESS/POI, GROUP incl. `routingPOI`, `_building` variants for building-like types):
  - `tourism_apartment` (node, area) + `tourism_apartment_building` (area)
  - `tourism_gallery` (node, area) + `tourism_gallery_building` (area)
  - `tourism_wilderness_hut` (node, area) + `tourism_wilderness_hut_building` (area)
  - `tourism_camp_pitch` (node, area)
  - `tourism_trail_riding_station` (node, area)
- Add style rules to `stylesheets/include/tourism.oss` so areas of the new types get the standard tourism fill (matching museum/camp_site rendering); `_building` variants are covered by the existing `GROUP tourism, building` rules.
- No change to existing types: wiki element table confirms current definitions (e.g. `tourism_viewpoint` is node-only, `tourism_artwork` node/way/area).
- No file-format change: existing databases can be re-imported to pick up the new types.

## Capabilities

### New Capabilities
- `tourism-type-definitions`: import-time types for the missing documented `tourism=*` values and their rendering.

### Modified Capabilities
<!-- none: existing specs are unrelated -->

## Impact

- `stylesheets/map.ost` — 5 new types + 3 `_building` variants
- `stylesheets/include/tourism.oss` — extend `[MAG detail-]` area fill list
- Imported databases: new types appear in `TypeConfig` after re-import; POI search and routing POI search gain the new categories
- Renderers: areas of new types get tourism fill at detail zoom; nodes get the generic tourism icon via the existing `GROUP tourism` rules
