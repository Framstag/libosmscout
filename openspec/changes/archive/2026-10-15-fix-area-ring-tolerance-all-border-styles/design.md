# Design

See `proposal.md` — Why for the motivation, and `specs/map-painter-area-culling/spec.md` for the
contract this design implements.

## Context

Two expression sites decide whether an area ring can be visible, and both read a single border style:

- The shared painter, `libosmscout-map/src/osmscoutmap/MapPainter.cpp:1329-1355` (`PrepareAreaRing`).
  It reads `borderStyles.front()`, keeps it only when that style has neither an offset nor a display
  offset (`:1332-1337`), takes half its width (`:1339`) and converts it with
  `projection.ConvertWidthToPixel` (`:1352`). A front style that is offset leaves `borderWidth` at
  `0.0`. The rings' borders are drawn later from every style (`:1401-1433`), where each style's
  `offset` is projected with `GetProjectedWidth` and its `displayOffset` converted with
  `ConvertWidthToPixel` before `GenerateParallelWay` shifts the ring.
- The OpenGL backend, `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:266-280`,
  repeats the same rule and calls the exported predicate
  `osmscoutmapopengl::IsAreaRingVisible(projection, box, borderWidthMM, minDimensionMM)`
  (`libosmscout-map-opengl/src/osmscoutmapopengl/AreaVisibility.cpp`), which applies the `0.5` factor
  and the conversion itself.

The early area rejection (`MapPainter.cpp:1526-1543`) uses half of
`StyleConfig::GetMaxAreaBorderWidthMM(magnification)` — a per-level maximum over the built area
border style selectors, filled in `StyleConfig.cpp:786-801` from `selector.style->GetWidth()` only.
`MapPainter.cpp:1347` asserts the per-ring width stays within that bound.

The frame-wide conversion of a style sheet's reach already has a home:
`StyleConfig::VisibilityBounds` (`StyleConfig.h:555-564`, units of the style sheet, per level, filled
by `StyleConfig::PostprocessVisibilityBounds`, `StyleConfig.cpp:1036-1066`) plus
`MapPainter::UpdateVisibilityBounds` (`MapPainter.cpp:311-344`), which converts it per frame into
`DatabaseCacheEntry::wayReachPixel` and `pointReachPixel` and is called once per frame. The area step
does not use it yet.

Constraints:

- `ConvertWidthToPixel` (mm → pixels) depends on the frame's DPI only; `GetProjectedWidth`
  (map units → pixels) depends on `Projection::GetPixelSize()` and is therefore a single value per
  frame.
- A frame-wide bound cannot be the maximum of the per-style sums without iterating every style of the
  sheet per frame; `wayReachPixel` already accepts a sum of separate maxima for this reason
  (`MapPainter.cpp:318-330`).
- Both build systems and all shipped stylesheets stay as they are; no stylesheet in the repository
  declares an offset area border today, so the change is unobservable in the shipped configurations.

## Goals / Non-Goals

**Goals:**

- The per-ring decision covers every border style the ring resolves: the widest width and the reach
  of an offset border.
- The early rejection stays at least as wide as any per-ring decision, without iterating the style
  sheet per frame.
- The shared painter and the OpenGL backend derive the tolerance from one expression, so the two
  cannot drift again.

**Non-Goals:**

- Reducing the early bound's over-approximation for style sheets that declare an offset border
  (see Decisions, D2).
- Changing the fill-style decision, the draw order, the clip geometry or the OpenGL area step's skip
  of fill-less rings (TODO §98).
- Replacing the vendored triangulator (TODO §100) or the OpenGL node-path defect (TODO §97, open
  branch `harden-polygon-triangulation`).

## Decisions

### D1: The per-ring tolerance is the maximum reach over the resolved border styles

Tolerance for a ring = max over the styles `GetAreaBorderStyles` returned of
`ConvertWidthToPixel(width/2) + ConvertWidthToPixel(|displayOffset|) + GetProjectedWidth(|offset|)`.
The three terms are the reach the drawing loop below actually uses (`MapPainter.cpp:1410-1418`), so
the decision covers what the drawing step draws and nothing more.

*Alternatives:*

- **Half the width of the widest resolved style only.** Leaves the defect for offset borders: the
  ring is still decided with a tolerance that ignores where its border is drawn.
- **Today's single-style rule, widened only when the front style is offset.** Two rules in one
  function that can disagree; leaves the multi-style case (a ring whose widest drawn border is not
  the first) wrong, which the added spec requirement also covers.
- **Sheet-wide maximum for every ring.** Conservative but wrong direction: a ring with a narrow
  border would be kept as far out as the widest style of the sheet, adding prepared entries for
  style sheets that never needed them.

*Risk:* the tolerance is now the maximum of a sum, so a ring is kept slightly further out than any
single drawn border needs; the per-ring decision remains the only one that can reject a ring, so the
effect is bounded by one expression.

### D2: The frame-wide bound becomes an area reach on the existing visibility-bounds mechanism

Add the area border reach to `StyleConfig::VisibilityBounds` as separate per-level maxima in the
units each part is declared in — `maxAreaBorderWidth` (mm, the existing
`maxAreaBorderWidthMM` value), `maxAreaBorderDisplayOffset` (mm) and `maxAreaBorderOffset` (map
units), filled in `StyleConfig::PostprocessAreas`/`PostprocessVisibilityBounds` from the same built
`areaBorderStyleSelectors` that fills the width today. `MapPainter::UpdateVisibilityBounds` converts
them once per frame into `DatabaseCacheEntry::areaReachPixel`, together with the same projection the
per-ring decisions use. The early rejection of `PrepareAreas` consumes that value; the sum of the
three separate maxima is an upper bound of every per-style sum, which is what the conservative
requirement needs.

*Alternatives:*

- **Keep the inline `GetMaxAreaBorderWidthMM` expression and extend it.** Leaves the frame-wide bound
  in the area step while the equivalent bound for ways and points lives in `UpdateVisibilityBounds`;
  the assert at `MapPainter.cpp:1347` would have to grow its own conversion of the offset terms.
- **Compute the exact maximum of the per-style sums at postprocess time.** The offset term is in map
  units and can only be converted with the frame's projection, and the selectors are per type per
  level, so this means iterating every area border style of the sheet per frame — the cost the early
  decision exists to avoid.
- **Resolve the area's own types' border styles in the early decision** to get a per-area tolerance.
  That is the per-ring style resolution the early rejection deliberately precedes
  (`openspec/specs/map-painter-area-culling` — "The decision precedes per-ring style resolution"), and
  it would move the cost back onto every loaded area.

*Risk:* the early bound is looser than the per-ring one whenever a style sheet declares an offset
border, so more areas reach per-ring preparation for such a sheet. The shipped stylesheets declare no
area border offset, so their cost is unchanged; the looser bound is recorded in the spec's scenario
"An area whose only reachable border is offset is not rejected early" rather than hidden.

### D3: One tolerance expression, shared by the shared painter and the OpenGL backend

The tolerance computation of D1 lives in one place that both consumers call: the exported free
function `osmscout::GetAreaRingTolerancePixel` in `libosmscout-map`
(`include/osmscoutmap/AreaBorderReach.h`, `src/osmscoutmap/AreaBorderReach.cpp`) that takes the
frame's projection and the resolved border styles and returns the tolerance in pixels.
`MapPainter::PrepareAreaRing` uses it for the per-ring decision, and
`MapPainterOpenGL::ProcessAreas` uses it instead of its repeated front-style rule. The exported
OpenGL predicate then receives an already-converted tolerance in pixels and keeps its
`minDimensionMM` argument and its role as the single place where the decision is made — it no longer
converts a stylesheet width.

The helper is a free function rather than a `MapPainter` member because `MapPainterOpenGL` is not a
`MapPainter` (`libosmscout-map-opengl/include/osmscoutmapopengl/MapPainterOpenGL.h:33` declares a
standalone class), so a protected member could not serve it; the pair is registered in both build
systems like every other header and source of the library.

*Alternatives:*

- **Extend the predicate's parameters** to `(borderWidthMM, borderOffset, borderDisplayOffset,
  minDimensionMM)` and convert in all three call sites. Keeps the predicate self-contained but puts
  the conversion back into more than one place and grows a five-parameter free function.
- **Pass the resolved `std::vector<BorderStyleRef>` to the predicate.** Couples the predicate to
  `StyleConfig` and puts style resolution semantics into a helper the headless test would have to
  drive through style objects.
- **Mirror the widened rule in the OpenGL backend independently.** This is the arrangement that
  produced the defect; a follow-up fix could drift the two apart again.
- **Make the helper a `MapPainter` member anyway** and give the OpenGL painter a `MapPainter`
  instance to call it on. Adds an unrelated base object to the OpenGL painter for one expression.

*Risk:* the OpenGL predicate's signature and the mm conversion it owned change, which is a public
header of `libosmscout-map-opengl`; the existing headless test
(`Tests/src/OpenGLAreaVisibilityTest.cpp`) is re-pointed at the pixel tolerance, and the DPI
conversion it used to cover is now covered through the shared helper in the same file.
`fix-opengl-area-visibility-cull` still owes a rendered OpenGL/Cairo comparison on the same file
(its task 4.2), so that change should land first.

*Known divergence (recorded, not fixed):* the OpenGL area step draws every resolved border style with
the width of the front style and applies no border offset at all (`MapPainterOpenGL.cpp:377-410`, no
`GenerateParallelWay` in the library), so its decision is now wider than its drawing for a stylesheet
that declares an offset. The decision stays conservative — it keeps rings the step draws degenerately
rather than dropping a ring the shared painter would draw — and the drawing divergence is recorded in
`TODO.md` for its own change.

### D4: Keep the per-ring invariant assertion, split by term

The assert at `MapPainter.cpp:1347` compares the per-ring width with the sheet-wide maximum. It stays
in a split form: each term of the ring's tolerance (half its widest resolved width, its largest
border offset, its largest display offset) is compared with the corresponding frame bound before the
conversion. A postprocess that stops filling one of the three maxima then fails in a Debug or
sanitizer build instead of silently rejecting a visible ring.

*Alternatives:* drop the assert and rely on the unit test only (loses the cheap invariant check);
keep one combined assert on the converted pixels (compares values already scaled by the frame's DPI,
which makes a failure harder to attribute).

### Flow

```
PrepareAreas(frame)
  |
  +-- UpdateVisibilityBounds(projection)             once per frame, per database
  |     bounds = styleConfig.GetVisibilityBounds(mag)
  |     entry.areaReachPixel = ConvertWidthToPixel(bounds.maxAreaBorderWidth/2
  |                                                + bounds.maxAreaBorderDisplayOffset)
  |                           + GetProjectedWidth(bounds.maxAreaBorderOffset)
  |
  +-- for each loaded area
  |     if !IsVisibleArea(projection, area.bbox, entry.areaReachPixel): reject area
  |     |
  |     +-- PrepareArea
  |           for each ring
  |             (fillStyle, borderStyles) = styleConfig.GetArea...
  |             if !fillStyle && borderStyles.empty(): return false
  |             tolerance = GetAreaRingTolerancePixel(projection, borderStyles)
  |             if !IsVisibleArea(projection, ring.bbox, tolerance): return false
  |             transform ring; emit one prepared entry per drawable style
  |
  +-- OpenGL backend, ProcessAreas: same tolerance helper, same decision
```

## Risks / Trade-offs

- **A wider kept set changes rendered output for style sheets that declare an offset area border or
  differing border widths per ring** → Mitigation: the spec's scenario "A ring without an offset
  border keeps its tolerance" pins the unchanged case, and the pixel-level review is part of the
  verification; the shipped stylesheets declare no offset area border, so their output is unchanged.
- **The OpenGL step's decision is wider than its own drawing** (it draws every border style with the
  front style's width and applies no border offset) → Mitigation: the decision stays conservative, so
  no ring the shared painter would draw is lost in that backend either; the drawing divergence is
  recorded in `TODO.md` as a defect of its own change, with the file and line evidence.
- **A looser early bound adds per-ring work for a style sheet with a large offset** → Mitigation:
  the additional term is zero for every shipped stylesheet; the bound is per frame and per database,
  computed once, not per area.
- **The assert at `MapPainter.cpp:1347` would fire in a Debug build if only the per-ring rule were
  widened** → Mitigation: D2 and D4 change both sides together; the Debug/`build-asan` configuration
  is part of the verification.
- **A public API change in `libosmscout-map-opengl` (predicate signature) and in
  `StyleConfig::VisibilityBounds`** → Mitigation: `GetMaxAreaBorderWidthMM` stays as a wrapper of the
  new field, so no existing caller breaks; the predicate's only in-repo caller and test are updated
  in the same change.
- **Merge order against the unarchived `fix-opengl-area-visibility-cull`**, which modifies the same
  capability delta and the same OpenGL file → Mitigation: land that change first (or rebase on it);
  its task 4.2 rendered comparison needs the OpenGL node path to stop crashing (TODO §97, PR #1874).
- **The OpenGL side cannot be compared pixel-wise until §97 is fixed** → Mitigation: the OpenGL
  verification is the headless predicate/step test; the rendered comparison is stated as pending in
  `verification.md` rather than claimed.

## Migration Plan

No data, format or API migration: the change is internal to the rendering path, `GetMaxAreaBorderWidthMM`
keeps working, and no stylesheet or build option changes. Rollback is a revert of the change; the
library has no state that outlives a call.

## Open Questions

- Should the early bound later be tightened per area type (its own styles' maxima) instead of the
  sheet-wide sum? It would reduce the over-approximation for a style sheet with one very wide or
  very offset border type, but it needs a per-type reach table. Deferrable: it changes cost, not the
  contract this change establishes.
