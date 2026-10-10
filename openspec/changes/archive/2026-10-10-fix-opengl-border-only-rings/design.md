## Context

See `proposal.md` - Why for the defect and its root cause. The ring loop of
`MapPainterOpenGL::ProcessAreas` (`libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp`)
resolves a ring's fill style and its border styles, rejects a ring that resolves neither, decides
visibility, prepares the ring's geometry, and then fills the ring's triangles and draws the border styles
that ring resolved. `fix-opengl-area-visibility-cull` hoisted the visibility decision above the per-ring
geometry work and left this order in place, including the `if (!fillStyle) continue;` that sits between
the geometry preparation and the fill.

The other backends separate fill and border already. `MapPainterAgg::DrawFill`
(`libosmscout-map-agg/src/osmscoutmapagg/MapPainterAgg.cpp:151-170`) draws the fill only when a fill style
exists and strokes `borderStyle` whenever it is set, and the core painter's `PrepareAreaRing`
(`libosmscout-map/src/osmscoutmap/MapPainter.cpp:1342-1345`, `:1419-1449`) prepares an `AreaData` with a
null fill style and the resolved border styles for exactly this ring shape.

## Goals / Non-Goals

Goals: the OpenGL area step keeps a ring whose only resolved style is a border and draws that border; a
ring that resolves no style at all is still not examined and not kept; the prepared geometry, the draw
order and the vertex buffers of every other ring are unchanged.

Non-goals: the derivation of the border width the loop draws with (TODO §99 records the same "first
border style" shape for the visibility tolerance), the loss of the border of a filled ring whose triangulation throws, the
public API of the painter, and the shader, font and image resource handling of the backend.

## Decisions

### 1. Guard the fill path with the resolved fill style instead of skipping the ring

The fill path is made conditional on the fill style and the border loop below it is left where it is, so
the ring reaches the border loop whether or not it has a fill. `keptRingCount` moves above the guard: a
ring the step keeps is a ring it prepares geometry for, and a ring drawn by a border only is such a ring.

Alternatives the evidence eliminated:

- **Delete the skip.** `fillStyle->GetFillColor()` is called unconditionally after it; the red run proves
  the step resolves *no* fill style for the case's area type, so the deletion dereferences a null
  reference. Eliminated by the red case's own data.
- **Two ring passes, one for the fills and one for the borders.** The second pass repeats the per-ring
  style resolution, the visibility decision and the geometry work for every ring the step kept - the work
  the previous change removed from the step - to reach a loop that already exists below the fill.
- **Move the border loop in front of the fill block for fill-less rings.** It encodes the ring's
  fill/border order in two places and makes the order of the two buffers depend on which styles a ring
  resolves. The guard keeps one order for every ring.
- **Extract the fill/border contribution decision into a free function beside `IsAreaRingVisible`** so a
  case can decide it without a GL context. Rejected: the case has to be red on HEAD, where the function
  does not exist, so it cannot carry the red evidence; its only caller would be a test, which is
  scaffolding rather than a behavioural delta.

### 2. No new diagnostic counter

The case observes the step through the existing `GetExaminedRingCount()` and `GetKeptRingCount()`, whose
documented meaning is "rings kept and prepared geometry for". A counter of the border vertices the step
adds would observe the drawing half more directly, but it does not exist on HEAD, so the assertion could
not appear in the case that has to be red there; it would be verified by the probe alone while adding
public API.

## Risks / Trade-offs

- [The border of a border-only ring whose *first* border style carries an offset is drawn with a width of
  zero, because the loop derives one width from that style for all of them] → pre-existing, shared with
  TODO §99; reported, not fixed here.
- [The case skips when no offscreen GL context can be created] → the gate run in this change creates one
  and the skip is recorded in the evidence log; a job without a context loses the case rather than failing
  it, which is the behaviour the neighbouring painter case has.
- [The case observes the step's kept counts, not the vertex buffer] → `keptRingCount` is documented as
  "kept and prepared geometry for"; the drawing itself is the border loop that follows the guard
  unconditionally - the reviewer has to look at that hunk.

## Migration Plan

Revert the diff: the painter again drops rings that resolve a border and no fill. No database, file
format, build option or public API is involved, so there is no migration.

## Open Questions

None. The two findings named above are recorded as follow-ups, not as questions.
