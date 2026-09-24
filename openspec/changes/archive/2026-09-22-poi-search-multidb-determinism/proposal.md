# Proposal

## Why

More than one map database is routinely loaded at once: a regional extract together with the larger extract it lies in, or a map the user downloaded on top of an already installed one. The same OSM object then exists in several databases.

`searchPOIs` currently walks the loaded databases in the order they were discovered on disk, appends every database's hits to one list, and stops as soon as the result limit is reached. Two consequences for the caller:

- The same POI is returned several times, once per database that holds it.
- The result set depends on the filesystem scan order: a database that fills the limit first hides the hits of the databases that were not searched yet, and which database's copy of a duplicated POI survives is equally arbitrary.

Both are visible to the user as a list with repeated entries, and as hits that appear or disappear depending on which maps happen to be installed.

## What Changes

- A POI search over several loaded databases returns every POI at most once.
- When the same POI is contained in more than one loaded database, the returned copy is the one from the database whose area contains the search center.
- Databases whose area contains the search center are searched before the others, so the map the user is currently looking at takes precedence over background maps.
- A database that already produced a full page of results no longer prevents the remaining databases from being searched.
- The merged result keeps the nearest-first order; results at the same distance have a stable, reproducible order instead of an order that depends on the scan order.
- The requested limit applies to the merged, deduplicated list.

## Capabilities

### New Capabilities
<!-- none -->

### Modified Capabilities
- `poi-search-api`: adds the behaviour of `searchPOIs` when several databases are loaded — duplicates across databases collapse to one entry, the database containing the search center wins, and the merged list is deterministic and complete up to the limit.

## Impact

- `libosmscout-client-java/src/OSMScoutClient.cpp` — `searchPOIsByTypes`: the per-database walk and the merge, sort and truncation of the result list.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java` — multi-database scenarios; they need a directory of overlapping databases and are skipped when the corresponding test property is unset.
- No public Java API change: `PoiEntry`, the category set and the `searchPOIs` signature stay as they are.
- No change to the basemap exclusion, the category mapping, or the single-database behaviour.
