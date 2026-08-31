# Tasks: Label Layout Stability Tests

Parent spec: `specs/label-layout-stability-tests/spec.md` (capability `label-layout-stability-tests`).

## 1. Test infrastructure

- [x] 1.1 Create `Tests/src/LabelLayouterTest.cpp` with `FakeTextLayouter` (mock text layouter implementing the `LabelLayouter` contract from design.md D1: fixed deterministic width/height per text, no Qt/fonts) and supporting `FakeGlyph`/`FakeLabel` types. Verify: file compiles standalone together with existing test headers.
- [x] 1.2 Implement the test harness that builds a synthetic label scene (labels at fixed pixel positions with priorities), constructs an initialized `MercatorProjection` + `MapParameter`, sets up `LabelLayouter` with a viewport (parent spec: "Backend-independent label layout execution", scenario "Layout runs without a rendering backend"), and exposes result inspection (visibility + top-left position + width/height) from `Labels()`. Verify: helper compiles, single dummy scenario runs.
- [x] 1.3 Register the new test program in `Tests/CMakeLists.txt` and `Tests/meson.build` following existing test registration patterns. Verify: `cmake -B build && cmake --build build` and `meson setup debug && meson compile -C debug` both succeed without errors or warnings.

## 2. Stability scenarios (parent spec requirements)

- [x] 2.1 Determinism test: lay out a competing label set twice with the identical viewport, assert identical visible set and positions (parent spec: "Deterministic layout for identical inputs"). Verify: `ctest -R LabelLayouterTest` executes the case.
- [x] 2.2 Pan-shift invariance tests: same label set with viewport origin (0,0) vs (50,0) and vs (0,50); assert labels fully inside both viewports keep visibility and shift by exactly the origin delta (parent spec: "Stability of fully visible labels under panning"). Verify: test case green, or documents the defect with per-label visibility report.
- [x] 2.3 Enlarged-viewport stability test: same label set inside 800x600 with no labels outside, re-layout with 1200x900 canvas centered on the same point; assert unchanged visibility and positions within the original viewport (parent spec: "Stability under enlarged layout viewport"). Verify: test case green, or documents the defect.
- [x] 2.4 Collision-winner stability test: two competing labels of distinct adjacent priorities offset by 50 px pan; assert the same label stays visible in both runs (parent spec: "Stability of competing labels under viewport shift"). Verify: test case green, or documents the defect with winner report.
- [x] 2.5 Dense-scene diagnostic: extended candidate set of border labels overlapping a dense label grid; assert labels visible in the smaller candidate set remain visible (parent spec: "Stability under changed layout candidate set"). Verify: test case green, or documents the defect with the number of vanished labels. (Outcome: 17/63 labels vanish — defect reproduced.)

## 3. Verification of whole change

- [x] 3.1 Build CMake `build/` and Meson `debug/` without errors or warnings (per operations guidance). Verify: clean build output in both systems.
- [x] 3.2 Run existing test suite subset around map painting/layout (`ctest -R "LabelLayouterTest|ScreenMaskTest"`) and confirm no regressions in registered tests. Verify: all green.
- [x] 3.3 Record observed outcome of the stability scenarios (stable vs. defect reproduced) in the change notes for later reference by the follow-up fix. Verify: artifact file with result summary exists in the change directory.