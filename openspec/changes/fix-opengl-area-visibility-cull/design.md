# Design

## Context

See `proposal.md` for the motivation. This section is the state the approach has to work with.

The whole area step of the OpenGL backend is one function,
`MapPainterOpenGL::ProcessAreas` (`libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:184`).
Per call it

1. merges `data.areas` and `data.poiAreas` into a local `std::vector<AreaRef>` and sorts it by
   bounding-box area descending (`:188-200`) - that sort *is* the draw order,
2. walks the rings of each area, skips the master ring and clipping inner rings
   (`ring.IsTopOuter() || !ring.GetType()->GetIgnore()`), resolves fill and border styles and skips a
   ring that resolves neither (`:230-250`),
3. **copies** the ring's nodes into a local `std::vector<Point> p` (`:253`) and removes duplicate
   points with a nested loop over all point pairs (`:255-270`),
4. copies the following clipping rings into a local `std::vector<Area::Ring> r` (`:272-281`),
5. skips the ring when it has no fill style (`:283-287`),
6. derives `borderWidth` from the *first* resolved border style, or 0.0 when that style has an offset
   (`:290-299`), and
7. tests visibility with `IsVisibleArea(loadProjection, ringBoundingBox, borderWidth / 2.0)`
   (`:301-303`) - the declared millimetres handed to a parameter that enlarges a screen box by pixels.

The predicate is the painter's own private `IsVisibleArea` (`:415-441`): it projects the two corners
with `GeoToPixel`, enlarges the box by `pixelOffset`, compares against
`projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM())` and the frame's screen box.

The core painter fixed the unit half of this in `f0176205b` (change
`fix-area-cull-pixel-tolerance`) by converting at the call site
(`libosmscout-map/src/osmscoutmap/MapPainter.cpp:1335`). No code is shared: `MapPainterOpenGL` is not
a `MapPainter` (`libosmscout-map-opengl/include/osmscoutmapopengl/MapPainterOpenGL.h:33`), so the fix
does not reach the OpenGL backend.

Constraints that shape the approach:

- `MapPainterOpenGL` creates GL programs and renderers in its constructor, so **no headless unit test
  can instantiate it**; `IsVisibleArea` is private and needs `parameter`.
- The OpenGL performance tests already run in CI with the resources they need:
  `Tests/CMakeLists.txt:306-380` registers `PerformanceTest-<driver>-<stylesheet>` with `--shaders`,
  `--font` and `--icons`, and `Tests/meson.build:597-608` links the OpenGL branch of the same driver.
  Ubuntu runs them under `xvfb-run`.
- Adjacent defects in the same loop are recorded in `TODO.md` and stay out of scope: the
  `if (!fillStyle) continue;` skip of border-only rings, the unused-but-set `lineOffset` (`:466`), and
  the fact that the tolerance only looks at the first border style.

## Goals / Non-Goals

**Goals:**

- The OpenGL area step conforms to the area rejection and area preparation contracts, with the
  tolerance applied in the frame's pixels and the per-ring decision ahead of the per-ring geometry
  work.
- The unit-bearing part of the decision is testable without a GL context.
- The work and the allocation of the step follow the rings the frame keeps, measurably.
- The prepared ring set, the draw order and the rendered output strictly inside the view are
  unchanged.

**Non-Goals:**

- Unifying the core painter's and the OpenGL painter's visibility predicates (decision D1c).
- Fixing the border-only-ring skip, the `lineOffset` warning, or the first-border-style tolerance.
- Any change to the other backends, to the database formats or to the OpenGL painter's public API.

## Decisions

### D1 - The tolerance is converted inside a GL-free predicate that takes millimetres

| | approach | rationale |
|---|---|---|
| a | mirror the core fix at the call site: `IsVisibleArea(loadProjection, ringBoundingBox, loadProjection.ConvertWidthToPixel(borderWidth/2.0))` | smallest diff, but the property stays untestable without GL and the call site can be given millimetres again |
| b | **chosen** - a predicate in the OpenGL library that takes the declared millimetre lengths and converts them internally: `libosmscout-map-opengl/{include/osmscoutmapopengl/AreaVisibility.h,src/osmscoutmapopengl/AreaVisibility.cpp}`, `bool IsAreaRingVisible(const Projection&, const GeoBox&, double borderWidthMM, double minDimensionMM)` | the unit mistake becomes unrepresentable at the call site; only `Projection`/`GeoBox` are needed, so it is testable headless; blast radius stays in one library |
| c | extract the core painter's `MapPainter::IsVisibleArea` (`libosmscout-map/src/osmscoutmap/MapPainter.cpp:257`) into a shared free function for both painters | removes a genuine duplication, but re-touches the core painter right after its own fix, and moves backend-specific minimum-dimension handling into `libosmscout-map` |

Chosen (b). Risks: a new installed header and one more exported symbol - export it with
`OSMSCOUT_MAP_OPENGL_API` and register it in both build systems, as `LoadPNGOpenGL`
(`PNGLoaderOpenGL.h:30`) does. The predicate duplicates the core semantics; if the core decision
changes later, the two can drift, which `TODO.md` records as the reason (c) is deferred.

### D2 - The visibility decision moves ahead of the per-ring geometry work

| | approach | rationale |
|---|---|---|
| a | keep the order, fix only the unit | fewer lines touched, but the O(n²) duplicate removal and the clipping copy stay on rings that are discarded - and the corrected, larger tolerance makes *more* rings reach that work, so the step would get slower |
| b | **chosen** - hoist the existing decision above the node copy (`:253`) and the duplicate-point removal (`:255-270`); the ring's bounding box and its resolved styles are available before the copy | the inputs precede the copy, no new state, and the kept-ring set is unchanged, so the output inside the view is unchanged |
| c | two passes: decide all rings first, then prepare the kept ones | more state and a second traversal of every ring list, with a draw-order hazard (draw order is the sorted area order, ring by ring) |

Chosen (b). The style resolution has to stay before the decision, because the tolerance is half the
width of the resolved border style. Risk: a later edit could drag work above the decision again - the
kept-ring counter of D4 makes that observable in a test.

### D3 - The merged, sorted area list lives in painter-owned scratch

| | approach | rationale |
|---|---|---|
| a | keep the local `std::vector<AreaRef> areas` and its `std::sort` (`:188-200`) | the sort defines the draw order and has to stay, but the copy allocates per loaded area on every call |
| b | **chosen** - painter-owned scratch, `clear()` + insert with the capacity kept, sorted in place per call | no heap allocation grows with the loaded areas, and the draw order stays bit-identical |
| c | cache the merged and sorted list keyed on the `MapData` identity | fewer per-call comparisons, but the same `MapData` can carry different areas per frame, and a stale cache would silently change the draw order |

Chosen (b). Risk: painter-owned mutable state assumes one thread per painter per frame, which the
painter already assumes for its swapped render buffers (`OpenGLMapData`) and which
`PerformanceTestBackendOGL::DrawMap` and the clients satisfy; the member comment states the affinity.

### D4 - Test seam: a headless predicate test plus a kept-ring counter

| | approach | rationale |
|---|---|---|
| a | unit test with an offscreen GLFW context, as `PerformanceTestBackendOGL` creates one | covers the real `ProcessAreas`, but needs a context in every job (the sanitizer job runs `ctest` without `xvfb`) and couples a contract test to GL |
| b | **chosen** - a headless Catch2 test of the predicate from D1 (new `Tests/src/OpenGLAreaVisibilityTest.cpp`) plus a small diagnostic counter of examined and kept rings on `MapPainterOpenGL`, asserted through the offscreen driver where one is available | the unit-bearing seam is testable everywhere and needs no display; the counter makes the per-ring work observable, the way `AreaIndex::GetEntryCount`/`GetExaminedEntryCount` do |
| c | measurement only, through the existing `PerformanceTest-opengl-*` entries | cheapest, but the borderline-tolerance scenario needs a rendering comparison and the unit conversion needs a deterministic case, neither of which a timing run provides |

Chosen (b), with (c) as the second half of the verification, not as its only evidence. Risks: the
counter is test-only surface - keep it a const getter, documented as a diagnostic for tests; the GL
case must be skipped, not failed, when no context can be created.

## Sequence diagram

BEFORE - the order inside one ring:

```text
  resolve fill and border styles
  no style at all? ........................ yes -> next ring
  copy the ring nodes
  remove duplicate points (nested loop, O(n^2))
  copy the clipping rings
  no fill style? .......................... yes -> next ring
  derive the tolerance from the border style (millimetres)
  visible? (the millimetres used as pixels)  yes -> next ring
  transform and triangulate
```

AFTER - the same step, with the decision ahead of the per-ring geometry work:

```text
  resolve fill and border styles
  no style at all? ........................ yes -> next ring
  visible? (the tolerance converted to pixels)  no -> next ring
  no fill style? .......................... yes -> next ring
  copy the ring nodes
  remove duplicate points (kept rings only)
  copy the clipping rings
  transform and triangulate
```

Per call, around the ring loop:

```text
  BEFORE: merge data.areas and data.poiAreas into a local vector,
          sort it by bounding-box area (the draw order)  -> allocates per loaded area

  AFTER:  merge into painter-owned scratch, sort it in place
          -> no heap allocation that grows with the loaded areas
```

## Risks / Trade-offs

- [The corrected tolerance keeps more rings, so the step prepares more geometry at high DPI] ->
  D2 makes the per-ring work follow the kept rings; `verification.md` reports the kept-ring count
  *and* the step time, so a higher kept count is not mistaken for a regression.
- [The new predicate drifts from the core painter's decision] -> D1c is deliberately deferred and the
  reason is recorded in `TODO.md`.
- [A new public header that only one build system installs] -> register it in
  `libosmscout-map-opengl/CMakeLists.txt`, `include/meson.build` and `src/meson.build`; the sanitizer
  job's `cmake --install` step exercises the install.
- [The deliberate viewport-edge output change is seen as a regression] -> the spec states it, and
  `verification.md` shows the edge difference and the unchanged output inside the view.
- [The border-pixel scenario cannot be reproduced with a border-only area, because the step skips
  rings without a fill style] -> the scenario is verified with a filled and bordered area; the skip is
  recorded in `TODO.md` rather than worked around.

## Migration Plan

None. No format, database or API change. Rollback is a revert; the only observable difference would
be the viewport-edge geometry of the OpenGL backend.

## Open Questions

- Whether the diagnostic counter should also expose the number of examined rings (for the
  measurement) or only the kept ones. This is resolvable during implementation and changes neither the
  specs nor the approach.
