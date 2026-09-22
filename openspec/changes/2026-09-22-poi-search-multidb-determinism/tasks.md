# Tasks

## 1. Test harness for overlapping databases

- [x] 1.1 Add the multi-database test harness to
  `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java`: a
  directory property gate (`poi.test.multidb.dir`) that skips the scenarios when unset, plus a
  helper that repeats a search until two consecutive calls return an identical list (the databases
  load asynchronously). Verify: the suite runs green with the property unset, and the new scenarios
  are reported as skipped. (spec `poi-search-api` — "POI search covers every loaded database")
- [x] 1.2 Add the multi-database scenario tests for the three delta requirements — every loaded
  database is searched, a duplicated POI collapses to one entry, and the order is reproducible —
  against a directory with two overlapping extracts. Verify: with the property set, the new tests
  fail against the current implementation (red), confirming they exercise the change.
  (spec `poi-search-api` — all three added requirements)

## 2. Multi-database merge in the POI search

- [x] 2.1 Order the databases of the `RunSynchronousJob` walk so that databases whose area contains
  the search center come first, preserving discovery order inside each group. Verify: the
  containment-precedence scenario passes, and a search with a single loaded database returns the
  same list as before. (spec `poi-search-api` — "A POI present in several databases is returned
  once")
- [x] 2.2 Search every loaded non-basemap database instead of stopping once the result list reaches
  the requested limit, and keep truncating the merged list to the limit after sorting. Verify: the
  scenario where a database that fills the limit hides a nearer hit of another database passes, and
  the result never exceeds the limit. (spec `poi-search-api` — "A database filling the limit does
  not hide nearer results", "The limit applies to the merged list")
- [x] 2.3 Deduplicate the merged list, keeping the first occurrence per object type plus coordinate
  rounded to 1e-5 degrees, so the copy from the database containing the search center survives
  (design decision 3). Verify: the duplicate-collapse and winner scenarios pass, and the surviving
  entry equals the entry the same search returns with only that database loaded.
  (spec `poi-search-api` — "A POI present in several databases is returned once")
- [x] 2.4 Replace the distance-only comparator with a total order over distance, label, object type
  and coordinates. Verify: the same-distance scenario returns the label-first entry first, and two
  identical searches over the same database set return identical lists.
  (spec `poi-search-api` — "POI search result order is deterministic")

## 3. Build, regression and conventions

- [x] 3.1 Build `libosmscout-client-java` through the Meson option
  (`meson setup build -DOSMSCOUT_BUILD_CLIENT_JAVA=ON && meson compile -C build`) and confirm the
  native library and the JAR build with no new warnings. Verify: build completes clean.
- [x] 3.2 Run the existing JavaScout test suite with `poi.test.multidb.dir` unset and confirm the
  single-database POI scenarios behave exactly as before the change. Verify: suite green, no changed
  expectations in the pre-existing scenarios.
- [x] 3.3 Run the multi-database suite with `poi.test.multidb.dir` pointing at overlapping extracts
  and confirm all new scenarios pass. Verify: suite green.
- [x] 3.4 Run the repository test suite for the affected modules (client-java plus the client and
  map libraries it links) and confirm no regressions. Verify: `ctest`/`meson test` green for those
  targets.
- [x] 3.5 Check `libosmscout-client-java/src/OSMScoutClient.cpp` against `guidelines/` and run
  `.clang-tidy` and `.uncrustify` on it; confirm the change introduces no formatting or analysis
  findings. Verify: formatter and tidy report no new findings.
- [x] 3.6 Confirm the change needs no public API or documentation update — `PoiEntry`, the category
  mapping and the `searchPOIs` signature are untouched — and record that check in the change.
  Verify: no entry in `AGENTS.md`, `TODO.md` or `Documentation/` needs editing for this change.
