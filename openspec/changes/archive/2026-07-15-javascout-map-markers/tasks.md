## 1. Java Client Library — Builder API

- [x] 1.1 Add `withCustomPoiType(String typeName)` to `OSMScoutClientBuilder.java` and store types in a list (spec: `custom-poi-types`)
- [x] 1.2 Pass the custom POI type list through JNI `build()` to C++ `DBThread` (spec: `custom-poi-types`)
- [x] 1.3 `OSMScoutClientBuilder.java` already present in native_headers and jar build (no change needed)

## 2. Java Client Library — Render API

- [x] 2.1 Add new native method `renderWithRouteAndPois(...)` to `OSMScoutClient.java` taking route arrays + favorite arrays + selected search lat/lon (spec: `favorite-markers`, `search-selection-marker`)
- [x] 2.2 Keep existing `renderWithRoute(...)` as a convenience overload calling the new method with empty favorite arrays and `NaN` selected coordinates
- [x] 2.3 Update `MapRenderer.java` to hold `double[] favoriteLats`, `double[] favoriteLons`, and `double searchSelectedLat/Lon`, with setters and a `requestRender...` variant that preserves route + markers

## 3. JNI — Custom POI Type Registration

- [x] 3.1 Read `customPoiTypes` list from `OSMScoutClientBuilder` in C++ `build()` JNI (spec: `custom-poi-types`)
- [x] 3.2 Pass the vector to `osmscout::DBThread` constructor instead of the current empty vector (spec: `custom-poi-types`)

## 4. JNI — Marker Rendering

- [x] 4.1 Implement `Java_com_framstag_libosmscout_client_OSMScoutClient_renderWithRouteAndPois()` in `OSMScoutClient.cpp` (spec: `favorite-markers`, `search-selection-marker`)
- [x] 4.2 Create `_favorite` nodes from favorite lat/lon arrays and add to `mapData.poiNodes`
- [x] 4.3 Create `_search_selected` node when selected lat/lon are not NaN and add to `mapData.poiNodes`
- [x] 4.4 Fix existing `_route_start` / `_route_end` nodes to actually render now that the types are registered (spec: `route-visualization`)
- [x] 4.5 Make `render()` JNI call `renderWithRouteAndPois()` with empty marker data

## 5. JavaScout — Integration

- [x] 5.1 Update `MapRenderer` to expose `setFavoriteLocations(...)`, `setSearchSelected(...)`, and `clearSearchSelected()` (spec: `favorite-markers`, `search-selection-marker`)
- [x] 5.2 Update `MainController` to call `renderer.setFavoriteLocations(...)` after loading favorites and after the favorites dialog saves changes
- [x] 5.3 Add a selection callback to `SearchOverlay` so `MainController` can update the search-selected marker when the user highlights a result
- [x] 5.4 Clear the search-selected marker when `SearchOverlay` collapses without navigating to a result; keep it when a result is navigated to
- [x] 5.5 Ensure all `requestRender...` paths in `MapRenderer` preserve route + markers, and marker setters trigger a re-render

## 6. Stylesheets

- [x] 6.1 Add `SYMBOL` definitions for favorite and search markers in `stylesheets/include/route.oss` (spec: `favorite-markers`, `search-selection-marker`)
- [x] 6.2 Add `[TYPE _route_start]`, `[TYPE _route_end]`, `[TYPE _favorite]`, `[TYPE _search_selected]` node style rules to `stylesheets/include/route.oss` with zoom-level filtering
- [x] 6.3 Verified that `standard.oss` and `winter-sports.oss` include `include/route`; `cycle.oss` does not include it, so markers will not appear there until it is added or an alternative stylesheet is chosen

## 7. Build / Test

- [x] 7.1 Build `libosmscout-client-java` C++ shared library with Meson (ninja target `libosmscout-client-java/src/libosmscout_client_java.so.1.1.1`)
- [x] 7.2 Build `libosmscoutclientjava.jar` with Meson and install to local Maven repository
- [x] 7.3 Build `JavaScout` with Maven
- [ ] 7.4 Run JavaScout with a map directory, add favorites, open search, select results, and verify markers appear at appropriate zoom levels (pending: no local map database available in this environment)

## 8. Specs

- [x] 8.1 Create delta specs under `openspec/changes/javascout-map-markers/specs/` for `custom-poi-types`, `favorite-markers`, and `search-selection-marker`
- [x] 8.2 Sync delta specs to main specs under `openspec/specs/<capability>/spec.md`
- [x] 8.3 Validate new specs with `openspec validate --specs`
