# Tasks

## 1. Accounting unit and budget type (libosmscout-map)

- [x] 1.1 Add the accounting unit with the per-kind weights (point 1, line 3, polygon 30, route 3,
  index cell 1) derived from the object layouts, plus the bytes-per-weight factor. Spec:
  `map-data-memory-budget` — "Cached map data is accounted in comparable units". Verify: new unit test
  asserts that the weight of a polygon object exceeds the weight of a point object and that the
  reported size is the sum of the weights of the accounted objects.
- [x] 1.2 Add the budget type holding the total budget, the floor and the current total usage, and the
  conversion between memory units and weight units. Spec: "One budget bounds the caches of all open
  databases". Verify: unit test covers set/read of the budget, the floor, and the rejection of a
  non-positive budget.
- [x] 1.3 Extend the tile cache to account its content (per tile, per cache) and to report it, so that
  resizing is driven by the accounted size rather than by the tile count. Spec: "Cached map data is
  accounted in comparable units". Verify: unit test fills a cache with tiles of a known content and
  asserts the reported size and that a resize to a smaller accounted size strips the oldest tiles.
- [x] 1.4 Register the new files in both build systems (`libosmscout-map/CMakeLists.txt`,
  `libosmscout-map/meson.build`) and verify the library compiles without warnings in a CMake and in a
  Meson build.
- [x] 1.5 Register the new test file in `Tests/CMakeLists.txt` and `Tests/meson.build` and verify the
  new tests run and pass (`ctest -R` for the new test name).
- [x] 1.6 Document the accounting unit and its weights where the map service is documented, and verify
  the documented weights match the constants in the header.

## 2. Runtime resizing of the object caches (libosmscout)

- [x] 2.1 Add a size setter to the data file that takes the file's access mutex and resizes the object
  cache, so that the oldest entries are stripped. Spec: "One budget bounds the caches of all open
  databases". Verify: unit test opens a data file, fills its cache, resizes it smaller and asserts the
  cache holds fewer entries and still serves reads.
- [x] 2.2 Add a size setter to the database that keeps its parameter in sync and applies the new size
  to every already created data file, so that a data file created later inherits the size. Spec: same
  requirement. Verify: unit test resizes before and after a data file has been created and asserts
  both paths end with the requested size.
- [x] 2.3 Verify the existing database, cache and conversion tests still pass (`ctest` for
  `TileDataConversionTest` and the database tests) and that the library compiles without warnings in
  both build systems.

## 3. Relevance, distribution and hysteresis (libosmscout-map)

- [x] 3.1 Derive relevance of a database from its geographic extent and the view area, using the
  database the map service already holds, and expose it to the budget. Spec: "The budget is distributed
  by relevance to the view". Verify: unit test with two databases whose extents do and do not cover the
  view asserts the relevant set contains exactly the covering database.
- [x] 3.2 Distribute the budget over the relevant databases and reduce the others to the floor when a
  distribution is applied. Spec: "The budget is distributed by relevance to the view". Verify: unit test
  asserts in-view databases hold shares, out-of-view databases are at the floor, and the total stays at
  or below the budget.
- [x] 3.3 Apply a distribution only after the relevant set has been stable for the settling period, and
  keep a share when a database becomes relevant again inside that period. Spec: "Changes of the relevant
  set are hysteretic". Verify: unit test drives a relevance sequence with a controllable clock and
  asserts no rebalance before the period elapses and one rebalance after it.
- [x] 3.4 Verify the tile loading and map service tests still pass and that the accounting does not
  change the objects a view contains: unit test renders the same view with a budget far below and far
  above the data it needs and asserts both object sets are equal. Spec: "Memory bounding does not change
  the rendered map".

## 4. Idle release of out-of-view databases

- [x] 4.1 Release the caches of a database that has not been relevant for longer than the idle period,
  using the database's existing cache release for the object caches and the cache's own invalidation for
  the tile cache. Spec: "Databases that stay out of view release their caches". Verify: unit test drives
  a database out of view past the idle period and asserts its accounted size is zero.
- [x] 4.2 Verify a released database works again: unit test makes it relevant again, loads tiles and
  asserts loading succeeds and the accounted size rises above zero. Spec: same requirement.
- [x] 4.3 Verify the library compiles without warnings in both build systems and that the release does
  not break concurrent loading (the load and release tests run under the sanitizer configuration).

## 5. Client wiring (Qt and Java)

- [x] 5.1 Let the map service constructor accept an optional shared budget, and have the database
  thread own one budget per client and hand it to every map service it creates. Spec: "The budget is
  configurable and observable by clients". Verify: builds in both build systems and a unit test
  creates two map services with one budget and asserts the total usage covers both.
- [x] 5.2 Add the budget configuration and a mobile default budget to the Qt client builder next to the
  existing tile cache size configuration. Spec: "Shipped clients bound memory without configuration".
  Verify: a Qt client test renders with the default and asserts the total usage is at or below the
  default.
- [x] 5.3 Add a budget setting to the Java client, keep the existing per-database cache size effective
  when no budget is set, and stop applying the per-database size to every database on every render.
  Spec: "No budget configured keeps per-database sizing" and "Shipped clients bound memory without
  configuration". Verify: the existing cache size test still passes and a new test covers setting a
  budget and reading the usage through the bridge.
- [x] 5.4 Document the budget, its units, its defaults and the unchanged meaning of the per-database
  cache size in `Documentation/` and in the Javadoc of the Java API, and verify the documented default
  matches the value in the builder.
- [x] 5.5 Verify the Qt and Java clients build (`cmake --build` and the Java target) and that their
  existing tests still pass.

## 6. Integration checks

- [x] 6.1 Verify a full build of the affected subprojects succeeds without warnings in CMake and that
  the Meson build of the same subprojects succeeds.
- [x] 6.2 Verify the full test suite passes (`ctest -j 2 --output-on-failure`), including the tile
  conversion, database and cache tests, and report any test that needed a change.
- [x] 6.3 Verify tools and tests that configure no budget are unaffected: `PerformanceTest` with its
  cache size arguments and `TileDataConversionPerformanceTest` produce the same results as before the
  change.
- [x] 6.4 Record the two pre-existing defects (the client's idle release path has no caller, and a
  render refreshes the last-usage time of every database) in `TODO.md` with the files and lines found,
  and verify `TODO.md` names both.
- [x] 6.5 Run the static analysis and formatting checks the project requires on all touched files
  (`.clang-tidy`, `.uncrustify`) and verify they report no new findings.
