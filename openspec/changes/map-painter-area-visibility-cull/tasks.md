## 1. Baseline measurement

Parent spec: `map-painter-area-culling` - Requirement: Preparation work follows the visible areas, not
the loaded ones.

- [x] 1.1 Record the before-baseline of the area preparation step on a fixed view: build
  `Tests/PerformanceTest` in Release, render zoom 15 over `51.514 7.463 51.510 7.470` with
  `maps/Dortmund` and `stylesheets/standard.oss`, `--draw-repeat 5 --load-repeat 1`, and capture the
  step 4 `ProcessAreas` per-frame minimum and average, the prepared area and way counts, and the
  per-frame allocation count. Verification: the numbers are written into
  `openspec/changes/map-painter-area-visibility-cull/verification.md`, together with the note that
  repeated runs of the same view vary by up to 3.7x on this machine (ProcessAreas min 3.68 - 13.52 ms),
  so only within-run comparisons are used.

## 2. Stylesheet-derived tolerance bound

Parent spec: `map-painter-area-culling` - Requirement: The early rejection is conservative.

- [x] 2.1 Add the per-magnification-level maximum area border width in millimetres to `StyleConfig`,
  computed in the existing per-level postprocessing that builds the area border style lookup tables
  (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp`), and expose it with a documented accessor in
  `libosmscout-map/include/osmscoutmap/StyleConfig.h`. Verification: a unit test asserts the bound for a
  stylesheet with known border widths at two levels, and the accessor carries a doc comment.
- [x] 2.2 Confirm the bound covers every border style the per-ring decision can read: compare the
  bound against the largest per-ring offset observed over the fixed view by a temporary probe
  (`TEMP-PROBE`, reverted before the change is finished). Verification: the recorded observation is
  written into `verification.md` and is not larger than the derived bound.

## 3. The early decision in the area preparation

Parent spec: `map-painter-area-culling` - Requirement: Areas that cannot be visible are rejected before
their rings are prepared.

- [x] 3.1 Add the early visibility decision to `MapPainter::ProcessAreas`
  (`libosmscout-map/src/osmscoutmap/MapPainter.cpp`) ahead of both `PrepareArea` call sites - the
  `mapData.areas` loop and the `mapData.poiAreas` loop - using `IsVisibleArea` on the area's bounding
  box with the bound of 2.1 converted by the frame's projection, and skipping the area when it fails.
  Verification: the CMake build succeeds and the prepared area count of the fixed view stays at 726 and
  the prepared way count at 335.
- [x] 3.2 Add the debug assert of design D5 in the per-ring path, checking that the per-ring offset
  never exceeds the derived bound. Verification: builds warning-free, no assert fires in the test suite,
  and a scratch build with a deliberately lowered bound makes it fire (recorded, not committed).

## 4. New unit tests

Parent spec: `map-painter-area-culling` - all four requirements.

- [x] 4.1 Add a test that installs a counting `FillStyleProcessor` through
  `MapParameter::RegisterFillStyleProcessor` and asserts that per-ring style resolution is not reached
  for areas lying outside the view, while it is reached for an area inside the view
  (Requirement: Preparation work follows the visible areas, not the loaded ones). Verification: the test
  fails when the early decision is disabled and passes with it.
- [x] 4.2 Add a test that renders the same view with two stylesheets whose area borders differ in width
  and asserts that no ring that is styled and visible without the early decision is lost with it
  (Requirement: The early rejection is conservative). Verification: the test passes for both stylesheets
  and reports the derived bound used for each.
- [x] 4.3 Add a test that compares a frame prepared with and without the early decision: identical
  number, type, role and transformed coordinates of prepared area entries, identical prepared way count,
  identical clipping ranges for entries that have them, and identical draw order including for
  equal-comparing areas (Requirement: Prepared entries, clipping geometry, orders and rendered output are
  unchanged). Verification: the test passes, and it fails if the early decision changes what is prepared -
  checked by disabling the early decision and by a view that loads only areas outside the viewport, which
  must prepare nothing. Note: the "deliberately aggressive bound" case of the original plan is detected by
  4.2 and by the assert of 3.2, not by this test, because the areas it compares lie inside the viewport and
  are lost by both sides of the comparison together.
- [x] 4.4 Register the new test file in both builds (`Tests/CMakeLists.txt` and `Tests/meson.build`).
  Verification: the test runs under `ctest` and under `meson test`.

## 5. Verification

Parent spec: `map-painter-area-culling`; covers the regression concerns recorded in the design.

- [x] 5.1 Verify a warning-free CMake build of the map libraries and the test targets. Verification:
  clean build log with the project's warning settings, no new warning.
- [x] 5.2 Verify the Meson build of the same targets. Verification: clean build log.
- [x] 5.3 Verify the existing test suite still passes under CMake. Verification: `ctest` reports 100 %
  passed, including the map painter, area preparation, frame buffer, route, shield and text metric tests.
- [x] 5.4 Verify the existing test suite still passes under Meson. Verification: `meson test` passes.
- [x] 5.5 Measure after the change with the same fixed view and commands as 1.1, interleaved with and
  without the early decision so both are measured under the same load, and record the minimum and
  average of the step plus the prepared counts and allocations. Verification: the comparison is written
  into `verification.md` and the prepared counts are unchanged from 1.1.
- [x] 5.6 Verify the changed files against the project's linters. Verification: Uncrustify reports no
  finding on added lines and clang-tidy reports no new finding in the changed code.
- [x] 5.7 Add the findings of this work that are not part of this change to `TODO.md`: the area
  preparation early-out that no shipped stylesheet reaches (`MapPainter.cpp:1172`), and the OpenGL
  backend's per-frame reprocessing of all loaded areas in `ProcessData` including its duplicate point
  removal ahead of the visibility test. Verification: both entries present in `TODO.md` and clearly
  marked as pre-existing, not part of this change.
- [x] 5.8 Verify the change under AddressSanitizer and UndefinedBehaviorSanitizer. Verification: the
  sanitizer build and its test run pass with `PerformanceTest` excluded as in CI.
