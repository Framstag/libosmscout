## 1. Distance display implementation

- [x] 1.1 Add a distance formatting helper (km value with one decimal place below 10 km, "km" unit suffix) — e.g. static method in `LocationSearchRanker` or private method in `SearchOverlay` (spec: location-search-ui / Result distance display)
- [x] 1.2 Extend the result list cell factory in `SearchOverlay.java` to compute the haversine distance from `mapCenterLat`/`mapCenterLon` to each entry's `lat`/`lon` using `LocationSearchRanker.haversine()` (spec: location-search-ui / Result distance display)
- [x] 1.3 Render the distance as a right-aligned label in a smaller font than the entry's primary text (new style class, e.g. `search-result-distance`, wired via CSS or inline style) (spec: location-search-ui / Result distance display)
- [x] 1.4 Verify distances recompute against the current map center on each search (map center already tracked via `setMapCenter()` and navigation) (spec: location-search-ui / Result distance display)

## 2. Tests

- [x] 2.1 Add unit test for distance formatting: sub-kilometer precision, km unit suffix, rounding (JavaScout/src/test/java/com/framstag/libosmscout/)
- [x] 2.2 Add unit test verifying distance computation matches expected haversine values for known coordinate pairs

## 3. Verification

- [x] 3.1 Build JavaScout with `mvn package` and confirm no compile errors
- [x] 3.2 Run existing JavaScout tests (`mvn test -Dnative.lib.dir=...`) and confirm all pass
