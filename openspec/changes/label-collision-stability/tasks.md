# Tasks: Label Collision Stability

Parent spec: `specs/label-collision-stability/spec.md` (capability `label-collision-stability`).

## 1. Layouter implementation

- [x] 1.1 Add previous-round visibility state (`lastVisibleRefs`) to `LabelLayouter`, refresh it at the end of `Layout()`, and keep it across `Reset()` (parent spec: "Previous visibility state survives the per-draw reset"). Verify: file compiles.
- [x] 1.2 Implement two-group sorting in `LayoutJob::SortLabels()` via a comparator that orders previously visible refs before new candidates, priority inside each group (parent spec: "Previous visibility precedence in collision resolution"). Verify: existing LabelLayouterTest still compiles and passes for fresh-instance scenarios.

## 2. Test updates (existing test program from label-layout-stability-tests)

- [x] 2.1 Add a `LayoutSession` harness (one layouter instance, multiple `Frame()` calls) to `Tests/src/LabelLayouterTest.cpp` mirroring the renderer sequence Layout -> Reset -> next frame. Verify: harness compiles, dummy frame runs.
- [x] 2.2 Session scenarios: previously visible label precedes new overlapping candidate; state survives reset; unregistered labels lose state (parent spec requirements 1 and 2). Verify: test cases execute.
- [x] 2.3 Session determinism scenarios: identical rounds, and pan between rounds (parent spec: "Deterministic and translation-invariant sticky resolution"). Verify: test cases green.
- [x] 2.4 Update the dense-scene diagnostic test of `label-layout-stability-tests` to run its two layouts as two frames of one session. Verify: diagnostic turns green (was red with 17/63 vanished labels).

## 3. Verification of the whole change

- [x] 3.1 Build CMake `build/` and Meson `debug/` without errors and without new warnings in changed files. Verify: clean build output in both systems.
- [x] 3.2 Run test subset `ctest -R "LabelLayouterTest|ScreenMaskTest"` plus Meson equivalents; all green (parent spec: all scenarios). Verify: ctest and meson test outputs.
- [x] 3.3 Record outcome in a result summary artifact in this change directory (parent spec traceability and archive guidance). Verify: artifact exists.
- [x] 3.4 Add gated diagnostics logging (OSMSCOUT_DEBUG_LABEL_HYSTERESIS) for layout rounds and renderer swaps to collect real-world evidence. Verify: log lines appear with the env var set.
- [x] 3.5 Renderer-side stabilization: PlaneMapRenderer skips renders from partially loaded tile sets while a finished render exists (parent spec: "Rendering uses complete tile data"). Verify: OSMScout2 target compiles; manual retest shows no notRegistered churn spikes.
- [x] 3.6 Path label hysteresis: previously visible contour labels claim their space before new candidates, with the same 2px jitter tolerance (parent spec: "Previous visibility precedence in collision resolution"). Verify: label suite green.