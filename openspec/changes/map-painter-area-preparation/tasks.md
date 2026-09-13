## 1. Baseline measurement

Parent spec: `map-painter-area-preparation` - Requirements: Area preparation does not
allocate per loaded area; Prepared areas, draw order and rendered output are unchanged.

- [x] 1.1 Record a before-change baseline on fixed views: build `PerformanceTest`
  (`Tests/src/PerformanceTest.cpp`, per-step draw loop at `:1087-1102`, per-step report at
  `:1175-1190`) and render the Dortmund database at z14-z17 with `--draw-repeat 3`, both
  with the wide tile box (z15, 304 tiles) and the small tile box, capturing per-step time
  and the per-step allocation counts (`#4 ProcessAreas` in particular), plus the prepared
  area count for one fixed view. Verification: the baseline numbers are written into
  `openspec/changes/map-painter-area-preparation/verification.md`.

## 2. Reuse the per-area and per-ring scratch storage

Parent spec: `map-painter-area-preparation` - Requirement: Area preparation does not
allocate per loaded area.

- [x] 2.1 Add the reused per-ring coordinate range store as painter state and use it
  instead of the per-area vector at `MapPainter.cpp:1224`, keeping the "invalid range for
  skipped rings" initialisation of the reused store (`MapPainter.cpp:1228-1233`).
  Verification: `libosmscout-map` builds without warnings and the requirement scenario
  "Repeated rendering of a loaded view stays within a constant allocation bound" holds in
  the allocation counter of `Tests/src/PerformanceTest.cpp`.
- [x] 2.2 Add the reused border style store as painter state and use it instead of the
  per-ring vector at `MapPainter.cpp:1110`, relying on `GetAreaBorderStyles` clearing its
  output (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:1386`). Verification: build
  without warnings plus the same allocation bound as 2.1.
- [x] 2.3 Verify the reused stores cannot leak state between rings and areas: confirm
  every read site re-checks validity (`.IsValid()`/`IsValid()`, `MapPainter.cpp:1105`,
  `:1161`) and that no site keeps a reference to a store across a call. Verification: code
  review recorded in `verification.md`, plus `MapPainterShieldTest` and
  `MapPainterRouteTest` passing.

## 3. Non-allocating ring visitor

Parent spec: `map-painter-area-preparation` - Requirement: Area preparation does not
allocate per loaded area.

- [x] 3.1 Replace the seven-capture visitor built at `MapPainter.cpp:1266` with a visitor
  built from a single pointer to a local context struct, so the closure fits the
  `std::function` small buffer and `Area::VisitRings` (`libosmscout/src/osmscout/Area.cpp:608`)
  stops allocating per area. Verification: the per-area allocation contribution disappears
  in the `#4 ProcessAreas` allocation counter at the baseline view.
- [x] 3.2 Keep the visitor's return value semantics unchanged: `true` for ignore-typed
  rings and for accepted rings, `false` otherwise (`Area.cpp:627` uses it as the descend
  signal, `MapPainter.cpp:1099-1103`, `:1153`, `:1215`). Verification: a view with nested
  inner rings outside the viewport still prepares the same entries as the 1.1 baseline.
- [x] 3.3 If the context-pointer visitor still allocates, fall back to the templated
  visitor overload (design D2 option 1) and record the decision. Verification: the
  allocation counter of `#4 ProcessAreas` shows no per-area allocation.

## 4. Decide styling and visibility before transforming

Parent spec: `map-painter-area-preparation` - Requirements: Styling and visibility are
decided before geometry is transformed; Clipping rings keep valid geometry.

- [x] 4.1 Restructure the ring preparation so the fill/border style resolution and
  `IsVisibleArea(projection, ring.GetBoundingBox(), borderWidth/2)` (`MapPainter.cpp:1150`)
  run before `TransformArea`, and only accepted rings are transformed
  (`MapPainter.cpp:1226-1264` becomes the accepted-ring path). Verification:
  `MapPainterShieldTest`, `MapPainterRouteTest` and the SVG drawing tests pass unchanged,
  and the 1.1 baseline view prepares the same area count.
- [x] 4.2 Keep ignore-typed rings transformed unconditionally so clipping ranges stay
  valid (`MapPainter.cpp:1099-1103`, clipping collection at `:1160-1166`), and document
  why at the site. Verification: the clipping scenario "Holes of a drawn area are still
  clipped" passes and rendering an area with holes is unchanged against the 1.1 baseline.
- [x] 4.3 Keep the emission order of prepared entries identical to the current ring visit
  order (`MapPainter.cpp:1177`, `:1212`) so the stable sort input is unchanged.
  Verification: the draw order comparison test of 5.4 shows no difference.
- [x] 4.4 Verify that the accepted set is unchanged for the baseline views: prepare one
  fixed view before and after the change and compare per-entry type, ring role and
  bounding box. Verification: comparison recorded in `verification.md` with no
  differences.

## 5. New unit tests

Parent spec: `map-painter-area-preparation`.

- [x] 5.1 Add a test that renders one fixed view twice and asserts the `ProcessAreas`
  step performs at most a small constant number of heap allocations independent of the
  loaded area count (Requirement: Area preparation does not allocate per loaded area).
  Verification: the test passes and fails against the pre-change code.
- [x] 5.2 Add a test with a loaded area whose rings are outside the viewport and one whose
  rings resolve no fill and no border style, asserting neither contributes transformed
  coordinates for the frame (Requirement: Styling and visibility are decided before
  geometry is transformed). Verification: the test fails when the transform runs before
  the decision.
- [x] 5.3 Add a test with a styled and visible ring asserting the coordinates of its
  prepared range are identical to the coordinates the pre-change preparation produced
  (compare coordinates, not the buffer indices, which are frame-local) (Requirement:
  Styling and visibility are decided before geometry is transformed - Scenario: Styled
  and visible rings are transformed as before). Verification: the recorded coordinates
  match the 1.1 baseline.
- [x] 5.4 Add a test asserting the prepared area count and the prepared area order are
  unchanged for a view with several areas including equal-comparing ones (Requirement:
  Prepared areas, draw order and rendered output are unchanged). Verification: the test
  compares against the order recorded in the 1.1 baseline.
- [x] 5.5 Register the new test file in both builds (`Tests/CMakeLists.txt` and
  `Tests/meson.build`) and verify it runs under `ctest` and under `meson test`.

## 6. Verification

Parent spec: `map-painter-area-preparation`.

- [x] 6.1 Verify a warning-free CMake build of `libosmscout-map`, the map backends and the
  tests with the project's warning settings. Verification: clean build log.
- [x] 6.2 Verify the Meson build of the same targets. Verification: clean build log.
- [x] 6.3 Run the full test suite and confirm no pre-existing test regresses, in
  particular the `MapPainter*Test` suite, `MapPainterFrameBuffersTest`, the SVG/Cairo/Qt/
  Skia drawing tests and `PerformanceTest`. Verification: `ctest -j 2 --output-on-failure`
  green with `QT_QPA_PLATFORM=offscreen`.
- [x] 6.4 Run Uncrustify against `.uncrustify` and clang-tidy against `.clang-tidy` on the
  changed files. Verification: no new findings.
- [x] 6.5 Re-run the 1.1 measurement on the same fixed views and record per-step time and
  allocation counts against the baseline, including z14-z17 to show no regression at the
  label-dominated zooms. Verification: `#4 ProcessAreas` shows the constant allocation
  bound and a lower wall time at z15; the numbers are written into `verification.md`.
- [x] 6.6 Update `TODO.md`: close the `MapPainter::ProcessAreas` scratch allocation finding
  and keep the remaining findings (label pipeline, font cache, way paths, data loading)
  open. Verification: the entry reflects the current code and mentions this change.
