# Verification evidence

## 1. CMake

- `cmake --build build --target SearchScopeExtentTest` — builds the case and the moved derivation; 17 ninja
  steps, no error and no compiler warning from the touched files (the only warning is the pre-existing
  `-Wdeprecated-enum-enum-conversion` in `libosmscout-import/src/osmscoutimport/GenTextIndex.cpp`).
- `cd build && ctest -R SearchScopeExtentTest --output-on-failure` — `1/1 Test #68: SearchScopeExtentTest
  ... Passed`, `100% tests passed out of 1`.
- `cd build && ctest -R SearchScopeExtentTest -V | grep -E "assertions|test cases"` —
  `All tests passed (115 assertions in 3 test cases)`, matching the reference's figure.
- `cmake --build build --target osmscout_client_java` — the JNI module compiles and links with the moved
  source: `[2/2] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so`.

## 2. Meson

- `meson setup build-meson -DenableMapAgg=false -DenableMapDirectX=false -DenableMapIOSX=false
  -DenableMapQt=false -DenableClientQt=false -DbuildDemos=false` — configures (the disabled backends keep
  the build small; none of them is involved in this unit).
- `meson compile -C build-meson SearchScopeExtentTest` — builds the target and its dependencies; 274 steps,
  no error.
- `meson test -C build-meson "Check search scope extent"` — `1/1 libosmscout:Check search scope extent OK
  2.36s`, `Ok: 1 Fail: 0`.

## 3. Falsification: the derivation must read the region's own object

The derivation was made to read the database's own box instead of the region's object, in
`libosmscout-client-java/src/search_scope_extent.cpp` (the `refArea` branch changed from
`box=area->GetBoundingBox()` to `loaded=database->GetBoundingBox(box)`). Rebuilt and run:

- `cmake --build build --target SearchScopeExtentTest`
- `cd build && ctest -R SearchScopeExtentTest --output-on-failure` — `***Failed`, `0.67 sec`:

```
Tests/src/SearchScopeExtentTest.cpp:218: FAILED:
  REQUIRE( extent.maxLon==Catch::Approx(objectBox->GetMaxCoord().GetLon()) )
with expansion:
  11,00000076740980148
  Approx( 10,3333323473731582 )
  region: Regierungsbezirk Düsseldorf
test cases:  3 |  2 passed | 1 failed
assertions: 30 | 29 passed | 1 failed
```

The failing region is the reference's own (`Regierungsbezirk Düsseldorf`), and the two values match the
reference's expansion (`11.000000767 == 10.333332347`). The derivation was then restored (the working tree
matches the index again — `git diff -- libosmscout-client-java/src/search_scope_extent.cpp` is empty),
rebuilt and run green again: `1/1 Test #68: SearchScopeExtentTest ... Passed`,
`100% tests passed out of 1`.

## 4. Why the case imports its own fixture

`Tests/data/testregion/location.idx` is 77 bytes (measured 2026-11-01) — a header with no region — so the
committed database holds no admin region to derive an extent from. The case therefore imports
`Tests/LocationTest.olt` (a full import, not eco) into its own subdirectory of `TESTS_TMP_DIR` and removes
the import's `coord.dat` temporary once done, exactly as the reference does. It was not weakened to a region
the committed fixture does not hold.

## 5. Spec scenario coverage

| Scenario | How it is verified |
|----------|--------------------|
| The host suite derives the extent of a real region | `A scope extent is the bounding box of the region's own object` iterates every outline region of the imported fixture and asserts the derived box equals the object's box. |
| A derivation from the wrong object is observable | The falsification in section 3 fails that case on `Regierungsbezirk Düsseldorf`. |
| A region without a loadable object leaves the extent unset | `A region without a loadable object leaves the extent unset` asserts an empty handle and an unresolvable object offset both leave `isSet` false and admit a position anywhere. |

The reference figure (115 assertions in 3 cases) is reproduced in both build systems.

## 6. What is not verified here, and why

- The moved derivation's node branch (a region represented only by a position) is not reachable from this
  fixture: the import writes a 93-byte `nodes.dat` carrying no node object that represents a region, so no
  case can build one. The fallback box it produces is the pure `BoxAroundPoint` function, which
  `Tests/src/SearchScopeTest.cpp` already covers; the case documents this rather than inventing a fixture.
- The JNI call site in `DoSearchLocations` is a name change only; it is exercised by the JavaScout suite,
  which this change does not touch and which needs a usable map database. No map database was imported for
  it: the `maps/` databases here are format v26 while the library expects v27.
- `scripts/format-check.sh` is red repo-wide; it was not used as a gate. The added lines follow the style of
  the code around them.

## 7. Change hygiene

- `openspec validate client-java-scope-extent-host-unit --strict` — `Change
  'client-java-scope-extent-host-unit' is valid`.
- The diff touches only the files the proposal's Impact section lists: the two new `libosmscout-client-java`
  sources, `OSMScoutClient.cpp`, `Tests/src/SearchScopeExtentTest.cpp`, the two `libosmscout-client-java`
  build files and the two `Tests` build files. No database, map, style or file-format-version file changed.
- The two new files and the test are byte-identical to the reference commit's blobs
  (`git diff --stat 5188f2138 -- <those three paths>` is empty), and the build-file hunks carry the
  reference's own blob ids, so this port is the reference's content against current `master`.
