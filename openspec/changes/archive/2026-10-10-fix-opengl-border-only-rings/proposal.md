## Why

The OpenGL area step drops a ring whose loaded style sheet resolves a border style and no fill style:
it leaves the ring loop after the style resolution and the visibility decision, so the area contributes
nothing, while every other backend strokes the border of that ring
(`libosmscout-map-agg/src/osmscoutmapagg/MapPainterAgg.cpp:170` strokes `borderStyle` independently of
`fillStyle`). Style sheets that draw an area by a border alone therefore lose those areas in the OpenGL
backend only.

`TODO.md` §98 recorded this as a finding of `fix-opengl-area-visibility-cull`, which kept the skip and
the order around it. Why now: the skip is the last known case of an area the OpenGL backend silently
drops although the other backends draw it, and the ring loop it sits in was restructured by that change
so that removing it is now a guard rather than a reordering.

Root cause: The OpenGL area step treats a ring that resolves no fill style as nothing to draw and leaves
the ring loop before the loop that draws the border styles the loaded style sheet resolved for it.

Evidence:    `Tests/src/OpenGLAreaVisibilityTest.cpp` case "A ring drawn only by a border is kept" fails
on HEAD - `CHECK( painter.GetKeptRingCount()==1 )` with expansion `0 == 1`,
`test cases: 6 | 5 passed | 1 failed`, `assertions: 116 | 115 passed | 1 failed`, 2026-10-10 12:00 UTC
(log `evidence/red-head-ctest-LastTest.log`).

Repro:       `cmake --build build --target OpenGLAreaVisibilityTest` then
`cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests" TESTS_TMP_DIR="$PWD/Tests"
ctest -R OpenGLAreaVisibilityTest --output-on-failure`

Spec:        `map-painter-area-preparation` / "A ring drawn by a border is kept".
Guideline:   no rule of `guidelines/CodeStyles.md` is contradicted by the fix; `guidelines/FileFormatVersion.md`
requires no version bump because no type config, database file format or public serialized layout changes.

## What Changes

- A painter that prepares the areas of a frame SHALL keep a ring whose loaded style sheet resolves a
  border style and no fill style, and SHALL prepare that ring for the border its style sheet declares,
  instead of dropping the ring for the missing fill style.
- A ring that resolves neither a fill style nor a border style SHALL stay outside the frame, as before.

## Capabilities

### New Capabilities

None. The behaviour is the preparation contract of every painter that prepares areas for a frame, which
`map-painter-area-preparation` already owns.

### Modified Capabilities

- `map-painter-area-preparation`: the capability says a painter decides whether a ring is styled before it
  prepares the ring, but not what a ring whose only style is a border contributes. It gains one requirement,
  with two scenarios: the border-only ring that is kept, and the ring that resolves no style at all and is
  still not prepared. Its existing requirements keep their claim strength; the one that says an unstyled
  ring is not transformed is not delivered by this change and is not modified here.

## Impact

- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` - `ProcessAreas`: the ring loop's
  `if (!fillStyle) continue;` and the fill block it guards.
- `Tests/src/OpenGLAreaVisibilityTest.cpp` - the border-only style helper, the extraction of the
  offscreen context of the painter cases, and one case for the two scenarios.
- `TODO.md` - the §98 status becomes `fixed-by fix-opengl-border-only-rings`; the findings this change does
  not fix are reported for a later entry.

Not affected: the other painters, the public API of the OpenGL painter and of the core libraries, the
shader and font resource resolution, the database and file formats, the build descriptions of both build
systems (no file is added), and the prepared-area and draw-order contracts of the culling and preparation
capabilities.
