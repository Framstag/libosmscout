## Context

See `proposal.md` - Why, and `specs/map-painter-area-culling/spec.md` for the requirements.

Current state that shapes the approach:

- `MapPainter::ProcessAreas` (`libosmscout-map/src/osmscoutmap/MapPainter.cpp`) walks every loaded
  area of every tile and calls `PrepareArea` for it; `PrepareArea` assigns the per-ring coordinate
  ranges, builds the ring visitor and runs `Area::VisitRings`, which calls `PrepareAreaRing` once per
  ring. Only `PrepareAreaRing` decides anything: it resolves the fill and border styles, then tests
  visibility, then transforms the ring.
- The visibility test is `MapPainter::IsVisibleArea(projection, bbox, pixelOffset)`; it converts the
  box to screen space, **enlarges it by `pixelOffset` pixels**, rejects boxes whose enlarged size is
  at or below `areaMinDimension`, and intersects the result with the screen box.
- The per-ring call passes `borderWidth/2.0` as that offset, where `borderWidth` comes from the
  resolved border style. The offset therefore depends on the stylesheet *and* on the frame's DPI.
- Measured on the Dortmund database with the Cairo backend, single z15 tile at the city centre:
  17629 areas / 17650 rings loaded per frame, **726 rings visible (4.1 %)**, and
  `styled == rings == 17650` - every loaded ring reaches style resolution, and the
  `!fillStyle && borderStyles.empty()` early-out at `MapPainter.cpp:1172` is never taken with the
  shipped stylesheets.
- The largest per-ring offset used anywhere in that frame is **0.050 px**.

## Goals / Non-Goals

**Goals:**

- Remove the per-ring work of areas that cannot contribute to the view, with a decision that
  provably cannot change which rings are prepared.
- Make the tolerance of that decision a value derived from the loaded stylesheet, so it stays
  correct when stylesheets change and contains no magic number.
- Keep the change local to the core painter's area preparation and its style configuration.

**Non-Goals:**

- Changing what is drawn, in any backend, including the antialiased sub-pixel borders that dominate
  the cairo draw step - see Risks.
- Culling inside the per-ring loop for areas that survive the early decision, i.e. the ring-level
  placement of the same test (measured, but strictly dominated for the current data; deferred).
- Filtering areas in the tile load path (`MapService`): the loaded set is the cached tile data and is
  shared between views.
- Reusing prepared data, changing draw order or reducing allocation, which the earlier
  `map-painter-frame-containers` and `map-painter-area-preparation` changes own. This change removes
  work that is not allocation: measured step allocations stay at ~160 per frame either way.

## Decisions

### D1: The early decision is taken per area, before `PrepareArea`

Alternatives:

1. **Per area, in `ProcessAreas`, before calling `PrepareArea`** (chosen).
2. Per ring, inside `PrepareAreaRing` before the style lookups.
3. Per area, in the tile load path.

Measured comparison of 1 and 2 on the fixed view (same binary, one probe mode each):

| | areas rejected | rings reaching style resolution | prepared entries | ProcessAreas |
|---|---|---|---|---|
| baseline | 0 | 17650 | 726 areas / 335 ways | 1.00x |
| 2 (per ring) | 0 (16736 rings rejected) | 914 | 726 / 335 | 0.50 - 0.71x |
| 1 (per area) | 16735 | 915 | 726 / 335 | 0.41x |

Chosen because 1 dominates 2 for this data: it reaches the same styled set (915 vs 914 rings, the
difference being one ring of an area whose bounding box is marginally larger than the ring's) and the
same prepared set, while also skipping, for the 16735 rejected areas, the per-ring range assignment,
the ring visitor construction, the `Area::VisitRings` traversal and the per-ring visibility test. It is
also the smaller diff: one guard, not a branch inside the per-ring path. Alternative 3 was rejected
because the loaded set is shared, cached tile data, so filtering it would change the cache contract and
every consumer of `MapData`.

### D2: The tolerance is half of the widest area border width, in the unit the per-ring decision uses

Alternatives:

1. **Half of the widest area border width of the level, expressed in the same unit the per-ring
   decision uses** (chosen).
2. The same width converted to pixels with the frame's projection.
3. A conservative constant (the probe used 4 px).

Chosen because it is exact: it is the maximum over the same border widths the per-ring decision reads,
in the same unit, so it can never be smaller than a per-ring tolerance and needs no frame-dependent
conversion at all. Alternative 2 was the original plan and turned out to be *unsafe*: the per-ring
decision passes `borderStyle->GetWidth()/2` - a width in millimetres - as the `pixelOffset` of
`IsVisibleArea`, which expects pixels, while `Projection::ConvertWidthToPixel` multiplies by `dpi/25.4`.
For any DPI below 25.4 the converted bound is *smaller* than the per-ring tolerance, so the early
decision could reject an area whose ring would have been kept - a violation of the requirement this
change exists to satisfy. Expressing the bound in the unit the per-ring decision actually uses removes
that dependence (and the mm/pixel mismatch itself is recorded in `TODO.md`, since correcting it changes
which rings are visible and is therefore a separate, behaviour-changing task). Alternative 3 needs a
hardcoded number, and its value would have to exceed every border width any future style sheet declares.

Deriving the bound from the built per-level selector tables rather than from the conditionals guarantees
it covers exactly the border styles the per-ring decision can read. Implementation site: `StyleConfig`'s
per-level postprocessing of the area border style lookup tables
(`libosmscout-map/src/osmscoutmap/StyleConfig.cpp`, next to the `areaTypeSets`/`CalculateUsedTypes`
work), exposed through `include/osmscoutmap/StyleConfig.h` and consumed once per frame in
`MapPainter::ProcessAreas`.

### D3: The early decision reuses `IsVisibleArea`

Alternatives:

1. **Reuse `MapPainter::IsVisibleArea` on the area's bounding box with the derived offset** (chosen).
2. A separate, simpler screen-box intersection for the early decision.

Chosen because the correctness argument then reduces to one property of one function: an area's
bounding box contains the bounding box of every one of its rings, and `IsVisibleArea` is monotone under
that containment - enlarging a larger box yields a larger box, and the `areaMinDimension` comparison
tests one constant against a box that is at least as large as the ring's. So an area-level rejection
with an offset at least as large as every per-ring offset implies a rejection for each ring. Alternative
2 would duplicate the box conversion, the margin and the degenerate-size rule, and any future change to
one of them could silently break the implication.

### D4: The "no style resolution for invisible areas" contract is observed through the public fill style processor

Alternatives:

1. **A counting `FillStyleProcessor` registered through `MapParameter::RegisterFillStyleProcessor`**
   (chosen).
2. A new public counter on the painter or its statistics object.
3. A counting allocator in the test binary, relying on the style composition that allocates when two or
   more declarations match a ring.

Chosen because it observes exactly the contracted step (per-ring style resolution reached this ring)
through an existing public seam, needs no new API, and needs no assumption about which styles allocate -
which alternative 3 does, and which the measurements show is not met by the shipped stylesheets.
Alternative 2 would add public surface for a test.

### D5: The invariant is enforced by construction and checked by an assert

The safety invariant is `earlyOffset >= every per-ring offset`. It holds by construction (D2), so an
assert in the per-ring path that the offset used does not exceed the derived bound can only fire on a
logic error, not on user input such as a wide border in a stylesheet - which AGENTS.md requires us to
avoid. A test additionally renders with two stylesheets of different border widths (D2, spec scenario
"The tolerance grows with the stylesheet").

## Sequence diagram

```
MapPainter::DrawMap(frame)
  |
  +-- ProcessAreas(projection, parameter, data)
  |     |
  |     |  earlyOffset = projection.ConvertWidthToPixel(
  |     |                  styleConfig->GetMaxAreaBorderWidthMM(level)) / 2
  |     |
  |     for each loaded area (data.areas, then data.poiAreas)
  |       |
  |       +-- IsVisibleArea(projection, area->GetBoundingBox(), earlyOffset)
  |       |     |
  |       |     +-- false ---> reject: no per-ring work at all        [~95% of loaded areas]
  |       |     |
  |       |     +-- true  ---> PrepareArea(...)
  |       |                       |
  |       |                       +-- ringCoordRanges.assign(rings)
  |       |                       +-- Area::VisitRings -> PrepareAreaRing(ring)
  |       |                             |
  |       |                             +-- style resolution (fill + borders)
  |       |                             +-- IsVisibleArea(ringBBox, borderWidth/2)  [unchanged]
  |       |                             +-- true --> TransformAreaRing -> prepared entry
  |       |
  |       +-- areaData: unchanged entry set, unchanged order
  |
  +-- DrawAreas / ... (unchanged)
```

## Risks / Trade-offs

- [The derived bound is wrong - for example a border style contributed by a code path the bound does
  not scan - and the early decision drops a ring that should be drawn] -> The bound is the maximum over
  the same border style rules the per-ring decision reads, computed in the same postprocessing pass
  that builds the per-ring lookup tables, so the two cannot disagree without a logic error; the assert
  of D5 catches that in debug builds, and the spec's "prepared area entries are unchanged" and
  "rendered output is unchanged" scenarios are regression tests in every build.
- [The bound depends on the frame DPI while being stored per level] -> the stored value is in
  millimetres and is converted with the frame's projection, so a DPI change is handled by the
  conversion rather than by the stored value.
- [A stylesheet is replaced at runtime, leaving a stale bound] -> the bound is computed in the same
  pass that builds the per-level lookup tables, so it is recomputed with every `StyleConfig` load, the
  same lifetime the lookup tables already have.
- [The measured win does not reproduce, because the win depends on how much of the loaded data lies
  outside the view] -> The measured view is a worst case for the current bug (95 % rejected) but a best
  case for the fix; the fix's value scales down for views that cover all their loaded data. Acceptance
  rests on the unit tests and the before/after `PerformanceTest` evidence, not on a wall-clock
  threshold: the measuring machine's frame time for the same view varied by 3.7x between runs
  (ProcessAreas min 3.68 - 13.52 ms), so only within-run comparisons are used.
- [The change is small in frame terms] -> The measured win is 0.41 - 0.71x of the area preparation step,
  which is roughly 14 % of a frame at z15, i.e. about 5 - 8 % of the frame. The change is accepted as a
  work-removal step with no output risk, not as a large single win.
- [Antialiased sub-pixel area borders dominate the area *draw* step and are left alone] -> Measured on
  the same view, `cairo_stroke` is ~77 % of `DrawAreas` and a merged-path prototype that cuts the
  fill+stroke operation count by 59 % changes the achievable time by nothing (7.82 vs 8.03 ms floor), so
  that cost is geometry, not overhead. Removing it would change pixels, which this change must not do.
  Recorded as a TODO entry instead.

## Migration Plan

No migration and no rollback concern: the change is internal to the core painter, adds no API and no
persisted data. Rolling back is removing the guard. Verification is the new unit test plus the existing
rendering tests, and the before/after `PerformanceTest` measurement for the step.

## Open Questions

- Whether the ring-level placement (D1 alternative 2) should be added later for views dominated by
  multi-ring relations: here there are 17650 rings for 17629 areas, so it adds nothing, but a
  relation-heavy region could differ. It does not change the specs or the approach, only adds a second
  placement of the same test.
