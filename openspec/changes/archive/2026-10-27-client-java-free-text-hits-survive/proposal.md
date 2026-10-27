# Proposal

## Why

The Java client's location search merges structured results with the hits of the text index. The merge is
specified to deduplicate by object reference, and the bridge implements that in two steps: the free-text
pass remembers the object offsets it has already pushed, and after the structured pass it drops every
free-text hit whose offset a structured result also reported.

The two steps share one bookkeeping set (`seenOffsets[db]` in
`libosmscout-client-java/src/OSMScoutClient.cpp`): the free-text loop inserts each hit's offset into it, and
the dedup pass that follows compares the free-text entries against that very set. Every free-text hit
therefore matches its own entry and is removed, so **no free-text hit ever reaches a caller** — for any
query, any database and any scope.

Measured on 2026-10-27 against the v27 database `maps/nordrhein-westfalen` with a temporarily instrumented
bridge (log lines reverted afterwards): the text index yields candidates (`resultsTxt=6`, `refs=3076` for
"Haupstraße"), the loop builds the page (`beforeDedup=50`), and the reported list holds `freeText=0` in
every call, scoped and unscoped alike.

Consequences:

- The `search-free-text` requirement "Free-text results merged with structured results" is not satisfied:
  the text index contributes nothing, so a POI, region or other object that only the index can name is
  never found.
- The free-text filter of `client-java-search-scope-extent` (the geographic extent of the resolved admin
  region) is applied to an empty list, so its spec scenario cannot be observed at all. That change's task
  4.4 is blocked by this defect.

## What Changes

- The free-text pass SHALL keep its own per-database offset set, so it collapses duplicates the index
  reports for the same object without touching the set the structured-dedup pass reads.
- The structured-dedup pass SHALL keep comparing free-text hits against the offsets the structured results
  reported, so an object found by both searches is still returned once.
- A free-text hit that no structured result reports SHALL be delivered to the caller, after the extent
  filter of the active search scope has been applied to it.
- The bookkeeping SHALL stay per database: the same raw file offset in two databases denotes two different
  objects, as it does today.

## Capabilities

### Modified Capabilities

- `search-free-text`: the merge requirement gains the case that a text-index hit which no structured result
  reports is part of the result list, next to the existing once-only case.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — `DoSearchLocations`: the free-text loop's dedup set
  becomes its own map, the structured-dedup pass keeps reading the structured offsets.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientFreeTextResultTest.java` — new
  test: a text-index-only hit is returned, and an object found by both searches appears once. Both cases
  need a map database and skip without `-Dsearch.test.db.dir` (the convention of
  `OSMScoutClientPoiSearchTest` and `SearchReproTest`).

No database, map, style or file-format change, so no `FileFormatVersion.md` bump applies. No JNI signature,
Java declaration or native export changes: the fix is inside one existing function. No new dependency.

Unblocks `client-java-search-scope-extent` task 4.4 (its free-text half becomes observable); the extent
filter itself is not touched by this change.
