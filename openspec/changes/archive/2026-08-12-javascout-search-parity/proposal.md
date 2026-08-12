## What Changes

Close the search capability gap between JavaScout and OSMScout2. JavaScout currently only performs structured location search (`SearchForLocationByString`) without free-text search, context scoping, cancellation, or the ranking refinements OSMScout2 applies. This change brings JavaScout search to parity with OSMScout2 across seven dimensions.

## Capabilities

### New Capabilities

- `search-free-text`: Free-text search over POIs, locations, regions and other objects via the text search index (MARISA), matching OSMScout2's `TextSearchIndex` usage. Falls back gracefully when the text index is absent.
- `search-context-region`: Search scoped to the current map region via a default admin region, improving relevance and latency.
- `search-cancellation`: Long-running searches can be cancelled via a breaker, matching OSMScout2's `Breaker` support.
- `search-ranking-match`: Ranking includes match quality boost (exact label match and prefix match rank higher than fuzzy candidates), matching OSMScout2's `matchRank`.
- `search-ranking-coordinate`: Coordinate/GPS results rank first, matching OSMScout2.
- `search-dedup`: Near-identical results (same type, close together, far from search center) are deduplicated, matching OSMScout2.

### Modified Capabilities

- `location-search-api`: Extended with free-text search, default admin region, and breaker.
- `location-search-ui`: Result ranking extended with match-quality boost and deduplication.

## Impact

- **libosmscout-client-java**: JNI bridge `OSMScoutClient.searchLocations` extended — free-text search, default admin region, breaker; `LocationEntry` gains fields needed for ranking/dedup.
- **JavaScout UI**: `SearchOverlay` ranking comparator extended with match-rank and dedup; coordinate results handling.
- **libosmscout**: No core changes — `TextSearchIndex`, `LocationService`, `Breaker` already exist and are reused.
- **Basemap**: explicitly excluded from search — the basemap is a low-zoom background map and searching it adds no value.
