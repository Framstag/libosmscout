# Tasks

## 1. Extract the derivation (spec: search-context-region)

- [x] 1.1 Create `libosmscout-client-java/src/search_scope_extent.h` declaring
  `naviveylin::DeriveScopeExtent(const osmscout::DBInstanceRef&, const osmscout::AdminRegionRef&)` beside
  `search_scope.h`. Verify: the header includes no JNI header, so the host suite can compile a user of it.
- [x] 1.2 Create `libosmscout-client-java/src/search_scope_extent.cpp` with the function body moved verbatim
  from `OSMScoutClient.cpp`. Verify: the body matches the removed block, apart from the namespace
  qualification.
- [x] 1.3 In `libosmscout-client-java/src/OSMScoutClient.cpp`, remove the `static` function, add
  `#include "search_scope_extent.h"` beside `#include "search_scope.h"`, and qualify the one call site in
  `DoSearchLocations`. Verify: the removal/addition diff is limited to the move and the include.
- [x] 1.4 Add `src/search_scope_extent.cpp` to `libosmscout-client-java/CMakeLists.txt` and
  `libosmscout-client-java/src/meson.build`. Verify: `cmake --build build --target osmscout_client_java`
  builds.

## 2. Host test (spec: search-context-region)

- [x] 2.1 Add `Tests/src/SearchScopeExtentTest.cpp`, which imports `Tests/LocationTest.olt` at start-up and,
  over every outline region the fixture's tree holds, asserts the derived box is the bounding box of the
  region's own object. Verify: the case fails if the derivation reads the wrong object.
- [x] 2.2 Add a case asserting a hit inside the derived extent is kept and one a degree outside it is
  dropped. Verify: the four `IsInsideGeoBox` decisions.
- [x] 2.3 Add a case asserting a region without a loadable object (no region, an unresolvable object offset)
  leaves the extent unset and admits a position anywhere. Verify: `extent.isSet` is false and
  `IsInsideGeoBox(extent, 0.0, 0.0)` is true.
- [x] 2.4 Register the test in `Tests/CMakeLists.txt` and `Tests/meson.build` (compile
  `search_scope_extent.cpp` beside the case). Verify: `cmake --build build --target SearchScopeExtentTest`
  builds.

## 3. Verification (spec: search-context-region)

- [x] 3.1 Rebuild the test target and run `ctest -R SearchScopeExtentTest --output-on-failure` from `build`.
  Verify: every case passes.
- [x] 3.2 Rebuild `osmscout_client_java`. Verify: the JNI module still compiles and links with the moved
  source.
- [x] 3.3 Build the Meson test and run `meson test "Check search scope extent"` in the Meson build directory.
  Verify: it passes.
- [x] 3.4 Make the derivation read the database's box instead of the region's object, rebuild and run the
  test. Verify: the case fails on a region whose own box differs from the database's box; restore the
  derivation and verify it is green again.
- [x] 3.5 Run `openspec validate client-java-scope-extent-host-unit --strict`. Verify: it passes.

## 4. Change hygiene

- [x] 4.1 Verify the diff touches only the files the proposal's Impact section lists, and that no database,
  map, style or file-format-version file changed.

## Workflow follow-up

- Archive the change once the review requirements are satisfied.
- Verify the archived result.
