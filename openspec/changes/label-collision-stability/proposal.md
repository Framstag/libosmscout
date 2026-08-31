# Proposal: Label Collision Stability

## Why

In dense label scenes (city centers), the visibility of fully visible labels changes between renders although the label content on screen is unchanged: every swap of the oversized render canvas in the plane renderer changes the render bounding box and therefore the set of label candidates that take part in collision resolution. The resulting flicker removes the reliability users need for navigation. The defect is reproduced and pinned by the diagnostic test of the change `label-layout-stability-tests` (17 of 63 labels lose visibility when border candidates are added).

## What Changes

- The label layouter resolves collisions in two groups: labels that were visible in the previous layout round claim their space first (within their group ordered by priority), newly arriving candidates are resolved afterwards.
- The previous-round visibility state is kept per layouter instance across draw calls; a per-draw reset no longer discards it.
- Labels keep the same visible state as long as the label content does not contradict it (they disappear only when leaving the viewport or when they collide with another previously visible label).
- The existing diagnostic test for candidate-set growth in the `label-layout-stability-tests` change turns from red to green.
- No public API signatures change.

## Capabilities

### New Capabilities

- `label-collision-stability`: Conflict resolution behavior of the label layouter: visibility decisions must be stable against candidate-set growth between layout rounds (last-rendered-visible labels keep their space first).

### Modified Capabilities

- `label-layout-stability-tests`: The diagnostic scenario pins the new contract and turns green once the layouter is stable; its expected outcome (defect reproduced, red) changes to stable.

## Impact

- Modified: `libosmscout-map/include/osmscoutmap/LabelLayouter.h` (layout job sorting, previous-round visibility state, documentation of the reset semantics).
- Modified: `Tests/src/LabelLayouterTest.cpp` (layout session harness for multi-round scenarios, diagnostic test driving two frames through one layouter instance).
- Behavior of all rendering backends via the shared layouter: visibility decisions gain stickiness for previously visible labels. No signature changes; no backend code changes.
- Follows `label-layout-stability-tests`, which documents the reproduced defect.