# Verification

## Diagnosis (tasks 1.1, 1.3)

Reproduced with the Dortmund database and the shipped `standard.oss`:

```bash
build/Tests/PerformanceTest --driver opengl --start-zoom 15 --end-zoom 15 \
  --icons libosmscout/data/icons/svg/standard \
  --font libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf \
  --shaders libosmscout-map-opengl/data/shaders \
  maps/Dortmund stylesheets/standard.oss 51.515 7.465 51.515 7.465
# before the fix: exit 139 (SIGSEGV), core dumped; after: exit 0
```

The polygons handed to the triangulation were captured with a temporary diagnostic in `Triangulate.cpp`
(`OSMSCOUT_DUMP_TRIANGULATION`, removed with the fix). Two classes reached the crash:

| input | before the fix | where |
|---|---|---|
| a node ring of **two points** (fixture `TwoPointRing()`) | `Release`: SIGSEGV; `Debug`: abort | `p2t::Sweep::NewFrontTriangle`; captured as `TRIANGULATION-DIAG Vertex2D ring(2): 7.4811195394480308,51.523777369078736 7.4811195394480308,51.523771532509144` |
| a **thin symbol rectangle** (fixture `ThinSymbolPolygon()`, a symbol polygon primitive mapped around a node, ~0.6 m x 0.05 m) | `Release`: SIGSEGV; `Debug`: `assert` in `p2t::Triangle::NeighborAcross`, `common/shapes.cc:353`; `gdb`: `#0 Triangle::EdgeIndex` <- `#1 Sweep::EdgeEvent` | `Sweep::EdgeEvent` rotates to a neighbour that does not exist and dereferences the null |

Measured behaviour of the vendored copy for the other degenerate classes: a ring with a repeated point trips
`common/shapes.h:139` (`// Repeat points`) in a Debug build and dereferences in `Release`; an all-collinear
ring throws `"EdgeEvent - collinear points not supported"` (`sweep/sweep.cc:130`, `:146`); a **self-crossing
ring is triangulated** (5 triangles for a bow tie) and is therefore *not* a rejection class — the spec's
scenario list was corrected for that before the fix (see `design.md`, decisions 2 and 3).

## What the fix does

- `Triangulate.cpp`: `NormalizeRing` drops a point that repeats its predecessor, a closing point that repeats
  the first point, and a point that is no corner of the ring; fewer than three remaining distinct corners is
  a rejection. All five entry points use it, every `p2t` call sits in a `catch (const std::runtime_error&)`
  that logs the reason and returns no triangle, and all four `Point`/`GeoCoord`/`Vertex2D` overloads and
  `TriangulateWithHoles` share one implementation.
- `Triangulate.h`: the contract is documented, and the class carries `OSMSCOUT_MAP_OPENGL_API` (it was hidden;
  `AreaVisibility.h:51` exports its tested function the same way).
- Vendored copy (marked as local deviations from upstream, referencing this change and TODO §97):
  `sweep/sweep.cc` — `Sweep::EdgeEvent` throws when it is handed no triangle; `common/shapes.cc` — the three
  null-neighbour asserts in `Triangle::NeighborAcross` throw instead.
- `MapPainterOpenGL.cpp` — the node path logs the object it skipped (`Skipping node <file offset>`) and draws
  the rest of the frame, the shape the area path already used.

## Falsification (the pre-fix runs)

| probe | result |
|---|---|
| the fix absent (before task 2.x): the captured two-point ring | `Release`: `PolygonTriangulationTest.cpp:121 FAILED: SIGSEGV`; `Debug` (meson): `p2t::Edge::Edge ... Assertion 'false' failed`, SIGABRT |
| `NormalizeRing` present, **no** `EdgeEvent` guard (before task 2.6): the captured thin rectangle | `Release`: `PolygonTriangulationTest.cpp:165 FAILED: SIGSEGV` |
| `EdgeEvent` guard present, `NeighborAcross` asserts untouched (before the second vendored patch): the same input in **Debug** | meson: `common/shapes.cc:353 ... Assertion 'neighbors_[1] != nullptr' failed`, SIGABRT |
| the goldens are pre-change values | the four triangle lists were captured from the branch-point code before the fix (task 2.3) |

## Scenario traceability

| Scenario (`specs/polygon-triangulation/spec.md`) | Case or observation |
|---|---|
| A ring of fewer than three distinct points is rejected | cases `A two-point ring is rejected instead of crashing`, `A ring of two distinct values repeated is rejected` |
| A ring that collapses to a degenerate shape is rejected | case `An all-collinear ring is rejected` |
| The geometry captured from the OpenGL node path is rejected rather than fatal | cases `A two-point ring …` and `A thin symbol polygon is rejected instead of crashing` (both captured fixtures) |
| A repeated closing point does not change the triangles | case `A repeated closing point does not change the triangles` |
| A duplicated vertex does not change the triangles | case `A duplicated vertex does not change the triangles` |
| The triangles of the shapes a renderer draws are unchanged | case `The triangles of an accepted polygon are unchanged` (triangle, square, L shape, square with a hole; values captured pre-change) |
| The frame's other objects are drawn after a rejected polygon | the three integration runs below: exit 0, 132 skipped node polygons, every level drawn |
| The skipped geometry is named in the report | the runs' log lines `Skipping node 680368, triangulation of its 2 points failed` (the painter) and `Skipping a polygon of 2 points: fewer than three distinct corners remain` (the triangulation), 132 each |
| The shipped area styles render on the Dortmund database again | the three runs below, each exit 0 across 21 zoom levels, against `exit 139` for the pre-change run |

## The integration runs (task 3.2)

```bash
for s in standard cycle winter-sports; do
  build/Tests/PerformanceTest --driver opengl \
    --icons libosmscout/data/icons/svg/standard \
    --font libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf \
    --shaders libosmscout-map-opengl/data/shaders \
    maps/Dortmund stylesheets/$s.oss 51.515 7.465 51.515 7.465
done
```

| stylesheet | before | after |
|---|---|---|
| `standard.oss` | exit 139 (SIGSEGV) at the zoom levels tried | exit 0, 264 skip lines, 21 levels |
| `cycle.oss` | SIGSEGV per TODO §97 | exit 0, 264 skip lines, 21 levels |
| `winter-sports.oss` | SIGSEGV per TODO §97 | exit 0, 264 skip lines, 21 levels |

## Build and test sweep (tasks 5.1, 5.2)

- `cmake --build build -j 8`: builds; the only warning in a touched file is the pre-existing
  `MapPainterOpenGL.cpp:454` `lineOffset` set-but-unused (TODO §93, untouched by this change) — the added
  code itself compiles warning-free.
- `QT_QPA_PLATFORM=offscreen ctest -j 4 --output-on-failure`: **145/145 passed** (twice, before and after the
  last vendored patch).
- `meson compile -C build-meson PolygonTriangulationTest OpenGLAreaVisibilityTest`: clean;
  `meson test "Check polygon triangulation" "Check OpenGL area visibility"`: both OK.
- `Tests/src/PolygonTriangulationTest.cpp`: 7 cases, 10 assertions, passing in the `Release` (CMake) and the
  `Debug` (meson, asserts active) configuration.
- Two repo checks caught defects in this change and were fixed: `check-vector-self-append`
  (`PolygonTriangulationTest.cpp` appended a vector element into the same vector — now copies first) and
  `HeaderCheckTest` (the module's only `osmscout/system` include went away with the rewritten
  `Triangulate.cpp`, so the table entry `osmscoutmapopengl => osmscout.system` was dropped). Both re-run green.
- `uncrustify -c .uncrustify -l CPP` on the touched files: the new test file was formatted (0 diff
  afterwards); the modified files keep their file-local style, and the formatter reports drift on every file
  regardless (TODO §80: 800 of 906 tracked files differ), so it is not a gate — `Triangulate.h` reports 2
  diff lines, `MapPainterOpenGL.cpp` 392 and the vendored `sweep.cc` 1270 before and after this change.

## Remaining residue (task 4.1)

The vendored copy still enforces some invariants with `assert`, which `Release` compiles out
(`sweep/advancing_front.cc:85` `assert(0)` and the remaining asserts). The two classes this change's
diagnosis hit now report by throwing, but any other assert-only invariant still turns invalid input into a
null dereference in a `Release` build. The `Debug` (meson) and sanitizer configurations are what keep those
asserts active today.

Entry text for the next `cleanup-todo` pass (it assigns the id):

> **The vendored poly2tri still enforces several invariants with `assert`, which `Release` compiles out**:
> `libosmscout-map-opengl/src/poly2tri/sweep/advancing_front.cc:85` (`assert(0)`) and the copy's remaining
> asserts. The two input classes the TODO §97 diagnosis found are covered by the guards added with
> `harden-polygon-triangulation` (`sweep/sweep.cc` `EdgeEvent`, `common/shapes.cc` `NeighborAcross`), but
> another assert-only invariant would still turn invalid input into a null dereference in a `Release` build.
> Updating the vendored copy to a revision that reports such input by throwing - or converting the remaining
> asserts as the two guards do - closes it; the `Debug` and sanitizer configurations keep the asserts active
> meanwhile.

## Limitations

- The OpenGL performance run crashed before the change, so task 2.4 has no pre-change step-timing baseline to
  compare against; the guard's cost is shown to be one linear pass plus the corner pass by reading the code.
- The `HeaderCheckTest` change narrows the module's declared allowed dependencies: `osmscoutmapopengl =>
  osmscout.system` is gone because no file of the module includes a system header any more. An alternative is
  to keep an (unused) system include; the table follows the code, so a later file that needs one adds it back.
- The exact drawn pixels are not compared; the case compares the returned triangle lists, which is what the
  renderer draws from.
