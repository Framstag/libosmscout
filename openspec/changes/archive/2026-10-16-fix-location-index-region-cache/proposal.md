# Proposal

## Why

Root cause: `LocationIndex` keeps no deserialized region entry — every traversal of the region tree of
`location.idx` deserializes each region it visits again through a pooled `FileScanner`
(`libosmscout/src/osmscout/db/LocationIndex.cpp:183` `LoadAdminRegion`, reached from every child and root
load), and `FlushCache()` only clears that scanner pool (`libosmscout/src/osmscout/db/LocationIndex.cpp:876`),
so the cost of a lookup follows the number of regions the index holds — the count §37 names as growing 9 -> 23
with the new `ADMIN_REGION` types.

Evidence:    `Tests/src/LocationIndexRegionCacheTest.cpp` case "A second traversal of the region tree reads no
region entry" fails without the cache — `REQUIRE( readsOfSecondTraversal==0 )`, expansion `12 == 0`, message
"second traversal: 12 regions visited, 12 region entries read"; `test cases: 12 | 11 passed | 1 failed`,
`assertions: 309 | 308 passed | 1 failed`, run at 2026-10-16T05:52:40Z; the run log is
`evidence/red-test.log` beside this proposal (archived as
`openspec/changes/archive/2026-10-16-fix-location-index-region-cache/`). The count is read through the
region-read diagnostic (`LocationIndex::GetRegionReadCount()`). On pristine HEAD that count has no host seam at
all — the diagnostic does not exist there — so the run measured `master` plus the diagnostic and no other
behaviour change, a revision that is not retained and cannot be reproduced by a hash now.
Repro:       `ctest -R LocationLookupTest --output-on-failure` from `build/`
Spec:        `location-index-lookup` (new capability) / "A lookup deserializes a region entry once while the
index retains it"   Guideline: `guidelines/FileFormatVersion.md` §"The content-versus-layout distinction"
(the retained regions are in-memory only, so no `FILE_FORMAT_VERSION` bump)

Every search of the location service walks the complete region tree of the index
(`libosmscout/src/osmscout/location/LocationService.cpp:2082`, `:2217`, `:2341`), so the regions one lookup
visits are the regions the index holds, and a second lookup of the same name pays for all of them again. The
same index is fed by `ADDRESS` (305 -> 880 types) and `POI` (5 -> 54 types), so the growth is not confined to
`ADMIN_REGION`. `LocationIndex::Load` reads only the header
(`libosmscout/src/osmscout/db/LocationIndex.cpp:30-96`), so this change does not touch database open.

## What Changes

- A lookup that deserializes a region entry retains it, so a second traversal of the same region tree reads
  no region entry from `location.idx` and reports the same regions, in the same order, as the traversal that
  read them.
- The index keeps the small memory footprint `FlushCache()` documents: a flush releases every retained
  region entry, and `Database::FlushCache()` reaches that release through the existing delegation, so no
  caller changes.
- No index file, no type definition and no database format changes: the retained data is in memory only, and
  the offsets, names, children and postal areas a caller sees are unchanged.
- `Database::GetLocationIndex()` hands the same index to every caller; the entries the fix retains and the
diagnostic are guarded by `regionCacheMutex` as a defensive guard, and the delta claims no concurrent-use
contract — no scenario and no test asserts concurrent lookups.

## Capabilities

### New Capabilities

- `location-index-lookup`: what a lookup on the location index costs relative to the region entries it
  visits, and that retaining those entries does not change what a traversal reports.

### Modified Capabilities

None. No existing capability states a cost or a result contract for the location index; the location search
capabilities (`location-search-api`, `poi-search-api`) describe what a search returns, not what reading the
index costs.

## Impact

Affected code:

- `libosmscout/include/osmscout/db/LocationIndex.h` — the retained region entries, the mutex that guards
  them, the region-read diagnostic and the cache-aware region load; the public signatures stay.
- `libosmscout/src/osmscout/db/LocationIndex.cpp` — `LoadAdminRegion` (the cache-aware overload),
  `VisitRegionEntries`, `VisitAdminRegions`, `VisitLocations`, `VisitRegionPOIs`, `VisitPostalArea`,
  `ResolveAdminRegionHierachie` (all region load sites) and `FlushCache`.
- `libosmscout/src/osmscout/db/Database.cpp` — read only: `FlushCache()` (`:975-977`) already delegates to
  `LocationIndex::FlushCache()`, and `GetLocationIndex()` (`:574-599`) already hands out one shared index.

Tests and build:

- `Tests/src/LocationIndexRegionCacheTest.cpp` — the new cases, added to the existing `LocationLookupTest`
  binary, which imports `Tests/LocationTest.olt` (1365 bytes) in process and opens the database on the host.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — the new source in the `LocationLookupTest` /
  `LocationServiceTest` target.

Records:

- `TODO.md` §37 — closed by this change: its metadata line was set to `**status:** fixed-by` with the
  change name, and the entry itself is archived with this change, removed from the backlog under the run's
  `--allow-todo-removal yes` allowance. The retained region entry a caller could mutate, which the entry's
  claim does not cover, is filed as the new entry §105.
- `TODO.md` §105 — new: a caller can write the retained region entry through the `AdminRegionRef`
  `ResolveAdminRegionHierachie` returns (design "Risk").

Not affected: the layout of `location.idx`, `GenLocationIndex` and the import pipeline, the scanner pool and
its reuse policy, the public API of `LocationIndex` and `LocationService`.
