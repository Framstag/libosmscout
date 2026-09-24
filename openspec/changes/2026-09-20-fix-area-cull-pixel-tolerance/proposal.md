## Why

The area culling the `map-painter-area-visibility-cull` change added derives its tolerance from the
border width the loaded style sheet declares, but hands that width - a length in millimetres - to the
painter's visibility test, which enlarges a screen box by a number of pixels. Every backend converts the
same declared width with the frame's projection before it draws it, so the tolerance the culling applies
is smaller than the border it is meant to leave room for: about 3.8x smaller at 96 DPI and about 11.8x
smaller at the 300 DPI the map tests project with. The per-ring visibility decision has the same defect,
so an area whose border would still cross the viewport edge is dropped, and the error grows with the DPI
of the frame. The early rejection also loses the guarantee that it covers every per-ring tolerance for
the unit reason alone, which is what its conservativeness rests on.

## What Changes

- A visibility tolerance derived from a width a style sheet declares SHALL be a screen-space length: it
  SHALL be converted from millimetres to the frame's pixels before it is applied, in the per-ring
  visibility decision and in the early rejection alike.
- The early rejection SHALL use the converted form of the same expression as the per-ring decision, so
  that it cannot reject an area a per-ring decision would keep, at any DPI.
- The rendered output of a view SHALL contain the border pixels of an area whose border crosses the
  viewport edge. The fix is therefore a deliberate, DPI-dependent output change at the viewport edge,
  not an output-neutral change.
- Nothing else changes: the preparation order, the entries of areas inside the view, the clipping
  geometry and the draw order stay as they are.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `map-painter-area-culling`: the capability now also states that a tolerance derived from a style sheet
  is a screen-space length (new requirement, since the tolerance of a visibility decision is part of the
  contract the capability makes); the conservativeness requirement keeps its scenarios and gains one for
  the DPI; and the "unchanged output" requirement is clarified to compare a culled preparation with the
  unculled preparation of the same build, because the tolerance conversion itself is an intentional
  output change.

## Impact

Affected modules:

- `libosmscout-map` (core rendering, `MapPainter`):
  - `src/osmscoutmap/MapPainter.cpp` - `PrepareAreaRing` (the converted per-ring `IsVisibleArea` call and
    the assert comment that binds the per-ring tolerance to the derived bound), `ProcessAreas` (the
    converted early offset).
  - `include/osmscoutmap/MapPainter.h` - the documentation of `IsVisibleArea` and `IsVisibleWay`, which
    now names the unit of `pixelOffset` and the conversion a style-sheet width needs.
- `Tests`:
  - `src/MapPainterAreaVisibilityCullTest.cpp` - the unit assumptions of the conservativeness test and a
    new regression test that prepares the same view at two DPIs, which fails under the millimetre-as-pixel
    behaviour.
- `TODO.md`: the entry "Millimetre border width used as a pixel offset in the per-ring visibility
  decision" is removed; the OpenGL entry keeps its note that the OpenGL painter's own visibility call
  carries the same defect.

**Public API**: no addition. `IsVisibleArea` and `IsVisibleWay` are protected helpers of `MapPainter`;
their pixel unit is documented, not changed.

No database format, style sheet syntax, import or build system impact. Not touched:
`MapPainterOpenGL::ProcessAreas`, which converts nothing for its own visibility call (recorded in
`TODO.md`).
