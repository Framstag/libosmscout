# Result Summary

## Observed outcome of the stability scenarios

Dates: 2026-08-29 (initial), 2026-08-29 (dense-scene diagnostic added)

GCC 13 / Linux, CMake (`build/`) and Meson (`debug/`).

### Sparse scenes — all stability properties hold

| Scenario                                   | Result                                        |
|--------------------------------------------|-----------------------------------------------|
| Backend-independent execution + result query | PASS                                        |
| Determinism (identical viewport, competing labels) | PASS                                  |
| Horizontal pan (50 px), fully visible labels | PASS (visibility + exact position shift)    |
| Vertical pan (50 px)                        | PASS                                          |
| Sub-pixel pan (0.5, 0.25 px)                | PASS                                          |
| Enlarged viewport (800x600 -> 1200x900), same visible center, no content outside | PASS      |
| Collision winner with distinct priorities under pan | PASS                                 |

### Dense scene diagnostic ("Dortmund center" case) — DEFECT REPRODUCED

Test case: "dense scene visibility is stable when enlarged viewport adds
off-viewport competitors (diagnostic)".

Scene: 9x7 grid of 120px-wide "ShopNN" labels (63 labels), horizontal
spacing 70 px -> heavy horizontal overlap, 5 priority classes. Baseline
layout: 800x600 viewport over the grid alone. Second run: 1600x1200
viewport, same center, plus 60 additional labels in the border ring, 14
of which are border-column labels that reach ~60 px into the visible
region and overlap the first/last grid column.

Result: **17 of 63 labels (27%) lose their visibility** in the second run
alone by adding border candidates, e.g. `Shop01` (ref Node 1) is visible
in the baseline run and invisible in the extended run. The failing test
reports `Labels vanished due to additional border content: 17`.

This reproduces the observed OSMScout2 behavior in Dortmund center: while
panning, every swap of the oversized render canvas changes the render
request bounding box and with it the set of label candidates that
participate in collision resolution. Labels visible before and after a
pan disappear when the candidate set of the intermediate renders differs,
and reappear later.

## Interpretation

- The label layouter itself is deterministic and translation-invariant for
  fixed content (all pan/enlargement without candidate change tests pass).
- The flicker mechanism lives in the **candidate-set dependence of the
  greedy collision resolution**: the winner set changes whenever the set
  of registered labels changes, even if the newly added labels overlap
  only the outermost visible labels and the label content inside the
  visible region is otherwise identical. Real renders make this worse,
  because every image swap has a different stale bounding box (1.5x
  canvasOverrun), progressively loaded tile data, and derived parameters
  (e.g. `SetLabelLineFitToWidth(width/1.5)`).
- Fix direction for a follow-up change: make collision decisions stable
  against candidate-set growth (e.g. carry previous decisions as
  constraints, or resolve competitions anchored to map objects instead
  of per-render canvas runs).

The stability requirement `Stability under changed layout candidate set`
documents the desired contract; the diagnostic test currently fails
against it and pins the defect until a follow-up fix change resolves it.

## Follow-up resolution note

2026-08-29: the diagnostic was switched to a multi-frame session harness
and now passes: the change `label-collision-stability` implemented
visible-label hysteresis in `LabelLayouter` (`lastVisibleRefs`, two-group
collision resolution). All 11 test cases pass on both build systems; see
`openspec/changes/label-collision-stability/tests-result.md`.

## Build warnings (pre-existing, unrelated to this change)

- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:464` unused variable `lineOffset`
- `libosmscout-client-qt/src/osmscoutclientqt/MapWidget.cpp` several deprecated Qt touch/hover APIs and enum mismatches

New code in this change builds warning-free on both build systems.