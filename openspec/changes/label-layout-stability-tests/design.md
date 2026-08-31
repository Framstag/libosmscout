# Design: Label Layout Stability Tests

## Context

See proposal.md and specs/label-layout-stability-tests/spec.md.

Relevant existing code:

- `libosmscout-map/include/osmscoutmap/LabelLayouter.h` — class template `LabelLayouter<NativeGlyph, NativeLabel, TextLayouter>`; drives registration (`RegisterLabel`), collision resolution (`Layout()` → `LayoutJob::ProcessLabels`), result access (`Labels()`, `ContourLabels()`), and drawing (`DrawLabels<Painter>`, not needed here).
- `libosmscout-map/include/osmscoutmap/LabelLayouterHelper.h` — `ScreenMask`/`ScreenRectMask` primitives; collision marking (already unit-tested in `Tests/src/ScreenMaskTest.cpp`, but only at pixel-mask level, not at label level).
- Label registration needs a `Projection` and `MapParameter` for the text layouter call chain; positions are given directly in pixel coordinates (`Vertex2D`).

Constraint: the test must not instantiate Qt, Cairo, Skia or any other backend, and must not depend on font files or icon directories.

## Goals / Non-Goals

**Goals:**

- Execute the label layouter end-to-end from label registration to the final visibility/position result, without a rendering backend.
- Reproducible synthetic label scenes that expose viewport-dependent instability (pan shift, enlarged canvas, sub-pixel shift, collision flip).
- Test that is fast (pure CPU, no I/O) and portable to all CI platforms.

**Non-Goals:**

- Fixing the layouter or the Qt plane renderer pipeline.
- Reproducing the full `PlaneMapRenderer` async pipeline (image swap logic is out of scope).
- Path/contour label layout testing (glyph paths require real font metrics; separate follow-up).

## Decisions

### D1: Daemon-free layouter instantiation with mock text layouter

Implement a minimal `FakeTextLayouter` inside the test that implements the contract documented on `LabelLayouter` (see the template doc-comment in `LabelLayouter.h:610`): `Layout(projection, parameter, text, fontSize, objectWidth, enableWrapping, contourLabel)` returning a `std::shared_ptr<Label<FakeGlyph, FakeLabel>>` with fixed deterministic width/height derived from the text string, and `GlyphBoundingBox()`.

- **Alternative A — reuse an existing backend's text layouter (e.g. `MapPainterNoOp` or the Qt `TextLayouter`)**: rejected; pulls in Qt/font initialization, breaks backend independence, and makes measured widths platform-dependent — tests would not be reproducible across CI machines.
- **Alternative B — drive a real renderer with pixel comparison of output images**: rejected as an integration-style probe of a different seam; it cannot distinguish layouter instability from drawing instability, and it needs GPU/display resource handling (see existing GUI test infrastructure).

Chosen A/B alternative: custom mock. Deterministic metrics, zero dependencies, and it isolates viewport sensitivity, which lives entirely in `LabelLayouter`.

### D2: Position labels in pixel coordinates via `RegisterLabel`, no map projection math

`RegisterLabel` takes a `Vertex2D` in projection pixel coordinates and the shared code path only forwards it to the text layouter. The test constructs a valid `MercatorProjection` (via `Set()` with a center coordinate, level 10 magnification, 96 DPI, viewport size) and a default-constructed `MapParameter`, then places labels at chosen pixel positions. This mirrors exactly how `MapPainter` base calls the layouter during real renders.

- **Alternative A — emulate painter-side culling and pass geographic coordinates through a real projection**: rejected; the behavior under test (viewport dependence of collision resolution) only depends on pixel geometry, and a real projection would add Mercator rounding noise to the experiment.
- **Alternative B — test only `ScreenMask` collisions and skip `LabelLayouter`**: rejected; existing `ScreenMaskTest.cpp` already covers mask primitives, but the reported defect operates at the `LayoutJob`/priority level which masks alone cannot capture.

Chosen alternative: direct pixel-coordinate registration with a real (initialized but otherwise unused) projection object.

### D3: Read results through `Labels()` accessor, not a drawing callback

The rule under test is *which labels survive collision resolution*. `LabelLayouter::Labels()` returns `LabelInstanceType` elements with `x`/`y`/`label->width`/`label->height` — fully sufficient to assert visibility (instance present or absent) and positions.

- **Alternative A — implement a counting `Painter` and pass it to `DrawLabels<Painter>()`**: viable, but `DrawLabels` additionally filters by `visibleViewport.Intersects()` at draw time, which would conflate layout-level visibility with the separate draw-time clipping decision. Layout-level assertions are the sharper instrument.
- **Alternative B — compare intermediate `ScreenMask` canvas state**: rejected; `LayoutJob` internals are local to `Layout()` and not exposed.

Chosen alternative: assert on `Labels()` (sorted, priority-stable), keeping draw-time clipping out of the test.

### D4: Failure policy of the stability assertions

The stability scenarios form the executable specification; if the observed behavior contradicts them today (labels flip when only the viewport shifts), the test failure is the accepted outcome that demonstrates the defect. Failures report the label identity, both viewport origins, and per-run visibility to make flickering reproducible and diagnosable. The tests stay in the suite either way (user decision); they are not skipped or marked `MAY_FAIL`.

- **Alternative A — mark stability checks as "expected failure" until the layouter is fixed**: rejected; per user requirement the tests remain plain, asserting regression coverage stays in place after a fix.
- **Alternative B — split diagnostics out into a manually-run demo binary**: rejected; CI coverage is the point.

## Flow

```
+----------------+   RegisterLabel()     +--------------------+
| Test builds    |  for each label,  --> | LabelLayouter      |
| synthetic set  |  fixed pixel point    | (LabelInstances)   |
+----------------+                       +---------+----------+
        ^                                          |
        | SetViewport(V)                           v
        |                                Layout() -> LayoutJob
        +---- runs 2..N with ---->       - sort by priority+ref
             shifted viewport            - greedy ScreenMask filling
                                                   |
                                                   v
                                        Labels() --> assertions:
                                        visibility x positions per run
```

## Risks / Trade-offs

- [LabelLayouter internals change (e.g. different padding defaults) → tests break] → Mitigation: assertions compare *two runs of the same code against each other*, not absolute pixel values; only scenario fixtures (positions, sizes, viewport deltas) are fixed.
- [Single-winning-label scenarios may hide mid-priority resolution quirks] → Mitigation: collision scenario uses a priority distance of 1 (adjacent priorities) to make order resolution observable.
- [Platform differences in Qt/Catch2 numeric formatting] → Mitigation: all arithmetic is integer pixel math; no floating-point comparisons except positions passed in, which are the test's own inputs.
- [Mock metrics diverge from real font behavior (no glyph overhang)] → Accepted: the defect class under test is geometric viewport sensitivity, which exists independently of glyph shape. Real-metric follow-up can extend the mock.

## Migration Plan

1. Add `Tests/src/LabelLayouterTest.cpp`, register in `Tests/CMakeLists.txt` and `Tests/meson.build`.
2. Build via CMake and Meson, run `ctest -R LabelLayouterTest` / `meson test`.
3. Result interpretation: passing = current layouter stable under the tested shifts (still valuable regression net); failing = reproduction artifact for the flicker defect, to be linked from the follow-up fix change.
Rollback: delete test file and the two build system registrations; no library code touched.

## Open Questions

None — the layouter can be instantiated without a projection's derived values being meaningful, as verified by reading `LabelLayouter.h` registration/collision paths. If the mock approach hits an unexpected compile-time dependency on a concrete backend, fall back to instantiating the layouter the way `MapPainterNoOp` does.