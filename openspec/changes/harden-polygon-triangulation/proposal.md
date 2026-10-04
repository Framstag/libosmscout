# Proposal

## Why

The OpenGL backend cannot render the area-drawing style sheets of a real database: `Tests/PerformanceTest
--driver opengl` with `maps/Dortmund` and `stylesheets/standard.oss`, `cycle.oss` or `winter-sports.oss`
dies with SIGSEGV at every zoom level and view size, with `gdb` placing the crash in the vendored
triangulator reached from the node path (TODO §97). Only `public-transport.oss` renders areas on that
data set, so the backend cannot be exercised with the shipped styles at all, and that is what blocked the
rendered comparison of `fix-opengl-area-visibility-cull`. The area path already survives the same class
of failure - it catches the triangulator's `std::runtime_error`, logs the skipped object and continues -
while the node path calls the triangulation without any guard, so one path's safety never reached the
other, and the triangulation itself trusts its input.

## What Changes

- Triangulating a polygon SHALL NOT terminate the process: a polygon the triangulation cannot handle
  SHALL be rejected (no triangles, no exception escaping to the caller), and the caller SHALL be able to
  continue drawing the frame.
- A polygon SHALL be normalized before it is triangulated, so that input which repeats points - a
  repeated closing point, a duplicated vertex - is triangulated rather than rejected, and the triangles
  do not depend on the repetition.
- The triangles of a polygon the triangulation accepts SHALL be unchanged, so this is a safety change and
  not a rendering change.
- A painter SHALL report the geometry it skipped and SHALL draw the rest of the frame, so a rejected
  polygon is visible to whoever runs the render instead of being a silent loss.
- The OpenGL backend's node and area paths SHALL share that behaviour, so the database and stylesheets
  above render.

Out of scope: replacing the vendored triangulator, changing which geometry is triangulated, and the
separate findings of that code (TODO §57 the OpenGL reprocessing cost, §98 the border-only rings and §99
the tolerance of the first border style).

## Capabilities

### New Capabilities

- `polygon-triangulation`: the contract of the triangulation a renderer uses to turn a polygon into
  triangles - what it accepts, what it does with input it cannot handle, and what a caller may rely on
  afterwards (a rejection is not fatal, the accepted triangles are stable, and normalization does not
  change the covered area).

### Modified Capabilities

None. The backend-level behaviour this change also restores (a painter draws the frame's other objects
after skipping one) is stated as a requirement of the new capability, because no existing capability
owns the triangulation a backend performs.

## Impact

Affected code:

- `libosmscout-map-opengl/include/osmscoutmapopengl/Triangulate.h` — the four `TriangulatePolygon`
  overloads and `TriangulateWithHoles`: normalization, rejection, and the logging of a rejected polygon.
  The signatures stay unchanged, and the class gains `OSMSCOUT_MAP_OPENGL_API` so that the already
  public header is actually linkable (the symbols were hidden, which is why no caller outside the
  library - or its test - could reach it; `AreaVisibility.h:51` exports the function its test calls
  in the same way).
- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` — the node path
  (`ProcessNode`, the triangulation call around `:925`), which today has no guard, brought in line with
  the area path's existing `catch`/skip/report around `:348`.
- `libosmscout-map-opengl/src/poly2tri/sweep/sweep.cc` — one guard, marked as a local deviation from
  upstream: `Sweep::EdgeEvent` dereferences the triangle of the edge it processes, and the sweep can
  lose that triangle when the neighbour it rotates to does not exist. Upstream crashes the caller there;
  the guard throws, which is how the library already reports the input it cannot sweep, so the
  triangulation's catch rejects the polygon and names it. The rest of the vendored copy is untouched,
  including its `assert`-based invariants that `Release` compiles out (`common/shapes.h:139`,
  `sweep/advancing_front.cc:85`).

Tests and build:

- `Tests/src/PolygonTriangulationTest.cpp` (new) — degenerate input, normalization, and the unchanged
  triangles of accepted polygons; no GL context needed by the triangulation itself.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — the new target, linking `OSMScout::MapOpenGL` as
  `OpenGLAreaVisibilityTest` does (`Tests/CMakeLists.txt:568`).
- `Tests/src/PerformanceTest.cpp` — only if the integration run needs a driver argument; the run itself
  is the verification, not a new test.

Not affected: the drawing output of the other backends, the stylesheets, and the area path's existing
skip behaviour except where it now shares the triangulation's guard.
