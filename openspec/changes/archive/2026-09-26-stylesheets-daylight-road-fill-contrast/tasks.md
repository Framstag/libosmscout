# Tasks

## 1. Daylight road fills (spec: road-fill-contrast)

- [x] 1.1 In `stylesheets/standard.oss`, lighten the daylight fill of the motorway, trunk, primary and secondary class (`#4440ec` → `#7d7af5`, `#7674ec` → `#a3a1f5`, `#ec4044` → `#f58b8b`, `#fdac44` → `#fdd08a`) and correct the two comments that name the old spellings. Verify: `CheckStyleSheet-standard.oss` passes, and the black way label measures 5.97:1 / 8.97:1 / 8.95:1 / 14.57:1 on the four fills (sRGB relative luminance), up from 3.19:1 on the motorway.
- [x] 1.2 In `stylesheets/winter-sports.oss`, take the same motorway and trunk fills; its primary and secondary class already sit in the required range. Verify: `CheckStyleSheet-winter-sports.oss` passes, and a search for the two literals finds them in the two style sheets and in the test's expectations, nowhere else.

## 2. Constants derived from a fill (spec: road-fill-contrast)

- [x] 2.1 In both style sheets, derive the thin variant of a class with 0.2 instead of 0.3. Verify: `RoadStyleColorsTest` resolves the thin paint at a zoom below the full road width and asserts it, and the thin variant stays lighter than the full width fill of its class.
- [x] 2.2 In both style sheets, give the highway shields their own darkened background (`darken(@*Color, 0.45)`) instead of the fill. Verify: `RoadStyleColorsTest` resolves the shield style and asserts the background of the motorway, trunk and primary class and its white text; the white shield text measures 8.78:1 / 6.64:1 / 6.66:1 on those backgrounds, where the new fills themselves would give it 3.52:1 / 2.34:1 / 2.35:1.
- [x] 2.3 In both style sheets, derive the motorway junction label with 0.3 instead of 0.5. Verify: `RoadStyleColorsTest` resolves the junction node text style of the standard style sheet and asserts its colour and that it differs from the motorway fill.

## 3. Test (spec: road-fill-contrast)

- [x] 3.1 Add `Tests/src/RoadStyleColorsTest.cpp`: load `map.ost` plus a style sheet, resolve the `_route`-style way line styles of `highway_motorway`, `highway_trunk`, `highway_primary` and `highway_secondary` through `StyleConfig::GetWayLineStyles` with a mercator projection, and assert the daylight fill of each class and of its thin variant. Verify: the test passes for `standard.oss` and `winter-sports.oss`.
- [x] 3.2 Assert the stacking of the cased paint (the outline darker than the fill and the wider, lower priority stroke) and the dark presentation (its fill differs from the daylight one). Verify: the test passes for both style sheets.
- [x] 3.3 Register the test in both build systems. Verify: `Tests/CMakeLists.txt` and `Tests/meson.build` both reference `RoadStyleColorsTest`, and the CMake build builds and runs it.

## 4. Verification

- [x] 4.1 Verify that the changed style sheets still parse without warnings. Verify: `ctest -R CheckStyleSheet --output-on-failure` passes for all seven style sheets.
- [x] 4.2 Verify the new test and the existing style tests. Verify: `ctest -R "RoadStyleColorsTest|RouteStyleColorsTest|StyleConfig" --output-on-failure` reports no failures, so the shared route module and the route paint are unaffected.
- [x] 4.3 Verify that the dark presentation is untouched. Verify: the dark section of `RoadStyleColorsTest` resolves the darkened variants, and a diff of the `ELSE` branches of the two `CONST` blocks shows no change.

## 5. Build and quality gates

- [x] 5.1 Verify the build compiles without errors for the affected targets. Verify: the CMake build builds `RoadStyleColorsTest` and the map library it links against, without new warnings for the affected targets. **Result:** the CMake build (`ninja RoadStyleColorsTest`) completes; the new file adds no compile warnings.
- [x] 5.2 Verify the new test file against the project rules. Verify: `clang-tidy` with the repository `.clang-tidy` reports no new findings for `Tests/src/RoadStyleColorsTest.cpp`, and the file has no uncrustify drift (`uncrustify -c .uncrustify -l CPP -f <file> | diff - <file>` is empty). **Result:** both hold. The first tidy runs reported findings in the new file (a `std::vector` with static storage duration in the zoom lists, two easily swapped string parameters, a test case over the cognitive complexity threshold, and the missing designated initialisers) - all were fixed rather than suppressed: the zoom lists became functions, the road class is passed as a `TypeInfoRef` instead of a second string, and the per style sheet assertions moved into helpers. Final: 0 findings, 0 uncrustify drift.
- [x] 5.3 Verify that the pinned fills are compared in the unit a palette decision is made in. Verify: the test compares `Color::ToHexString()` rather than `Color` objects, because `lighten()` lands one ULP away from a parsed literal; a run that compares the objects directly was observed to fail on an equal hex triple.
