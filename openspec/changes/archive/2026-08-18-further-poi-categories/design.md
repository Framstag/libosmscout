## Context

See proposal.md — Why. Current state: `PoiCategories` (libosmscout-client-java) hardcodes 3 categories → OSM type names; JavaScout `PoiSearchOverlay` lists them; JNI `searchPOIsByTypes` resolves names via `typeConfig->GetTypeInfo()` and skips unknown types with a warning (empty result, no error). Type definitions come from the stylesheet used at import time (`stylesheets/map.ost`); `amenity_police` and `amenity_doctors` do not exist there today.

## Goals / Non-Goals

**Goals:**
- Add 11 new categories to the shared hardcoded mapping, each with a curated set of existing OSM types.
- Add the two missing type definitions (`amenity_police`, `amenity_doctors`) to the import stylesheet.
- Keep the search engine, JNI bridge, result model, and UI flow untouched.

**Non-Goals:**
- User-configurable category mappings (still hardcoded; deferred).
- Category hierarchies or icons.
- Changes to the basemap stylesheet or basemap search behavior.
- Re-import of shipped/example databases.

## Decisions

### D1: Extend `PoiCategories`, keep single source of truth
All new category ids and type arrays go into `PoiCategories.CATEGORY_TYPES`; category id constants added alongside `HOTELS`/`RESTAURANTS`/`GROCERY`. The JavaScout overlay builds its combo from `PoiCategories.getCategoryTypes()` (as today), so the UI list and API mapping cannot drift. Verified: `PoiSearchOverlayTest` asserts 3 entries; it must be updated to 14.

### D2: Type sets per category

- `viewpoint` → `tourism_viewpoint`
- `museum` → `tourism_museum`, `tourism_museum_building`
- `fuel` → `amenity_fuel`, `amenity_fuel_building`
- `charging_station` → `amenity_charging_station` (`amenity_ev_charging` exists but is marked `IGNORE` in map.ost → not stored in the DB, not searchable)
- `atm` → `amenity_atm`
- `tourism` → umbrella: `tourism_attraction`, `tourism_attraction_building`, `tourism_artwork`, `tourism_aquarium`, `tourism_zoo`, `tourism_theme_park`, `tourism_picnic_site`, `tourism_camp_site`, `tourism_caravan_site`, `tourism_viewpoint`, `tourism_museum`, `tourism_information`, `tourism_alpine_hut`, `tourism_chalet`
- `parking` → `amenity_parking`, `amenity_bicycle_parking` (`amenity_parking_entrance`/`amenity_parking_space` are marked `IGNORE` in map.ost → not searchable)
- `police` → `amenity_police` (new)
- `hospital` → `amenity_hospital`, `amenity_hospital_building`
- `doctors` → `amenity_doctors` (new)
- `public_transport` → `railway_station`, `railway_halt`, `railway_tram_stop`, `amenity_bus_station`, `public_transport_platform`, `railway_subway_entrance`

Rationale: every name checked against `stylesheets/map.ost` TYPE entries, and IGNORE-marked types excluded (TypeConfig assigns no node/way/area ids to IGNORE types, so they never appear in the database). The `tourism` umbrella deliberately excludes hotels/lodging/guest houses (those live in `hotels` category, avoiding duplicates) but includes information+alpine_hut+chalet as tourist POIs. Parking excludes `amenity_parking_building` (rare, building-only) — acceptable; can add later.

Alternative considered: split public transport by mode. Rejected — one category matches user request; type set covers rail + bus + platform.

### D3: New type definitions follow existing map.ost patterns

Add to `stylesheets/map.ost`, mirroring `amenity_hospital` style:

```
TYPE amenity_police
  = NODE AREA ("amenity"=="police")
```

`amenity_doctors` analog with `"amenity"=="doctors"`. Node-only definitions (`= NODE`) suffice for POI search but would exclude area-tagged police stations; use `NODE AREA` to match hospital/restaurant convention. Alternative: add building variants like `amenity_hospital_building` — rejected, not needed for search.

### D4: No code change for missing types — degrade gracefully

Existing JNI behavior (warn + skip unknown type names) already yields empty results on databases without `amenity_police`/`amenity_doctors`; no new error paths needed. Verified in `OSMScoutClient.cpp` (`searchPOIsByTypes`).

## Risks / Trade-offs

- **Old databases can't return police/doctors POIs** → empty results with a warning logged; documented in proposal. Mitigation: re-import or upgrade note; no crash.
- **Type name typos silently reduce results** → all names verified against `map.ost` during this change; add a unit test asserting every mapped type resolves via a test `TypeConfig`? — not possible without import; instead keep names greppable and verified.
- **Duplicate POIs across umbrella categories** (`tourism_viewpoint` in both `viewpoint` and `tourism`) → accepted; per-search category filter means no cross-category contamination.
- **UI list length (14 entries)** → combo box still fine; no overflow.

## Migration Plan

1. Code change (`PoiCategories` + overlay test) ships independently of data.
2. Stylesheet change requires **re-import** of any database that should support `police`/`doctors` searches. Old databases keep working for the other 12 categories.
3. No rollback complexity: revert = remove entries + type definitions.

## Open Questions

None.
