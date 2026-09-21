## 1. Baseline

Parent spec: `map-painter-area-culling` - Requirement: A tolerance derived from a style-sheet width is a
screen-space length.

- [x] 1.1 Record the before-state of the fixed view: the prepared area and way counts and the
  `ProcessAreas` per-frame minimum of `Tests/PerformanceTest` rendered with
  `--driver cairo --start-zoom 15 --end-zoom 15 --draw-repeat 5 --load-repeat 1 maps/Dortmund
  stylesheets/standard.oss 51.514 7.463 51.510 7.470` on the pre-change build. Verification: the numbers,
  the cold-cache outlier and the tolerance in use are written into `verification.md` (section 1).
- [x] 1.2 Record which callers pass a millimetre value as the `pixelOffset` of `IsVisibleArea` /
  `IsVisibleWay` and which pass pixels. Verification: the audit is in `verification.md` (section 2) and
  names every caller, including the OpenGL site that stays unchanged.

## 2. Conversion at the call sites

Parent spec: `map-painter-area-culling` - Requirement: A tolerance derived from a style-sheet width is a
screen-space length.

- [x] 2.1 Convert the per-ring offset in `MapPainter::PrepareAreaRing`
  (`libosmscout-map/src/osmscoutmap/MapPainter.cpp`) with `projection.ConvertWidthToPixel` and update its
  comment to name the millimetre-to-pixel conversion. Verification: CMake and Meson builds succeed without
  warnings (section 5).
- [x] 2.2 Convert the early offset in `MapPainter::ProcessAreas` with the same projection call, so it is
  the converted form of the expression 2.1 applies per ring, and update the comment. Verification: the
  early offset is `ConvertWidthToPixel(maxAreaBorderWidthMM*0.5)`, i.e. the converted per-ring expression,
  and the probe of 3.1/3.2 shows the pair is not conservative with either conversion missing.
- [x] 2.3 Keep the debug assert that binds the per-ring border width to the derived bound and update its
  comment to state that both sides are compared in millimetres, before conversion. Verification: builds
  warning-free and the assert does not fire in the suites of section 6.
- [x] 2.4 Document the unit of `pixelOffset` on `MapPainter::IsVisibleArea` and `MapPainter::IsVisibleWay`
  in `libosmscout-map/include/osmscoutmap/MapPainter.h`, including that a width from a stylesheet has to
  be converted first. Verification: both helpers carry the doc comment, and Uncrustify reports no finding
  on the added lines (section 8).

## 3. Tests

Parent spec: `map-painter-area-culling` - Requirements: A tolerance derived from a style-sheet width is a
screen-space length, and The early rejection is conservative.

- [x] 3.1 Update the conservativeness test (`Tests/src/MapPainterAreaVisibilityCullTest.cpp`) so its
  per-ring tolerance and its bound comparison are converted values, and add a case that fails under the raw
  millimetre number. Verification: with both conversions reverted the test reports `0 == 1` for a 100 mm
  border (per-ring tolerance 590.551 px, raw value 50); with them in place the file passes (section 4).
- [x] 3.2 Add a regression test that prepares the same view with two projections which differ only in
  their DPI and asserts that an area within the converted tolerance of the higher DPI survives while the
  same area is rejected at the lower DPI. Verification: it fails with either conversion reverted
  (18.8976 px tolerance at 96 DPI, 59.0551 px at 300 DPI, reach 19.8976 px) and passes with them
  (section 4).
- [x] 3.3 Check the remaining assumptions of the test file and of `Tests/src/MapPainterAreaPreparationTest.cpp`
  for the unit and update any that encode the old behaviour. Verification: `MapPainterAreaPreparationTest`
  already converts its offset, the other culling tests pass unchanged, and no test asserts a millimetre
  value as a pixel distance (section 4).

## 4. Verification

Parent spec: `map-painter-area-culling` - all requirements; plus the regression concerns of the design.

- [x] 4.1 Verify a warning-free CMake build of the map library and the test targets. Verification: full
  build exit 0 with 6 warnings in the project, none in the changed files (section 5).
- [x] 4.2 Verify the Meson build of the same targets. Verification: the changed targets compile without
  warnings (section 5).
- [x] 4.3 Verify the test suite under CMake and under Meson, including the map painter, area preparation,
  culling, frame buffer, shield and text metric tests. Verification: 86/86 CMake tests (PerformanceTest
  excluded), 39/39 PerformanceTest tests, 125/125 Meson tests (section 6).
- [x] 4.4 Measure the after-state with the same fixed view and commands as 1.1 and record the prepared
  counts, the `ProcessAreas` minimum and the applied tolerance next to the baseline. Verification: the
  comparison is in `verification.md` (section 7); the loaded and prepared counts and the allocation count
  are unchanged and the tolerance grows from 0.05 to 0.189 px for the shipped stylesheet.
- [x] 4.5 Verify the changed lines against the project's linters (Uncrustify, clang-tidy). Verification: no
  finding on an added line in any of the three changed files; the remaining findings are the pre-existing
  drift and classes of `MapPainter.cpp`, `MapPainter.h` and the test file (section 8).
- [x] 4.6 Update `TODO.md`: remove the millimetre-as-pixel entry and keep the OpenGL entry's note that its
  own visibility call has the same defect. Verification: the entry is gone and the OpenGL entry names the
  defect and this change (section 9).
