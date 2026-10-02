# Verification

Change: `improve-area-index-offset-lookup`. Scope: `libosmscout/src/osmscout/db/AreaIndex.cpp`,
`libosmscout/include/osmscout/db/AreaIndex.h`, `Tests/src/AreaIndexLookupTest.cpp`,
`Tests/CMakeLists.txt`, `Tests/meson.build`.

## What the lookup costs now

The examined-entry count of a request against the committed test database
(`Tests/data/testregion`), read through the new `AreaIndex::GetExaminedEntryCount()`:

| index | entries the index carries | request | examined before | examined after |
|---|---|---|---|---|
| `areaway.idx` (`AreaWayIndex`) | 18 | 2 carried types | 18 | 2 |
| `arearoute.idx` (`AreaRouteIndex`) | 4 | 2 carried types | 4 | 2 |
| `areaway.idx` | 18 | 1 type the index does not carry | 18 | 0 |

The "after" column is asserted by `AreaIndexLookupTest`
(`REQUIRE(lookup.examinedEntries==Types(request).size())`, `REQUIRE(one.examinedEntries==1)`,
`REQUIRE(two.examinedEntries==2)`, `REQUIRE(none.examinedEntries==0)`). The "before" column is the
entry count of the index: the lookup before the change examined every entry (`for (const auto& data :
typeData)`), and the count of entries comes from `AreaIndex::GetEntryCount()` in the same test run.

The committed fixture is small (18 and 4 entries), so the win here is the independence of the request
from the entry count, not a large absolute saving; the type set the parked branch carries would make
the same request examine 1097 entries before the change and the requested handful after it.

## Result unchanged

The result of a request is pinned by golden assertions, taken from the lookup before the rewrite and
unchanged after it:

- `AreaWayIndex`, 1 km box at 50.47254/14.53186 in the test region, two carried types →
  offsets `{38105, 39348}`; the same request over the whole region returns a superset.
- `AreaRouteIndex`, whole test region, two carried types →
  offsets `{4, 114, 231, 1130, 1359, 1407, 1793, 1838, 1991, 2381, 2977, 4090, 4181, 4392}`.
- The reported loaded types equal the requested types, and a requested type whose entry resolves no
  offset (a box outside the region) is still reported as loaded.

Order is not asserted: the lookup deduplicates through an `unordered_set`, so the returned order is
that container's and never was the request's or the file's. Both assertions sort first.

## Test discrimination (skill: `verify-test-discriminates`)

Behaviour under test, in one sentence: with the request-driven lookup the code examines only the
entries of the requested types; before the change it examined every entry the index carries.

| probe (one relaxation each, marker `TEMP-REVERT-PROBE`, all restored) | outcome |
|---|---|
| the pre-change loop over `typeData`, counting every entry as examined | 3 cost cases failed: `REQUIRE( lookup.examinedEntries==Types(request).size() )` (`AreaIndexLookupTest.cpp:298`), `REQUIRE( one.examinedEntries==1 )` (`:335`), `REQUIRE( lookup.examinedEntries==1 )` (`:364`); the 2 result cases passed | 
| `loadedTypes` set only when the entry's box intersects the queried box | 2 cases failed: `REQUIRE( box.loadedTypes==Types(request) )` (`:239`), `REQUIRE( lookup.loadedTypes==Types(request) )` (`:366`) |
| resuming the read after the first resolved entry (`break`) | 4 cases failed, the loaded-types assertions first (`:238`, `:268`), then the cost ones (`:298`, `:336`) |
| resolving against the entry's own box instead of the requested box | `REQUIRE( box.offsets==std::vector<osmscout::FileOffset>{38105,39348} )` (`:249`) and `REQUIRE( lookup.offsets.empty() )` (`:365`) failed |

Per probe: rebuilt the target, ran `TESTS_TOP_DIR=$(pwd)/Tests ./build/Tests/AreaIndexLookupTest`,
restored the code, rebuilt, re-ran → `All tests passed (560 assertions in 5 test cases)`. The marker
grep over the tree excluding build directories is clean.

The two result cases pass under the first probe by design: they pin the result the pre-change lookup
produced, so they cannot fail against it. Their teeth are shown by the fourth probe (a wrong box moves
the offsets) and the second one (a wrong loaded-types rule).

## Suites

| command | result |
|---|---|
| `cmake --build build --target AreaIndexLookupTest` | builds, no warning in the touched files |
| `meson compile -C build-meson` (full tree, 316 targets) | builds, no warning in the touched files |
| `cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=…/Tests TESTS_TMP_DIR=…/build/Tests ctest -j 4 --output-on-failure` | 141/141 passed (47.8 s) |
| `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` | 141/141 OK |
| `ctest -R ThreadedDatabaseTest` (100 threads x 1000 iterations over this database) | passed (26.8 s) |

`ThreadedDatabaseTest` is the concurrency check for the new atomic diagnostic counter and for the
entry table built at `Open()`.

## Conventions

- `uncrustify -q -c .uncrustify -l CPP -f <file>` differences: `AreaIndex.cpp` 4 lines,
  `AreaIndex.h` 14 lines — both exactly the count the same files show at `HEAD`, so the change adds no
  formatter drift. `Tests/src/AreaIndexLookupTest.cpp` keeps the Tests tree's file-local style
  (namespace content at column 0, as in `StyleLoadResilienceTest.cpp`); its drift modulo leading
  whitespace is alignment inside declaration blocks, the same class as its neighbours. The
  repo-wide uncrustify baseline is a separate, open decision recorded in `TODO.md`.
- `clang-tidy -p build` on the touched files: no new finding class. Remaining findings are the
  classes the tree already carries — `misc-include-cleaner` (the file leans on transitive includes on
  lines that pre-date this change), `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`
  on the new `entryOfType[...]`/`typeData[...]` accesses (the same class as
  `libosmscout/include/osmscout/GeoCoord.h:161`, and `guidelines/CodeStyles.md` does not require
  bounds-checked access), and `concurrency-mt-unsafe` on `std::getenv`, which every test that reads
  `TESTS_TOP_DIR` uses. Findings that this change did introduce (a C-style cast, an unused include, a
  magic number for the fixture box, `std::includes`/`std::sort` instead of the ranges forms, a missing
  trailing comma) were fixed.

## Files and layout

- Changed: `libosmscout/include/osmscout/db/AreaIndex.h` (entry table, `kNoEntry`, examined-entry
  counter, `GetEntryCount()`, `GetExaminedEntryCount()`), `libosmscout/src/osmscout/db/AreaIndex.cpp`
  (table built in `Open()`, table cleared in `Close()`, request-driven `GetOffsets`),
  `Tests/src/AreaIndexLookupTest.cpp` (new), `Tests/CMakeLists.txt`, `Tests/meson.build`.
- No database, index file, importer, style sheet or generated file is touched
  (`git diff --stat` lists only the five files above), so `guidelines/FileFormatVersion.md` does not
  apply: no type config or database file format changes and no database has to be regenerated.
- Layout note for the release that ships this: `AreaIndex` gains a private member and two additive
  public getters, so a binary consumer has to be rebuilt against the new library. The repository has
  no release-notes file to record it in; this file is the record until the change is archived.
