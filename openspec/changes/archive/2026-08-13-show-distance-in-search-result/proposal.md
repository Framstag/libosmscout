# Proposal: Show distance in search result

## What Changes

JavaScout's search result list currently shows three text lines per entry
(label/address, admin region hierarchy, object type). Users have no way to
judge how far a result is from the current map view without navigating to it.

This change adds a distance display to each search result entry: the
straight-line (haversine) distance from the current map center to the result
location, shown in kilometers, right-aligned in a smaller font.

The map center is already tracked by `SearchOverlay` (`mapCenterLat` /
`mapCenterLon`, updated via `setMapCenter()` and on navigation) and the
haversine distance helper already exists in `LocationSearchRanker.haversine()`
(returns meters). The change reuses both — no new geometry code, no client
library changes.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `location-search-ui`: search result entries SHALL display the distance from
  the map center to the result location, in kilometers, right-aligned in a
  smaller font than the entry's primary text.

## Impact

- `JavaScout/src/main/java/com/framstag/libosmscout/SearchOverlay.java` — result
  list cell factory: add a right-aligned distance label per entry, computed from
  `mapCenterLat`/`mapCenterLon` and the entry's `lat`/`lon`.
- `JavaScout/src/main/java/com/framstag/libosmscout/LocationSearchRanker.java` —
  `haversine()` reused as-is (public static, returns meters); no change expected.
- `JavaScout/src/main/resources/com/framstag/libosmscout/` — CSS style class for
  the distance label (smaller font, muted color), if styling is done via CSS
  rather than inline styles.
- No changes to `libosmscout-client-java` (`LocationEntry` already carries
  `lat`/`lon`), no changes to the native client library.
- Unit tests: `JavaScout/src/test/java/com/framstag/libosmscout/` — optional
  test for distance formatting (km formatting, rounding).
