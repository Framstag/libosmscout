# Design

## The defect

`DoSearchLocations` (`libosmscout-client-java/src/OSMScoutClient.cpp`) collects two sources into
`results` (structured) and `freeTextEntries` (text index). The dedup has two places:

1. In the free-text loop, per database:

   ```
   if (seenOffsets[db].count(fref.GetFileOffset()) != 0) {
     continue;
   }
   seenOffsets[db].insert(fref.GetFileOffset());
   ```

   This collapses the same object reported under several matched names by one database.

2. After the walk, over the whole result set:

   ```
   for (const auto &result : results) { ... seenOffsets[result.db].insert(ref.GetFileOffset()); }
   freeTextEntries.erase(remove_if(..., [](const FreeTextEntry &e) {
       return seenOffsets[e.db].count(e.objectFileOffset) != 0; }), ...);
   ```

   This drops a free-text hit whose object a structured result already reported.

Both steps use `seenOffsets`. Since step 1 inserts every hit it accepts, step 2 finds each of those offsets
in the same set and erases the entry. The predicate is true for every entry by construction, so the free-text
part of the merge contributes nothing.

Measured 2026-10-27 on `maps/nordrhein-westfalen` (v27), query `Hauptstraße`, instrumented bridge:

```
search probe: resultsTxt=6 refs=3076 buildFail=0
search probe: beforeDedup=50
search probe: structured=0 freeText=0 scopeExtentSet=false query=Hauptstraße
```

`beforeDedup` is the page the loop built; `freeText=0` is what survives step 2.

## Fix

Give step 1 its own set:

```cpp
std::map<osmscout::DBInstanceRef, std::set<osmscout::FileOffset>> seenTextOffsets;
```

used in the free-text loop, while `seenOffsets` keeps only the structured offsets that step 2 compares
against. Both sets stay keyed by database, so the per-database identity rule is unchanged.

Why not the alternatives:

- **Drop step 1.** The text index reports one object under every matched name (the same street matched as
  `Hauptstraße` and `Hauptstrasse`), so the page would fill with duplicates of one object before the limit
  is reached, and step 2 would not remove them (they are not structured hits).
- **Drop step 2.** An object named by both the structured search and the index would be returned twice,
  which the merge requirement forbids.
- **Reorder the walk (structured first, then insert offsets, then free-text).** That makes the sets
  correct by accident and would still break when the structured pass is skipped; the two purposes stay
  clearer as two sets.
- **Compare on object identity instead of offsets.** `ObjectFileRef` equality across databases is exactly
  what the per-database keying avoids (the same offset in two databases is two objects).

## Effect on the search scope

`client-java-search-scope-extent` filters free-text hits by the extent of the resolved admin region. With
the merge defect in place that list was always empty, so neither the filter nor its "a hit outside the
extent is rejected" scenario could be observed. After this fix the filter runs over real hits; its own
verification is that change's task 4.4, not this one. This change does not touch the filter.

## Test strategy

The merge lives inside a JNI function that needs an open database, so the regression is a JavaScout test
rather than a host test:

- **A hit only the index reports is returned.** A bare street name (`Hauptstraße`) resolves to no
  structured result in an unscoped search on that database but is in the text index; after the fix the
  result list is non-empty and its entries report `matchedComponent = "freeText"`.
- **A hit found by both searches appears once.** The same query is also searched with a default admin
  region, where the structured search resolves the streets of the region too; no label/position pair may
  appear twice.

Both cases need a map database and skip without `-Dsearch.test.db.dir`, the convention `SearchReproTest`
and `OSMScoutClientPoiSearchTest` already use. Before the fix the first case fails, which is the test's
teeth; the evidence is recorded in `verification.md`.
