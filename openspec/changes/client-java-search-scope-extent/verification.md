# Verification evidence

## 1. CMake

- `cmake --build build --target SearchScopeTest osmscout_client_java` — compiles the extended test, the
  bridge and the Java sources; no error and no compiler warning from the touched files.
- `cd build && ctest -R SearchScopeTest --output-on-failure` — `1/1 Test #62: SearchScopeTest ... Passed`,
  `100% tests passed out of 1`.
- `./build/Tests/SearchScopeTest` — `All tests passed (50 assertions in 9 test cases)`. The three cases that
  the earlier `test-search-scope-rule` change added are unchanged; the six new cases are the extent ones.
- `cmake --build build --target java_jar` followed by
  `javap -classpath build/libosmscout-client-java/libosmscoutclientjava.jar ...LocationEntry` — reports
  `public boolean inSearchScope`, so the field the bridge looks up by name and descriptor `Z` exists in the
  shipped class.

## 2. JNI signature parity

- `bash scripts/check-jni-signatures.sh` — `JNI signatures match: 55 native declarations checked, 6 dead JNI
  functions reported above.`; the six reported functions are pre-existing and none of them was touched by
  this change.

## 3. Meson

- `meson compile -C build-meson SearchScopeTest osmscout_client_java libosmscoutclientjava` — builds the
  test, the bridge and the jar without errors.
- `meson test -C build-meson "Check search scope"` — `OK`.
- `meson test -C build-meson "Check JNI signature parity"` — `OK`.

## 4. Superset property has teeth

- The containment case was made to fail on purpose: the region box in
  `SearchScopeTest.cpp` ("A region's bounding box never drops a position inside the region") was shrunk to
  `BoxFromCorners(51.60, 7.60, 51.70, 7.70)`, which no longer contains the point box standing in for an
  object inside the region. Rebuild and run gave
  `REQUIRE( naviveylin::GeoBoxContains(region, insideTheRegion) ) with expansion: false`,
  `9 test cases: 8 passed | 1 failed`. Restoring the box and running again gave
  `All tests passed (50 assertions in 9 test cases)`.

## 5. Spec scenario coverage

| Scenario | How it is verified |
|----------|--------------------|
| A hit inside the extent is admitted | Host test: `A scoped search admits a position inside the extent and rejects one outside` (the admitting half). The admission of a free-text hit by the bridge cannot be host-tested (see 6). |
| A hit outside the extent is rejected | Host test: the same case (the rejecting half). The bridge's filter is the same call, see 6. |
| The extent never drops a hit inside its own region | Host test: `A region's bounding box never drops a position inside the region`, plus the teeth check in 4. |
| A region represented only by a position gets a box around it | Host test: `The node-region fallback box contains its point at the documented size`. The bridge's node branch is not host-testable, see 6. |
| An unestablished extent admits every position | Host test: `An unavailable extent admits every position instead of emptying the search`; the bridge reaches the unset box from no region, no database, an unloadable object or an invalid box (`DeriveScopeExtent`). |
| A non-finite position is rejected | Host test: `A non-finite position is never inside a set extent`. |
| A scoped search reports the verdict per result | Not host-testable, see 6. The reported value is exactly the helper's answer for the entry's own position, which the coordinate and structured serialization paths show in the diff. |
| A search without a scope reports every result as inside | Not host-testable, see 6. It follows from the unset box admitting every position (host-tested) and from the field's default. |

## 6. What is not verified here, and why

- The bridge part — deriving the extent from a loaded region object, dropping free-text hits outside it and
  writing the verdict into each result — has no host test: it needs an open database and a resolved admin
  region. The map databases on this machine are format v26 while the library expects v27, so they cannot be
  opened, and importing a v27 database is out of scope for this change. `tasks.md` 4.4 (verify on a real
  map) is therefore left unticked. No map-dependent test was invented for it.
- `tasks.md` 4.5 (the rest of the C++ and Java suites) is left unticked: the shared `build/` directory holds
  binaries from other branches and a full-suite run would need every target rebuilt, which the environment
  rules here do not allow; the `JavaScout` Maven suite needs the client jar installed into the local Maven
  repository and a usable map database. The touched surface is covered by `SearchScopeTest`, the JNI
  signature parity check and both build systems compiling the bridge and the Java sources.
- `scripts/format-check.sh` is red repo-wide on this machine, including pristine master files, so it is not
  used as a gate; the added lines follow the style of the code around them.

## 7. Change hygiene

- `git diff --stat` reports exactly the four files of the proposal's Impact section:
  `libosmscout-client-java/src/search_scope.h`, `libosmscout-client-java/src/OSMScoutClient.cpp`,
  `libosmscout-client-java/java/com/framstag/libosmscout/client/LocationEntry.java` and
  `Tests/src/SearchScopeTest.cpp`. No database, map, style-sheet, type-definition or file-format-version file
  is in the diff, so no `FileFormatVersion.md` bump applies.
- `Tests/CMakeLists.txt` and `Tests/meson.build` already register `SearchScopeTest` with the include path of
  `libosmscout-client-java/src`, so no build-file change was needed.
- The existing product-specific namespace and header guard (`naviveylin`, `NAVIVEYLIN_SEARCH_SCOPE_H`) are
  kept as master already carries them from the `test-search-scope-rule` change; the change introduces no new
  product name (see `tasks.md` 1.2).
- `openspec validate client-java-search-scope-extent --strict` — `Change 'client-java-search-scope-extent'
  is valid`.
