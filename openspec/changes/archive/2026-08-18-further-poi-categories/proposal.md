## Why

The first POI search iteration (`add-poi-search`) supports only three categories: Hotels, Restaurants, and Grocery store. Users of JavaScout frequently need more everyday categories — viewpoints, museums, fuel/charging stations, ATMs, parking, hospitals, public transport, and more. Extending the fixed category list covers the most common nearby-POI lookups without yet introducing user-configurable mappings.

## What Changes

- Extend the hardcoded category → OSM type mapping (`PoiCategories`) with eleven new categories:
  - `viewpoint` → `tourism_viewpoint`
  - `museum` → `tourism_museum`, `tourism_museum_building`
  - `fuel` (gas station) → `amenity_fuel`, `amenity_fuel_building`
  - `charging_station` → `amenity_charging_station`
  - `atm` → `amenity_atm`
  - `tourism` (general) → the umbrella of existing `tourism_*` feature types (attraction, artwork, aquarium, zoo, theme park, picnic site, camp site, viewpoint, museum, information, alpine hut, chalet, ...)
  - `parking` → `amenity_parking`, `amenity_bicycle_parking`
  - `police` → `amenity_police` (**new type**)
  - `hospital` → `amenity_hospital`, `amenity_hospital_building`
  - `doctors` → `amenity_doctors` (**new type**)
  - `public_transport` → `railway_station`, `railway_halt`, `railway_tram_stop`, `amenity_bus_station`, `public_transport_platform`, `railway_subway_entrance`
- Add two new type definitions to `stylesheets/map.ost`:
  - `amenity_police` (`"amenity"=="police"`)
  - `amenity_doctors` (`"amenity"=="doctors"`)
- Update the JavaScout POI search overlay category list to show the new categories.
- No change to the search engine, result data model, slider, or details dialog — they are category-agnostic already.

## Capabilities

### New Capabilities
- `poi-type-definitions`: New OSM feature type definitions (`amenity_police`, `amenity_doctors`) in the import-time stylesheet, needed so police stations and doctors offices exist in the database `TypeConfig` and can be searched.

### Modified Capabilities
- `poi-search-api`: The predefined category set is extended from 3 to 14 categories; each new category maps to a fixed set of OSM feature types.
- `poi-search-ui`: The category selection list SHALL offer the new categories; the search and result flow is unchanged.

## Impact

- `libosmscout-client-java/java/com/framstag/libosmscout/client/PoiCategories.java` — extend `CATEGORY_TYPES` map with the new categories; add category id constants.
- `JavaScout/src/main/java/com/framstag/libosmscout/PoiSearchOverlay.java` — category combo list gains new entries (driven by `PoiCategories`).
- `JavaScout/src/test/java/com/framstag/libosmscout/PoiSearchOverlayTest.java` — category list test updated for 14 entries.
- `stylesheets/map.ost` — two new type definitions (`amenity_police`, `amenity_doctors`).
- **Database re-import**: type definitions come from the stylesheet used at import time. Databases imported before this change lack `amenity_police`/`amenity_doctors`, so searches for `police`/`doctors` return empty results on old data; re-import is required to make them searchable.
- No changes to `POIService`, JNI bridge, or the `PoiEntry` result type.
