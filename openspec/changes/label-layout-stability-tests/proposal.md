# Proposal: Label Layout Stability Tests

## Why

When panning the visible map area (for example in OSMScout2 with the plane renderer), label visibility flickers: labels that are visible before the pan and again after the pan can disappear during the pan. The label layout result appears to depend on the bounding box of the render request, not only on the map content that is actually visible. There is currently no test that exercises the label layouter directly, so viewport-dependent instability cannot be reproduced, demonstrated, or guarded against by regression tests.

## What Changes

- Add unit tests that drive the label layouter directly with synthetic label data and in-process font metrics, without invoking any drawing backend.
- The tests check the layout result (which labels are visible, at which positions) for the same fixed set of labels under different layout viewports:
  - identical viewports (baseline determinism),
  - horizontally/vertically shifted viewports (simulated panning),
  - an enlarged layout viewport around an unchanged visible center (simulated oversized render canvases),
  - sub-pixel shifted viewports (simulated sliding positions with fractional pixel coordinates).
- The tests are accepted whether they demonstrate the instability or confirm stability; they remain as regression coverage for future layouter work.
- No library code is modified by this change.

## Capabilities

### New Capabilities

- `label-layout-stability-tests`: Executable specification that label layout results are verifiable without a rendering backend, and defines the required stability properties (determinism, shift invariance for fully visible labels) that the layouter must expose when the same labels are laid out with different viewports.

### Modified Capabilities

- None.

## Impact

- New file: `Tests/src/LabelLayouterTest.cpp` (Catch2 test program).
- Modified: `Tests/CMakeLists.txt` and `Tests/meson.build` (test registration in both build systems).
- Depends on (read-only, no modification): `libosmscout-map/include/osmscoutmap/LabelLayouter.h` (`LabelLayouter`, `LabelData`, `LabelInstance`, `LabelPriority`), `libosmscout-map/include/osmscoutmap/LabelPath.h`, `libosmscout/include/osmscout/projection` (`MercatorProjection`) and `MapParameter`.
- No changes to public library API, no changes to any map rendering backend, no changes to the Qt client rendering pipeline.