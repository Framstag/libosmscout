## 1. Baseline and attribution

Parent spec: all of `map-painter-label-reuse` - the numbers and the reference dump produced here
are what the "unchanged" and "reused" requirements are measured against.

- [x] 1.1 Record a before-change baseline: run `build/Tests/PerformanceTest` with
  `--driver cairo` on the fixed view (`maps/Dortmund stylesheets/standard.oss
  51.514 7.463 51.510 7.470`, zoom 15, `--draw-repeat 5`, one tile) and on the z16 band view
  (`51.45 7.35 51.46 7.37`, `--draw-repeat 3`, 16 tiles), capturing per-step times and per-step
  allocation counts. Write both tables into
  `openspec/changes/map-painter-label-reuse/verification.md`. Verification: the file contains
  the per-step allocation and time rows for both views, and the label steps add up to the
  expected share of the frame totals.
- [x] 1.2 Attribute the label steps' allocations to their sites (label measurement, per-label
  mask/canvas/element vectors, frame canvases, path label geometry, glyph derivation) with a
  temporary probe in the label stage, reverted before this change is finished. Verification:
  `verification.md` records an attribution table whose rows sum to the measured step counts.
- [x] 1.3 Record the reference label placement of the fixed view: dump every label that takes
  part in the frame (position, size, text, style, draw index) with the temporary probe of 1.2
  into `openspec/changes/map-painter-label-reuse/baseline-labels.txt`. Verification: the dump
  has one line per drawn label, in draw order.

## 2. Measurement reuse

Parent spec: `map-painter-label-reuse` - Requirement: Label measurement is reused while its
inputs are unchanged.

- [x] 2.1 Add the measurement cache to `LabelLayouter` (measurement key from the `Layout()`
  arguments, environment comparison, lookup before measuring) and consult it from `ProcessLabel`
  and `RegisterContourLabel` (`libosmscout-map/include/osmscoutmap/LabelLayouter.h:680`, `:782`).
  Verification: `libosmscout-map` builds without warnings and the layouter unit test of 7.1
  shows one measurement for two frames of the same label.
- [x] 2.2 Add the measurement environment of the Cairo backend: derive it from the font name,
  the DPI, the magnification and the drawing target's font options, refresh it in
  `MapPainterCairo::DrawMap` (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp:1380`)
  and discard the cached measurements when it changes. Verification: the backend-level test of
  7.4 measures the same text at two DPI values and observes two measurements, and
  `TextMetricsCairoTest` still passes.
- [x] 2.3 Add the measurement environment to the remaining backends that implement `Layout()`
  (Qt, Skia, SVG, AGG, GDI, DirectX), each from its own measurement inputs. Verification: every
  backend builds without warnings and `TextMetricsCrossBackendTest` plus the per-backend text
  metric tests pass unchanged.
- [x] 2.4 Bound the measurement cache and clear it when the environment changes, using
  `osmscout::Cache` (`libosmscout/include/osmscout/util/Cache.h:57`) or an equivalent bounded
  store with the same default size. Verification: the unit test of 7.5 exceeds the bound and
  observes eviction with a bounded entry count.

## 3. Glyph reuse

Parent spec: `map-painter-label-reuse` - Requirement: Per-glyph data is reused.

- [x] 3.1 Add the glyph table to `LabelLayouter` keyed by the cached label and route the
  contour label path through it (`LabelLayouter.h:802`). Verification: the unit test of 7.2
  observes zero glyph derivations for the second frame and glyph data equal to a fresh
  derivation.
- [x] 3.2 Clear the glyph table exactly with the measurement cache (same discard points as
  2.1/2.4). Verification: the unit test of 7.2 changes the environment between frames and
  observes a fresh derivation instead of a stale entry.

## 4. Frame-wide label state reuse

Parent spec: `map-painter-label-reuse` - Requirement: Label stage scratch storage is reused.

- [x] 4.1 Give `ScreenMask` a reset that zeroes its bitmask in place and resizes it only when
  the layout viewport changed (`libosmscout-map/src/osmscoutmap/LabelLayouterHelper.cpp:91`).
  Verification: new cases in `Tests/src/ScreenMaskTest.cpp` cover reset, reset with a changed
  width and reuse without reallocation.
- [x] 4.2 Turn `LayoutJob` (`LabelLayouter.h:310-344`) into reusable state of the layouter:
  canvases reset in place, ordered stores cleared and swapped as today, paddings recomputed per
  frame, copy/move still deleted. Verification: the placement test of 7.3 renders two frames
  with different label sets and both place their labels as a fresh painter would.

## 5. Per-object and per-label scratch reuse

Parent spec: `map-painter-label-reuse` - Requirement: Label stage scratch storage is reused.

- [x] 5.1 Give `ScreenRectMask` a reset that reuses its bitmask
  (`libosmscout-map/src/osmscoutmap/LabelLayouterHelper.cpp:29`). Verification: new cases in
  `Tests/src/ScreenMaskTest.cpp` assert the same cells before and after a reset and that no
  allocation happens on the second use.
- [x] 5.2 Reuse the per-label mask, canvas and visible-element vectors in
  `ProcessLabelInstance` (`LabelLayouter.h:399`, `:400`, `:403`) and the mask vector of
  `ProcessLabelContourLabel` (`:487`) as cleared layouter members. Verification: the allocation
  tests of 7.3 show the label stage's allocations no longer growing with the number of
  registered labels that do not take part in the frame.
- [x] 5.3 Reuse the per-object label list of `MapPainter::LayoutPointLabels`
  (`libosmscout-map/src/osmscoutmap/MapPainter.cpp:391`) as a cleared painter member.
  Verification: build without warnings and the route/shield label tests pass unchanged.
- [x] 5.4 Give `LabelPath` a reset and reuse the path geometry of a path label in
  `DrawWayContourLabel` (`MapPainter.cpp:982`) and in the route label path. Verification: a new
  `Tests/src/LabelPathTest.cpp` case covers reset and reuse, and the route label scenarios of
  `MapPainterRouteTest` pass unchanged.
- [x] 5.5 Replace the per-way `std::set<GeoCoord>` of `GetGridPoints` (`MapPainter.cpp:44`)
  with a reused sorted unique buffer. Verification: the shield label tests
  (`MapPainterShieldTest`, `MapPainterShieldQtTest`) pass and the placement dump of 8.6 is
  unchanged.

## 6. Backend environment review

Parent spec: `map-painter-label-reuse` - Requirement: Label measurement is reused while its
inputs are unchanged (the "changed drawing parameter" scenario).

- [x] 6.1 Review every backend's `Layout()` for inputs that are not part of the layouter key
  and not covered by its environment value, and record the review in `verification.md`.
  Verification: each backend has a named environment source, or a note explaining why an empty
  environment is correct for it.
- [x] 6.2 Record the pre-existing `MapPainterCairo::fonts` cache (`MapPainterCairo.cpp:309-320`,
  keyed by effective font size only, never invalidated on a font name change) in `TODO.md`.
  Verification: the entry names the file, the line and the reason it is out of scope here.

## 7. New unit tests

Parent spec: `map-painter-label-reuse` - all requirements.

- [x] 7.1 Add `Tests/src/MapPainterLabelReuseTest.cpp`: instantiate `LabelLayouter` with a
  counting fake text layouter and cover "repeated frame measures nothing", "changed font size
  is measured again", "reuse independent of the label having been drawn", "more labels do not
  raise the measurement count of a repeated frame", and "reused measurement equals a fresh
  measurement". Verification: the test passes and fails against the pre-change library.
- [x] 7.2 Extend the same file with the glyph scenarios: "glyphs of a label drawn twice are
  derived once", "reused glyphs equal fresh glyphs", "a path label reuses glyphs", "an
  environment change drops stale glyphs". Verification: the test passes and each scenario fails
  when the glyph table is bypassed.
- [x] 7.3 Add the allocation scenarios with a counting `operator new` as in
  `Tests/src/MapPainterAreaPreparationTest.cpp`: the label stage's allocations of a second
  frame are constant with respect to registered-but-undrawn labels and do not grow with the
  frame number. Verification: the test passes and fails against the pre-change library.
- [x] 7.4 Add the backend environment test: a Cairo painter measures the same text before and
  after a DPI change and performs two measurements, and performs one measurement across two
  frames with an unchanged environment. Verification: the test passes and fails when the
  environment is not refreshed.
- [x] 7.5 Add the cache bound test: exceeding the bound evicts and keeps the entry count at the
  bound. Verification: the test passes and reports the entry count.
- [x] 7.6 Register the new test file in `Tests/CMakeLists.txt` and `Tests/meson.build`.
  Verification: the test runs under `ctest` and under `meson test`.

## 8. Verification

Parent spec: all of `map-painter-label-reuse`.

- [x] 8.1 Verify a warning-free CMake build of the map libraries, the backends touched in 2.3
  and the tests. Verification: clean build log.
- [x] 8.2 Verify the Meson build of the same targets. Verification: clean build log.
- [x] 8.3 Run the full test suite and confirm no regression, in particular
  `TextMetricsCairoTest`, `TextMetricsQtTest`, `TextMetricsSVGTest`,
  `TextMetricsCrossBackendTest`, `MapPainterRouteTest`, `MapPainterShieldTest`,
  `MapPainterShieldQtTest`, `ScreenMaskTest`, `LabelPathTest`, `MapPainterFrameBuffersTest` and
  `PerformanceTest`. Verification: `ctest -j 2 --output-on-failure` green with Qt tests under
  `QT_QPA_PLATFORM=offscreen`.
- [x] 8.4 Run Uncrustify against `.uncrustify` and clang-tidy against `.clang-tidy` on the
  changed files. Verification: no new findings; pre-existing drift does not grow.
- [x] 8.5 Re-run the measurements of 1.1 with the same views and record the per-step difference
  against the baseline in `verification.md`. Verification: the label step allocation counts and
  times of the repeated frames are recorded next to the baseline numbers.
- [x] 8.6 Compare the label placement dump of the fixed view against `baseline-labels.txt` and
  the rendered output against the before-change rendering. Verification: `diff` reports no
  difference in the label set, order or placement.
- [x] 8.7 Update `TODO.md`: close the label scratch-vector finding, record the remaining
  `DrawLabels` draw-path share as the symbol/text raster follow-up, and keep the `fonts` entry
  of 6.2. Verification: the entries reflect the current code and the measured numbers.

## 9. Review fixes (PR review of 2026-09-16)

The review of the pull request found three defects in the reuse introduced by this change. The
following tasks fix them and extend the scenarios that cover them.

Parent spec: "Label measurement is reused while its inputs are unchanged", "Per-glyph data is
reused" and "The measurement cache is bounded, drops the least recently used measurement, and
can be switched off".

- [x] 9.1 Carry the line wrapping parameters (`LabelLineMinCharCount`, `LabelLineMaxCharCount`,
  `LabelLineFitToArea`, `LabelLineFitToWidth`) in `LabelMeasurementKey`, because the backends
  that wrap read them inside `Layout()` through `MapPainter::GetProposedLabelWidth`. Verification:
  the new scenario "a changed line wrapping parameter is measured again" fails when the key does
  not carry them.
- [x] 9.2 Make the glyph data a member of the measurement entry (`LabelMeasurement::glyphs` with
  its derivation flag) and drop the second, pointer-keyed table, so that glyph data cannot
  outlive the label it was derived from. Verification: the new scenarios "a bound of 0 remembers
  neither measurements nor glyph data" and "glyph data does not outlive the measurement it
  belongs to" pass, and the frame's contour labels equal those of a fresh layouter.
- [x] 9.3 Drop the least recently used measurement instead of the oldest measured one: keep the
  order of use in a `std::list` whose position each entry knows, and splice an entry to the back
  when it is used. Verification: the new scenario "a measurement that is used again outlives one
  that is not" fails when the entry used again is not moved to the end of the order.
- [x] 9.4 Let a bound of 0 mean what it documents: remember no measurement, hold the measurement
  of the current step in one slot, and empty that slot in `Reset()` at the end of the frame.
  Verification: the new "bound of 0" scenario asserts one measurement and one glyph derivation
  per frame and no remembered glyph data.
- [x] 9.5 Build the changed files without warnings, run the full test suite, and run Uncrustify
  and clang-tidy on them. Verification: `cmake --build`, `ctest` (118 tests), `uncrustify` drift
  of `LabelLayouter.h` at or below its pre-fix value and no drift in the test file, clang-tidy
  without a new finding category. Evidence in `verification.md`, section 14.
- [x] 9.6 Record the review outcome and the fix in `verification.md`. Verification: the review
  comments of the pull request are answered by a scenario, a task and a piece of evidence.
