## 1. Baseline measurement

Parent spec: `map-painter-frame-buffers` - Requirement: Prepared frame data is reused
across frames.

- [x] 1.1 Record a before-change baseline on a fixed view: build `PerformanceTest`
  (`Tests/src/PerformanceTest.cpp`, draw-repeat loop at `:953-960`) with the gperftools
  hooks enabled (`PERF_TEST_GPERFTOOLS_USAGE`, `:55`), render a fixed zoom/tile set
  with draw-repeat >= 3, and capture per-step times plus the total allocation count.
  Verification: the baseline numbers are written into
  `openspec/changes/map-painter-frame-containers/verification.md`.

## 2. Storage conversion

Parent spec: `map-painter-frame-buffers`.

- [x] 2.1 Convert prepared area storage to a reusable contiguous store and keep every
  fill/sort/draw site working (`MapPainter.h:292`, `MapPainter.cpp:1177`, `:1212`,
  `:1285`, `:2461`, `:2563`, `:2591`, `:2618`). Verification: CMake build of
  `libosmscout-map` succeeds without warnings and `MapPainterShieldTest` passes.
- [x] 2.2 Convert prepared way storage the same way
  (`MapPainter.h:293`, `MapPainter.cpp:1053`, `:1085`, `:1632`, `:1654`, `:1660`,
  `:1669`, `:1841`, `:2364`, `:2481`). Verification: build succeeds without warnings
  and `MapPainterRouteTest.cpp:182` still renders successfully.
- [x] 2.3 Convert prepared way path storage and replace the route label way-path
  reference with an index (`MapPainter.h:248`, `:255`, `MapPainter.cpp:1591`, `:1670`,
  `:1784-1786`, `:1987-1994`, `:2506`, `:2540`, `:2665`). Verification: build succeeds
  without warnings and `MapPainterRouteTest` plus the route label scenarios pass.
- [x] 2.4 Replace the node-based ordering of prepared areas and ways with a
  stability-preserving sort of the contiguous stores, keeping `AreaSorter`
  (`MapPainter.cpp:104-144`, `:2096-2097`) unchanged. Verification: the equal-comparing
  area ordering test from 3.3 passes and the existing rendering tests show no output
  change.
- [x] 2.5 Update the prepared-area and prepared-way accessors to the new storage and
  update the only in-repo reader, the SVG backend
  (`MapPainter.h:627-635`, `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp:878`,
  `:885`), including their doc comments. Verification: `libosmscout-map-svg` builds
  without warnings and the SVG drawing tests pass.
- [x] 2.6 Review and keep the appends that happen after ordering (`DrawGroundTiles`
  `MapPainter.cpp:2364`, `DrawOSMTileGrid` `:1053`, `:1085`) as unsorted tail appends,
  confirming no element address is held across them. Verification: a comment-free code
  review recorded in `verification.md` plus the full frame tests from 4.3 passing.

## 3. New unit tests

Parent spec: `map-painter-frame-buffers`.

- [x] 3.1 Add `Tests/src/MapPainterFrameBuffersTest.cpp` with a test that renders the
  same view twice and asserts the prepared stores keep their capacity instead of being
  empty at the start of the second frame (Requirement: Prepared frame data is reused
  across frames). Verification: the new test passes and fails when the store is
  re-created per frame.
- [x] 3.2 Add a test asserting consecutive prepared areas and ways are adjacent in
  their storage (Requirement: Prepared frame data is contiguous per object kind).
  Verification: the test passes and reports each element's address.
- [x] 3.3 Add a test with two equal-comparing areas (identical bounding box, same
  outer/inner role) asserting they keep their preparation order, and that repeated
  frames order identically (Requirement: Draw order is stable for prepared areas and
  ways). Verification: the test passes with the stability-preserving sort and fails
  with an unstable sort.
- [x] 3.4 Add a test where a route label's prepared way path is resolved after further
  prepared data is appended and the stores are ordered, asserting it resolves to the
  same way path (Requirement: Route labels resolve their prepared way path).
  Verification: the test fails when the reference is an iterator into a growing store.
- [x] 3.5 Add a test through a backend callback enumerating prepared areas and ways,
  asserting the count matches the drawn objects and the enumeration order matches the
  draw order (Requirement: Backends retain read access to prepared areas and ways).
  Verification: the test passes for the SVG backends path used by
  `MapPainterSVG::AfterPreprocessingCallback`.
- [x] 3.6 Register the new test file in both builds (`Tests/CMakeLists.txt` and
  `Tests/meson.build`) and verify it runs under `ctest` and under `meson test`.

## 4. Verification

Parent spec: `map-painter-frame-buffers`; covers the regression concerns recorded in
the proposal (area clipping, label placement, shield labels).

- [x] 4.1 Verify a warning-free CMake build of the map libraries and the tests with the
  project's warning settings. Verification: clean build log.
- [x] 4.2 Verify the Meson build of the same targets. Verification: clean build log.
- [x] 4.3 Run the full test suite and confirm no pre-existing test regresses, in
  particular `MapPainterRouteTest`, `MapPainterShieldTest`, `MapPainterShieldQtTest`,
  `LaneEvaluationCompare` junction rendering, and the SVG/AGG/Cairo/Skia drawing tests.
  Verification: `ctest -j 2 --output-on-failure` green, with Qt tests run under
  `QT_QPA_PLATFORM=offscreen`.
- [x] 4.4 Run Uncrustify against `.uncrustify` and clang-tidy against `.clang-tidy` on
  the changed files. Verification: no new findings.
- [x] 4.5 Re-run the measurement from 1.1 with the same fixed view and record the
  per-step and allocation-count difference against the baseline in `verification.md`.
  Verification: the recorded numbers show the per-frame allocation count no longer
  grows with the prepared object count.
- [x] 4.6 Update `TODO.md`: close the `MapPainter` prepared-data finding and record the
  `AreaData::clippings` conversion (`MapPainter.h:245`, read by GDI, AGG, Cairo, Skia,
  DirectX, SVG and the Android JNI painter) as a follow-up. Verification: the entry
  reflects the current code.
