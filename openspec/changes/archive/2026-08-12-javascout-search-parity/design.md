## ctx

JavaScout (JavaFX) search currently calls `OSMScoutClient.searchLocations(query, limit)` → JNI → `SearchForLocationByString` only. OSMScout2 additionally uses free-text search (MARISA text index), default admin region, breaker, match-quality ranking, coordinate-first ranking, dedup, and basemap search. This change brings JavaScout to parity.

## Goals / Non-Goals

**Goals:**
- Free-text search over text index (POIs, locations, regions, other) in the JNI bridge
- Default admin region scoping
- Breaker-based cancellation
- Ranking: match-quality boost + coordinate-first + dedup
- Basemap free-text search
- All changes in `libosmscout-client-java` (JNI) and `JavaScout` (UI ranking)

**Non-Goals:**
- No core `libosmscout` changes — `TextSearchIndex`, `LocationService`, `Breaker` already exist
- No changes to OSMScout2
- No reverse geocoding, no voice search

## Decisions

### 1. JNI bridge extension (libosmscout-client-java)

`Java_com_framstag_libosmscout_client_OSMScoutClient_searchLocations` is extended:

- **Free-text search**: after structured search, load `TextSearchIndex` from the DB path and call `Search` with `searchPOIs/searchLocations/searchRegions/searchOther = true`, `transliterate = true`. Merge results, deduplicate by `ObjectFileRef`, truncate to limit. Missing text index → warn (non-basemap) / debug (basemap), continue.
- **Default admin region**: new optional parameter `defaultRegion` (admin region name or offset) passed to `LocationStringSearchParameter::SetDefaultAdminRegion`.
- **Breaker**: new optional parameter; passed to `SetBreaker`. Cancellation stops the search.
- **Basemap**: explicitly excluded from search — the basemap is a low-zoom background map; searching it adds no value (OSMScout2 searches it, but JavaScout deliberately does not).

New Java API:

```java
public native LocationEntry[] searchLocations(String query, int limit,
                                              String defaultRegion, boolean cancel);
```

`LocationEntry` gains fields needed for ranking/dedup: `matchQuality` (already present), `objectType` (already present), `distance` computed on Java side.

### 2. Ranking extension (JavaScout SearchOverlay)

`LocationEntryComparator` is extended to match OSMScout2's `locationRank`:

```java
rank = typeRank * distanceRank * matchRank
```

- `matchRank`: exact label match 1.0, prefix match 0.75, else 0.5
- coordinate entries: rank 1 (first)
- dedup after sorting: same objectType, < 300 m apart, > 3000 m from search center → keep first

### 3. UI wiring

- `SearchOverlay` passes current map center admin region (from map context) as `defaultRegion`
- Cancel button / new query triggers breaker
- Coordinate results (if produced) render first

### 4. Coordinate results

A query that parses as a lat/lon pair (e.g. `"51.5, 7.4"`, `"51.5 7.4"`) yields a
`LocationEntry` with `type="coordinate"` from the JNI bridge, ranked first by the
ranker — matching OSMScout2's coordinate-first behavior.

## Open Questions

- ~~How to derive the default admin region from the current map view in JavaScout~~ — resolved: new JNI `OSMScoutClient.getRegion(lat, lon)` uses `LocationDescriptionService::ReverseLookupRegion`; `SearchOverlay` calls it with the current map center before each search.
