## 1. POI Category Mapping (poi-search-api)

- [x] 1.1 Add category id constants (`VIEWPOINT`, `MUSEUM`, `FUEL`, `CHARGING_STATION`, `ATM`, `TOURISM`, `PARKING`, `POLICE`, `HOSPITAL`, `DOCTORS`, `PUBLIC_TRANSPORT`) to `PoiCategories.java`
- [x] 1.2 Extend `CATEGORY_TYPES` map with the 11 new categories per design D2 (type names verified against `stylesheets/map.ost`)
- [x] 1.3 Verify `getTypeNames` still returns a defensive clone and `getCategoryTypes` stays unmodifiable

## 2. Stylesheet Type Definitions (`poi-type-definitions`)

- [x] 2.1 Add `TYPE amenity_police = NODE AREA ("amenity"=="police")` to `stylesheets/map.ost`
- [x] 2.2 Add `TYPE amenity_doctors = NODE AREA ("amenity"=="doctors")` to `stylesheets/map.ost`
- [x] 2.3 Grep the stylesheet set to confirm no duplicate/conflicting `amenity_police`/`amenity_doctors` type already exists

## 3. JavaScout UI (poi-search-ui)

- [x] 3.1 Confirm `PoiSearchOverlay` category combo is driven by `PoiCategories.getCategoryTypes()` and contains no separate hardcoded 3-category list
- [x] 3.2 If the combo uses display labels, map the 11 new category ids to readable labels (e.g. "Gas station", "Doctors office", "Public transport")
- [x] 3.3 Update `PoiSearchOverlayTest` — assert 14 categories in the combo, first entry still preselected

## 4. Tests & Validation

- [x] 4.1 Add unit test asserting every category in `PoiCategories` maps to at least one non-empty type name and ids are unique
- [x] 4.2 Confirm unchanged search behavior: unknown type names / missing types in the DB still produce an empty result without error (existing JNI path)
- [x] 4.3 Run JavaScout client + UI tests (`mvn test`) and fix failures
- [x] 4.4 Run `openspec validate further-poi-categories --type change` and confirm it passes
