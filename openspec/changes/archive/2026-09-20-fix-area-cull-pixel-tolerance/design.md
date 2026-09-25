## Context

See `proposal.md` - Why, and `specs/map-painter-area-culling/spec.md` for the requirements.

Current state that shapes the approach:

- `MapPainter::IsVisibleArea(projection, bbox, pixelOffset)` (`libosmscout-map/src/osmscoutmap/MapPainter.cpp`)
  converts the bounding box into screen space, enlarges it by `pixelOffset` **pixels**, compares the
  enlarged size against the member `areaMinDimension` - itself set from
  `projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM())` at `MapPainter.cpp:2302` - and
  intersects it with the screen box. The helper's other argument is therefore already a screen length.
- `MapPainter::PrepareAreaRing` (`MapPainter.cpp:1331`) passes `borderWidth/2.0` as that offset, where
  `borderWidth` is `borderStyle->GetWidth()`, a width in millimetres. `MapPainter::ProcessAreas`
  (`MapPainter.cpp:1512`) derives its early `earlyOffset` from
  `styleConfig.GetMaxAreaBorderWidthMM(...)` with the same factor and the same missing conversion.
- Every drawing path converts the same declared width with the frame's projection before it draws, for
  example `MapPainterCairo::DrawFillStyle`; the reach of a centred stroke is half of that converted
  width. The tolerance is therefore smaller than the geometry it must cover by `dpi/25.4`: about 3.8x at
  96 DPI, about 11.8x at the 300 DPI of `Tests/src/MercatorProjectionTest.cpp`.
- The debug assert in `PrepareAreaRing` (`borderWidth<=GetMaxAreaBorderWidthMM(...)`) compares the two
  values in millimetres, which is why the culling change could keep the invariant while both sites used
  the wrong unit.
- The callers of `IsVisibleWay`, the sibling helper, pass screen lengths already: `wayReachPixel` (the
  name says so, `MapPainter.cpp:362`), label and shield extents in pixels (`MapPainter.cpp:1038`), and
  `CalculateLineWith`, which returns pixels (`MapPainter.cpp:1626-1653`). `MapPainterOpenGL::ProcessAreas`
  has its own visibility call and passes `borderWidth/2.0` the same way (`MapPainterOpenGL.cpp:299`).
- The design (D2) of the culling change accepted the raw-millimetre unit deliberately, because converting
  only the early bound would have made it smaller than the per-ring tolerance at DPI below 25.4 and would
  have broken the conservativeness requirement; the mismatch itself was recorded in `TODO.md` as a
  separate, behaviour-changing task. This is that task.

## Goals / Non-Goals

**Goals:**

- Make the tolerance a visibility decision applies a screen-space length, so it covers the geometry the
  decision is about to discard, at every DPI.
- Keep the early rejection conservative at every DPI: the converted early offset is the converted form of
  the per-ring expression.
- Keep the change local to the two call sites, the helper documentation and the tests, with no API and no
  policy change.
- Pin the unit with a test that fails under the old behaviour, so the mismatch cannot return.

**Non-Goals:**

- Changing the tolerance policy itself (still half of a declared border width), or introducing a pixel
  floor for sub-pixel borders.
- Changing the signature of `IsVisibleArea` / `IsVisibleWay`, which are protected helpers with several
  pixel-passing callers.
- Fixing `MapPainterOpenGL::ProcessAreas`, whose own visibility call carries the same defect; it belongs
  to that backend and its performance entry in `TODO.md`.
- Any output-neutral guarantee: the fix adds border pixels at the viewport edge by design.

## Decisions

### D1: The call sites convert with `Projection::ConvertWidthToPixel`

Alternatives:

1. **Convert at the two call sites in `MapPainter`** (chosen).
2. Change `IsVisibleArea` to take a width in millimetres and convert inside.
3. Convert in `StyleConfig`, i.e. store the bound in pixels per frame.

Chosen because the helper's contract is pixels and every other caller already passes pixels (Context), so
converting at the two sites that hold a style-sheet width keeps one unit per parameter and leaves the
way, label, point and OpenGL callers alone. Alternative 2 flips the unit for all of them and would need
the OpenGL backend, the label path and the way path to be changed in the same commit; alternative 3 would
put a frame-dependent value (the DPI) into a per-level structure of the style configuration, which is
shared between frames.

### D2: Both the per-ring offset and the early offset are converted, and the assert stays

Alternatives:

1. **Convert `borderWidth/2.0` and the derived bound, keep the millimetre assert** (chosen).
2. Convert only the per-ring offset.
3. Convert only the early offset.

Chosen because the conservativeness requirement is an implication between the two values in the unit the
test applies. Converting one side only cannot hold at every DPI: the raw early bound is the converted
per-ring tolerance multiplied by `25.4/dpi`, so it is smaller than the per-ring tolerance for every DPI
above 25.4 (alternative 2), and the raw per-ring tolerance is smaller for every DPI below 25.4
(alternative 3). The assert compares the two widths in millimetres, before any conversion, so it stays
valid and keeps guarding the same logic error.

### D3: The regression test prepares the same view at two DPIs

Alternatives:

1. **Prepare one view with two projections that differ only in their DPI, and assert which areas survive**
   (chosen).
2. Expose the derived offset through an accessor and assert its numeric value.
3. Compare a rasterised frame at the viewport edge against a golden image.

Chosen because it observes the contract ("which areas reach per-ring preparation") through the seams the
existing tests use - a synthetic painter, a synthetic style sheet and a synthetic area placed by pixel
distance - and it needs no new API. It fails under the old behaviour for a wide border, because both
projections then use the same raw millimetre number. Alternative 2 asserts an implementation detail and
adds public surface for a test; alternative 3 depends on a font, a cairo surface and pixel comparison,
and the error is sub-pixel at low DPI.

### D4: The tolerance policy stays "half of the declared border width"

Alternatives:

1. **Keep the factor and fix only the unit** (chosen).
2. Use the full declared width as the tolerance.
3. Add a floor in pixels (for example 1 px) for borders narrower than a pixel.

Chosen because half of the width is the geometric reach of a centred stroke, which is the quantity the
decision must cover; changing the factor or adding a floor changes which areas are prepared beyond
correcting a unit error and has no measurement behind it. Alternative 2 doubles the band of prepared
areas around the viewport for no correctness gain; alternative 3 is a policy question for sub-pixel
borders, recorded as an open question rather than smuggled into a unit fix.

### D5: The OpenGL backend keeps its own call, recorded in `TODO.md`

Alternatives:

1. **Leave `MapPainterOpenGL::ProcessAreas` unchanged and keep the note in `TODO.md`** (chosen).
2. Convert its visibility call in the same change.

Chosen because the capability of this change is the core painter's decision, the OpenGL backend has its
own per-data-load reprocessing entry in `TODO.md` that will touch the same loop, and its visibility call
cannot be run on a machine without the shaders and a font it needs. Alternative 2 would put an untested
behaviour change into a backend this change does not otherwise touch.

## Sequence diagram

```
MapPainter::DrawMap(frame)
  |
  +-- ProcessAreas(projection, parameter, data)
  |     |
  |     |  earlyOffset = projection.ConvertWidthToPixel(
  |     |                  styleConfig->GetMaxAreaBorderWidthMM(level) * 0.5)
  |     |                                            ^ conversion moved here
  |     for each loaded area
  |       +-- IsVisibleArea(area->GetBoundingBox(), earlyOffset)  [screen px]
  |             |
  |             +-- true --> PrepareArea -> PrepareAreaRing(ring)
  |                           |
  |                           +-- style resolution -> borderWidth [mm]
  |                           +-- assert(borderWidth <= maxAreaBorderWidthMM)   [mm, unchanged]
  |                           +-- IsVisibleArea(ringBBox,
  |                                 projection.ConvertWidthToPixel(borderWidth/2.0))
  |                                                    ^ conversion added here
  |                                 +-- true --> TransformAreaRing -> prepared entry
  |
  +-- DrawAreas / ... (unchanged)
```

## Risks / Trade-offs

- [The fix changes rendered output at the viewport edge, which the culling change promised not to] ->
  The promise is about the cull not changing output relative to an unculled preparation of the same
  build; the requirement is reworded accordingly. The visible change is the border geometry the wrong
  unit discarded, which is the point of the fix, and it is asserted by the culling test's before/after
  comparison (both sides convert).
- [More areas reach per-ring preparation, so the culling win shrinks] -> The band that grows is the
  converted tolerance of the widest border style, i.e. `ConvertWidthToPixel(maxMM/2)` pixels around the
  viewport: below 1 px for the shipped style sheets at 96 DPI (0.05 mm per the culling change's
  measurement -> 0.19 px) and about 2.4 px for a 1 mm border at 96 DPI. The before/after measurement of
  the step on the fixed view records the prepared counts, which is what a review can check.
- [A high-DPI frame now prepares noticeably more areas for a wide border] -> The tolerance is proportional
  to the DPI by design, because the drawn border is too; the alternative is the current behaviour, which
  discards pixels that are drawn at high DPI.
- [Another caller passes a millimetre value as a pixel offset] -> Audited in Context: the way, label and
  point callers already pass screen lengths, and `IsVisibleArea`'s own `areaMinDimension` comparison is a
  converted value. The helper documentation now names the unit, and the OpenGL call is recorded.
- [The debug assert can fire for a style sheet whose border width exceeds the bound] -> It cannot: both
  sides of the assert are millimetres read from the same style configuration, unchanged by this change,
  and the assert tests the inequality the bound is derived from.

## Migration Plan

No migration, no persisted data and no API change; rolling back is removing the two conversions. The
verification is the updated conservativeness test, the new two-DPI test, the existing area preparation,
culling and rendering tests, and the before/after measurement of the preparation step on the fixed view.

## Open Questions

- Whether a sub-pixel floor (for example 1 px) should apply to tolerances below a pixel: the shipped
  style sheets produce a converted half-width below 0.2 px at 96 DPI, and the ring is kept or rejected by
  its geometry rather than by the tolerance in that regime. A floor is a policy change, not a unit fix.
