## 1. Client API — POI search (spec: poi-search-api)

- [x] 1.1 Add `PoiEntry` data class in `libosmscout-client-java/java/com/framstag/libosmscout/client/` with fields `label`, `objectType`, `lat`, `lon`, `distance` (spec: POI result data class)
- [x] 1.2 Add native method declaration `searchPOIsByTypes(String[] typeNames, double lat, double lon, double radiusMeters, int limit)` to `OSMScoutClient` (spec: POI search API on OSMScoutClient)
- [x] 1.3 Add Java wrapper `searchPOIs(String category, double lat, double lon, double radiusMeters, int limit)` on `OSMScoutClient` that resolves the category to type names and delegates to the native method (spec: POI search API on OSMScoutClient, design D1)
- [x] 1.4 Add hardcoded category→type-name map (hotels, restaurants, grocery) in client-java (spec: Predefined POI categories, design D1/D2)
- [x] 1.5 Implement JNI `Java_com_framstag_libosmscout_client_OSMScoutClient_searchPOIsByTypes` in `libosmscout-client-java/src/OSMScoutClient.cpp`: run synchronous job over databases, skip basemap, resolve type names via `TypeConfig::GetTypeInfo` into `TypeInfoSet` (node/way/area), call `POIService::GetPOIsInRadius`, convert results to `PoiEntry[]` (spec: JNI bridge for POI search, design D3)
- [x] 1.6 Return empty array for unknown category, zero radius, or uninitialized client without error (spec: POI search API on OSMScoutClient scenarios)
- [x] 1.7 Reuse existing breaker infrastructure so a new search cancels a running one (design D7)

## 2. JavaScout UI — POI search (spec: poi-search-ui, javascout-main-menu)

- [x] 2.1 Add "Search POIs…" item to the main menu in `MainController.createMainMenuButton` (spec: javascout-main-menu — POI search menu item)
- [x] 2.2 Create `PoiSearchOverlay` with category list (Hotels, Restaurants, Grocery store) and stepped radius slider (spec: POI category selection, Search area slider)
- [x] 2.3 Wire menu item to open `PoiSearchOverlay` and close the menu (spec: POI search opens from main menu)
- [x] 2.4 Implement search execution: read selected category + slider radius, search around current map center via `OSMScoutClient.searchPOIs` on a daemon-thread `Task`, marshal results via `Platform.runLater` (spec: POI search execution, design D7)
- [x] 2.5 Build result list similar to `SearchOverlay` result list: label + type per entry, sorted by distance ascending (spec: POI result list)
- [x] 2.6 Click on result pans/centers map to POI coordinates (spec: POI result list — Result click navigates map)
- [x] 2.7 Long-click on result: press-and-hold timer (500ms default) triggers `getDescription(lat, lon)` and shows `DescriptionOverlay`; suppress click on release after long-press (spec: Long-click on result shows details, design D6)
- [x] 2.8 Show "No description available" in details dialog when POI has no description data (spec: Long-click on result without description data)
- [x] 2.9 Add overlay styling to `style.css` / layout wiring in `MainController` (spec: poi-search-ui)

## 3. Tests

- [x] 3.1 Unit test category→type-name mapping for all three categories (hotels, restaurants, grocery) in client-java (spec: Predefined POI categories)
- [x] 3.2 Unit test `PoiEntry` field population (spec: POI result data class)
- [x] 3.3 Unit test `searchPOIs` wrapper: unknown category → empty, zero radius → empty (spec: POI search API on OSMScoutClient)
- [x] 3.4 Native test (JNI, requires `-Dnative.lib.dir`): `searchPOIs` on open database returns POIs of the category within radius; uninitialized client returns empty (spec: POI search API on OSMScoutClient)
- [x] 3.5 JavaScout UI test: menu shows "Search POIs…" item; overlay shows three categories and slider (spec: poi-search-ui, javascout-main-menu)

## 4. Build & Regression Verification

- [x] 4.1 Build native client (`libosmscout-client-java`) via Meson without errors
- [x] 4.2 Build JavaScout via `mvn package` without errors
- [x] 4.3 Run existing JavaScout unit tests (`mvn test`) — all pass
- [x] 4.4 Run existing native tests (`mvn test -Dnative.lib.dir=...`) — all pass (one pre-existing failure in `OSMScoutClientNavigationLiveTest.testBicycleRouteCalculation`, reproduced without this change)
- [ ] 4.5 Manual smoke test: open map, menu → Search POIs…, pick category, adjust slider, verify results, click + long-click behavior
