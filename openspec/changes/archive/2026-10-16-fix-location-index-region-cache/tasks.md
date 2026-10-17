# Tasks

## 1. The red case and its seam

- [x] 1.1 In `libosmscout/include/osmscout/db/LocationIndex.h` and
  `libosmscout/src/osmscout/db/LocationIndex.cpp`, add the cache-aware region load
  (`LoadAdminRegion(FileScanner&, FileOffset, AdminRegionRef&)`), route every region load site through it
  (`VisitRegionEntries`, the root loop of `VisitAdminRegions`, `VisitLocations`, `VisitRegionPOIs`,
  `VisitPostalArea`, `ResolveAdminRegionHierachie`), and add the region-read diagnostic
  `GetRegionReadCount()` guarded by `regionCacheMutex`, reset by `FlushCache()`. Verify: the project builds
  in both build systems and the case of 1.2 fails (the tally below).
- [x] 1.2 Add `Tests/src/LocationIndexRegionCacheTest.cpp` with the case "A second traversal of the region
  tree reads no region entry" (spec `location-index-lookup`, "A lookup deserializes a region entry once while
  the index retains it"), which traverses the region tree twice and reads
  `LocationIndex::GetRegionReadCount()` after each traversal; register the file in the `LocationLookupTest`
  target in `Tests/CMakeLists.txt` and in the `LocationServiceTest` executable in `Tests/meson.build`.
  Verify: the case fails without the cache: `REQUIRE( readsOfSecondTraversal==0 )`, expansion `12 == 0`,
  `test cases: 12 | 11 passed | 1 failed`, `assertions: 309 | 308 passed | 1 failed`, 2026-10-16T05:52:40Z,
  logged in `evidence/red-test.log`.

## 2. The retention

- [x] 2.1 In `libosmscout/include/osmscout/db/LocationIndex.h` and
  `libosmscout/src/osmscout/db/LocationIndex.cpp`, add the retained region entries
  (`std::unordered_map<FileOffset,AdminRegionRef> regionCache`), answer the cache-aware load from them and
  insert on a miss, and clear them in `FlushCache()` (design "Policy": size is the region entries the index
  holds, no eviction but the flush). Verify: `cmake --build build --target LocationLookupTest` compiles and
  the case of 1.2 passes, printing `second traversal: 12 regions visited, 0 region entries read`, with the
  target's tally `All tests passed (309 assertions in 12 test cases)` in `evidence/post-fix-test.log`.
- [x] 2.2 Add the case "A traversal after a flushed cache reads the regions it visits" to
  `Tests/src/LocationIndexRegionCacheTest.cpp` (spec `location-index-lookup`, "A flush releases the retained
  region entries"): a traversal after `FlushCache()` reads exactly one region entry per visited region.
  Verify: the case passes and reports `12 regions visited, 12 region entries read`.

## 3. The probe for the added invariant

- [x] 3.1 Probe the release: drop the `regionCache.clear()` line in `LocationIndex::FlushCache()`, marking the
  edited line `TEMP-REVERT-PROBE`, and run the case of 2.2. Verify: the case fails with
  `REQUIRE( readsAfterFlush==afterFlushTraversal.regions.size() )`, expansion `0 == 12` (and the first case
  fails on its read of the flushed index, `REQUIRE( readsOfFirstTraversal==firstTraversal.regions.size() )`,
  expansion `0 == 12`); archive the log in `evidence/probe.log`, then restore the tree (`git diff` of the
  production files empty, no marker left).

## 4. Build and regression checks

- [x] 4.1 Run the forced gate of the `code` shape: `cmake --build build -j "$(nproc)"` then
  `cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests" TESTS_TMP_DIR="$PWD/Tests" ctest -j 8
  -E 'NumberSetPerformanceTest' --output-on-failure` (the exclusion: that performance test alone needs ~112 s
  and this change is not what it measures), and `meson compile -C debug` then
  `meson test -C debug --timeout-multiplier 2 --print-errorlogs`. Verify: both suites report no failing test
  and the log of each run is copied into `evidence/` with its command, UTC timestamp, exit code and the
  `sha256sum` of the sources it pinned.
- [x] 4.2 Confirm the existing contracts of the touched target: the other cases of `LocationLookupTest`
  (string search, form search, POI search) pass unchanged. Verify: `ctest -R LocationLookupTest` passes with
  all 12 cases, and `git diff` shows no change outside `LocationIndex.{h,cpp}`, the new test file and the two
  build lists.
- [x] 4.3 Check the touched files against the repository conventions:
  `uncrustify -q -c .uncrustify -l CPP -f <file>` for `LocationIndex.h`, `LocationIndex.cpp` and
  `LocationIndexRegionCacheTest.cpp`, plus `guidelines/CodeStyles.md`. Verify: no *new* hunk relative to
  `HEAD` for the added lines; the tree is not clean repo-wide (`TODO.md` records that for other files), so
  the comparison is per file and per hunk.

## 5. Records

- [x] 5.1 Run `evidence-check.sh` in live mode against the change and quote its output. Verify: exit code 0
  and no dangling claim; `verification.md` names, for every number the change cites, the evidence file that
  carries it with its command, UTC timestamp, exit code and pin where one exists.
- [x] 5.2 Set `TODO.md` §37 to `**status:** fixed-by \`fix-location-index-region-cache\`` (the claim of
  `proposal.md` Impact) and, because the run was started with `--allow-todo-removal yes` and the archive
  exists, remove the entry afterwards; file the retained-entry mutability the fix surfaces as the new entry
  §105. Verify: the entry bullet had exactly one metadata line before removal, the structure check still
  holds afterwards (`grep -c '^- \*\*'` == `grep -c '^\s*\*\*id:\*\*'`) and no `§id` collides; the
  `**Clusters**` index is not edited here (`cleanup-todo` regenerates it).
