## 1. JNI bridge: free-text search

- [x] 1.1 Add `#include <osmscout/db/TextSearchIndex.h>` to `libosmscout-client-java/src/OSMScoutClient.cpp`
- [x] 1.2 Implement free-text search in `searchLocations` JNI: load `TextSearchIndex` from DB path, call `Search` (POIs/locations/regions/other, transliterate), merge with structured results, deduplicate by `ObjectFileRef`, truncate to limit
- [x] 1.3 Graceful fallback: missing text index → warn (non-basemap) / debug (basemap), continue with structured results

## 2. JNI bridge: context region, breaker, basemap

- [x] 2.1 Add optional `defaultRegion` parameter to `searchLocations` JNI; call `SetDefaultAdminRegion` on the search parameter
- [x] 2.2 Add optional breaker parameter; call `SetBreaker`; stop early on cancellation
- [x] ~~2.3 Basemap free-text search~~ — removed: basemap is a low-zoom background map, searching it adds no value (per review)

## 3. Java API

- [x] 3.1 Extend `OSMScoutClient.searchLocations` signature with `defaultRegion` and `cancel` parameters
- [x] 3.2 Add `LocationEntry` fields needed for ranking/dedup if missing (matchQuality, objectType verified)

## 4. Ranking extension (JavaScout)

- [x] 4.1 Extend `LocationEntryComparator` with `matchRank` (exact 1.0, prefix 0.75, else 0.5)
- [x] 4.2 Coordinate entries rank first (rank 1)
- [x] 4.3 Add dedup after sorting: same objectType, < 300 m apart, > 3000 m from search center

## 5. UI wiring

- [x] 5.1 `SearchOverlay` passes current map region as `defaultRegion` — new JNI `getRegion(lat, lon)` (ReverseLookupRegion) + `SearchOverlay` wiring
- [x] 5.2 Cancel button / new query triggers breaker
- [x] 5.3 Coordinate results render first in the list
- [x] 5.4 JNI produces coordinate results: query parsed as lat/lon pair (e.g. "51.5, 7.4") → `LocationEntry` with `type="coordinate"`, ranked first

## 6. Tests

- [x] 6.1 Unit test for ranking comparator (exact > prefix > fuzzy, coordinate first, dedup)
- [x] 6.2 Verify build: `libosmscout-client-java` links against `osmscout` (TextSearchIndex) and `JavaScout/pom.xml` picks up changes
