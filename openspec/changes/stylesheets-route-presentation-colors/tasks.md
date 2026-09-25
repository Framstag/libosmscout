# Tasks

## 1. Shared route paint per presentation (spec: route-visualization)

- [x] 1.1 In `stylesheets/include/route.oss`, define `routeColor` and a new `routeCasingColor` inside `IF daylight { ... } ELSE { ... }` (daylight: opaque violet fill, dark violet casing; dark: the current fill and a white casing) and use `@routeCasingColor` in the route outline rule instead of the literal white. Verify: `OSTAndOSSTest --warning-as-error map.ost standard.oss` and `... winter-sports.oss` still pass.
- [x] 1.2 Check that no daylight road class of the style sheets uses the daylight route colour, so the route cannot be confused with a road. Verify: a search over `stylesheets/include/` and `stylesheets/*.oss` for the chosen colour literal finds it only in `include/route.oss`.

## 2. Cycle style sheet adopts the shared paint (spec: route-visualization)

- [x] 2.1 In `stylesheets/cycle.oss`, remove the `COLOR routeColor` declaration and the `[TYPE _route] WAY` rule in the `[MAG world-]` block and add `MODULE "include/route"` to its module list, with a comment that names the module as the source of the route paint. Verify: `OSTAndOSSTest --warning-as-error map.ost cycle.oss` passes, and the style sheet no longer contains a route colour or a `_route` line rule of its own.

## 3. Test (spec: route-visualization)

- [x] 3.1 Add `Tests/src/RouteStyleColorsTest.cpp`: load `map.ost` plus a style sheet, resolve the `_route` line styles through `StyleConfig::GetRouteLineStyles` with a mercator projection, and assert for the daylight presentation (opaque violet fill over a dark violet casing), for the dark presentation (the dark variant) and for the casing/fill stacking (the outline style first, by slot and by display width). Verify: the test passes for `standard.oss`.
- [x] 3.2 Extend the test to every style sheet that can draw a route (`standard.oss`, `winter-sports.oss`, `cycle.oss`): each of them resolves the same casing and fill colours, and the cycle style sheet resolves the shared casing and fill rather than a single rule of its own. Verify: the test passes for all three style sheets.
- [x] 3.3 Register the test in both build systems. Verify: `Tests/CMakeLists.txt` and `Tests/meson.build` both reference `RouteStyleColorsTest`, and the CMake build builds and runs it.

## 4. Verification

- [x] 4.1 Verify that the changed style sheets still parse without warnings. Verify: `cd build && ctest -R CheckStyleSheet --output-on-failure` passes for all seven style sheets.
- [x] 4.2 Verify the new test and the existing style and rendering tests. Verify: `cd build && ctest -R RouteStyleColorsTest --output-on-failure` and `ctest -R "StyleConfig|OSTAndOSS" --output-on-failure` pass.
- [x] 4.3 Verify that the C++ test suite still passes. Verify: `cd build && ctest -j 2 --output-on-failure` reports no new failures.
- [x] 4.4 Verify that the clients that load these style sheets still work. Verify: `cd JavaScout && mvn test -Dnative.lib.dir=<build>/lib` passes, including `OSMScoutClientStyleTest`, which loads the real `standard.oss` together with the `include/` directory it references.

## 5. Build and quality gates

- [x] 5.1 Verify the full build compiles without errors and without new warnings for the affected targets. Verify: the CMake build (`cmake --build build`) and the Meson build (`meson compile -C build-meson`) both complete.
- [x] 5.2 Verify the new test file against the project rules. Verify: `clang-tidy` with the repository `.clang-tidy` reports no new findings for `Tests/src/RouteStyleColorsTest.cpp`, and the file has no uncrustify drift (`uncrustify -c .uncrustify -l CPP -f <file> | diff - <file>` is empty). **Result:** both hold. The first tidy run reported 26 findings in the new file; all were fixed rather than suppressed (include-cleaner includes, throwing static initialisation of the colour constants turned into functions, a `std::array` instead of a C array, named projection constants, `at()` instead of `[]`, and the per-style-sheet assertions moved into a helper to keep the test case's cognitive complexity down). The one justified `NOLINTNEXTLINE(concurrency-mt-unsafe)` covers the `std::getenv` read that the sibling style tests also do. Final: 0 findings, 0 uncrustify drift.
