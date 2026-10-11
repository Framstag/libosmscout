# Tasks

## 1. Test foundation

- [x] 1.1 Add `Tests/src/AreaIndexLookupTest.cpp` that opens the committed database `Tests/data/testregion` (from `argv[1]` or `TESTS_TOP_DIR/data/testregion`, the pattern of `Tests/src/StyleLoadResilienceTest.cpp:40-57`) and asserts the resolved offsets and the reported loaded types of a narrow request against `Database::GetAreaWayIndex()` (`libosmscout/include/osmscout/db/Database.h:415`) — this is the reference result of the spec requirement "The result of an area-index lookup follows the request, not the type set" and of "An area-index lookup reports the request's types as loaded". Verify: the test passes against the current lookup, i.e. it records today's behaviour before anything changes.
- [x] 1.2 Register `AreaIndexLookupTest` in `Tests/CMakeLists.txt` (the `osmscout_test_project(... COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")` form) and in `Tests/meson.build` (the `test(... args : [...])` form), and verify both build systems run it: `ctest -R AreaIndexLookupTest` and `meson test -C build-meson "Check area index lookup"`.

## 2. The lookup in the shared base class

- [x] 2.1 In `libosmscout/include/osmscout/db/AreaIndex.h` and `libosmscout/src/osmscout/db/AreaIndex.cpp`, build the entry-position table keyed by the global type index at `Open()` (design D1) with the invariants of design D4, and add the per-instance examined-entry counter with its getter (design D2, the `MapPainterCairo.h:125` pattern). Verify: the project builds in both build systems and `AreaIndexLookupTest` still reports the reference result of 1.1.
- [x] 2.2 Rewrite the public `AreaIndex::GetOffsets` (`libosmscout/src/osmscout/db/AreaIndex.cpp:205-248`) to iterate the requested `TypeInfoSet` and read only the entries the request resolves, keeping the append semantics and the loaded-types rule of design D3. Verify: `AreaIndexLookupTest` and `ctest -R ThreadedDatabaseTest` pass.
- [x] 2.3 Extend `AreaIndexLookupTest` to the second derived index, `Database::GetAreaRouteIndex()` (`Database.h:416`) — no test exercises it today — and verify the same reference semantics there (spec requirement "The result of an area-index lookup follows the request, not the type set"). Verify: the extended test passes against the rewritten lookup.

## 3. Cost assertions

- [x] 3.1 Assert the examined-entry count of a narrow request: it SHALL equal the number of requested types the index carries and SHALL be far below the index's entry count — the spec scenario "A request that names a subset touches only that subset". Verify: the test fails when the rewritten loop is reverted to the loop over `typeData` (check the discrimination with the `verify-test-discriminates` skill rather than by inspection).
- [x] 3.2 Assert the identity "examined entries = requested types the index carries" for several requests, including a request that names types the index carries no entry for — the spec scenario "Adding indexed types outside the request does not enlarge the lookup". Verify: the identity holds for every request in the test; state in the test's comment that the identity, not a second database, establishes the scenario, because the repository commits only one database and the second scenario's two-index comparison is not reproducible here.
- [x] 3.3 Assert that a requested type whose entry resolves no offset is still reported as loaded (spec scenario "A named type that resolves no offset is still reported as loaded"), choosing a request whose box lies outside the test region. Verify: the assertion fails if `loadedTypes` is set only for entries that contributed an offset.

## 4. Build and regression checks

- [x] 4.1 Build both configurations without new warnings: `cmake --build build` and `meson compile -C build-meson`. Verify: both commands succeed and the compiler output for the touched files contains no warning that HEAD does not already produce.
- [x] 4.2 Run the existing suites: `cd build && QT_QPA_PLATFORM=offscreen ctest -j 2 --output-on-failure` with `TESTS_TOP_DIR`/`TESTS_TMP_DIR` set as documented in `AGENTS.md`, and `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs`. Verify: no test that passed before the change fails.
- [x] 4.3 Check the touched files against the repository conventions: `scripts/format-check.sh` (uncrustify 0.83.0) and `clang-tidy -p build` on `AreaIndex.cpp`/`AreaIndex.h`/`AreaIndexLookupTest.cpp`, plus `guidelines/CodeStyles.md`. Verify: no *new* finding relative to HEAD — the tree is not clean repo-wide, which `TODO.md` records, so the comparison is per file and per finding, not a clean run.
- [x] 4.4 Verify the change touched no data or format: `git diff --stat` lists only library and test files. Verify: no `.dat`, `.idx`, `stylesheets/`, importer or generated file appears in the diff.

## 5. Documentation and bookkeeping

- [x] 5.1 Record the before/after examined-entry counts and the result-equality evidence in the change's `verification.md`, the way `openspec/changes/fix-pattern-path-wiring/verification.md` does. Verify: the file exists and names the commands the numbers came from.
- [x] 5.2 Add the "Closed by `improve-area-index-offset-lookup`" note to the `AreaIndex::GetOffsets` entry in `TODO.md`, in the form the file already uses. Verify: the note names the change and the file's own convention for a later removal is followed.
- [x] 5.3 Record the layout note of design D2 (a private member and one additive getter on `AreaIndex`) where the release notes are collected, and verify no other guideline in `guidelines/` applies: `guidelines/FileFormatVersion.md` is not triggered because no type config or database file format changes.
