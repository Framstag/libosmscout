## Why

The OpenGL painter's area step prepares every loaded area of a view and gives the per-ring visibility
decision a tolerance the loaded stylesheet declares in millimetres, while that decision expects a length
in the frame's pixels - the defect `fix-area-cull-pixel-tolerance` fixed in the core painter. It also
performs its per-ring geometry work for rings it discards, and copies and sorts the whole loaded area
list on every call. Measured on the fixed zoom-15 view of the Dortmund database, which loads 17 629
areas and shows 726 rings: 87 ms and 156 621 allocations per call against 5 ms and 158 for the equivalent
core step.

Two consequences. The wrong unit makes the decision drop rings whose border still crosses the viewport
edge - the error grows with the DPI of the frame, about 3.8x at 96 DPI and about 11.8x at 300 DPI by the
earlier change's measurement - so the OpenGL output is missing borderline geometry that the other
backends draw. And in `OSMScoutOpenGL` the step runs on a worker thread for every asynchronous data load,
so the same work is pan latency.

`TODO.md` has recorded this since the area-culling work and it was not part of it. Why now: the OpenGL
backend is the last painter that is not held to the culling and preparation contracts the core painter
already satisfies, and the test harness that can verify a change to it was just repaired
(`fix-opengl-install-destination`).

## What Changes

- A visibility tolerance a painter derives from a width a stylesheet declares SHALL be applied in the
  frame's pixels in the OpenGL painter as well, in the per-ring decision and in any area-level early
  rejection, so the tolerance grows with the DPI of the frame.
- The OpenGL painter SHALL decide visibility before it performs per-ring geometry work, so a ring the
  view cannot show costs no duplicate-point removal, no ring copy and no triangulation.
- The per-call work of the OpenGL area step SHALL NOT grow with the loaded areas beyond the areas it
  inspects: no copy of the whole loaded area list per call, and no per-ring container sized by the
  loaded rings.
- The two culling and preparation contracts SHALL say that they bind every painter that prepares areas
  for a frame, not only the painter of `libosmscout-map`, so the next backend cannot miss them.
- Rendered output at the viewport edge changes deliberately: an area whose border crosses the edge
  contributes its border pixels, as it already does in the other backends. Output strictly inside the
  view, the prepared area set, the draw order and the draw order of the backend are unchanged.
- Explicitly not changed: the public API of the OpenGL painter, the shader and font resource resolution
  repaired by `fix-opengl-install-destination`, and the unrelated Release-build
  `-Wunused-but-set-variable` warning in the same translation unit, which stays with the `TODO.md` entry
  that records it.

## Capabilities

### New Capabilities

None. The behaviour this change corrects is already claimed by the painter culling and painter
preparation capabilities; a new capability would restate them for one backend and duplicate their
requirements.

### Modified Capabilities

- `map-painter-area-culling`: its requirements are stated for "the painter", which readers could take as
  the painter of `libosmscout-map`, and the backend painter that owns its visibility decision was
  never held to them. The capability gains one requirement that binds every painter preparing areas
  for a frame to apply a stylesheet-derived tolerance in the frame's pixels, with three scenarios: the
  borderline border of such a painter, the DPI dependence of its tolerance, and an area beyond the
  converted tolerance.
- `map-painter-area-preparation`: the same gap applies to the requirements that visibility precedes
  per-ring geometry work and that the step does not allocate per loaded area. The capability gains one
  requirement that the per-ring work of every painter follows the rings it keeps and that its per-call
  allocation does not grow with the loaded areas, with three scenarios: a discarded ring costing no
  per-ring geometry work, the allocation bound, and the unchanged preparation of kept rings.

## Impact

Affected code:

- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` - `ProcessAreas` including its
  per-call copy and sort of `data.areas`/`data.poiAreas`, the ring preparation path, and the
  `IsVisibleArea` call that receives the declared border width.
- `libosmscout-map-opengl/include/osmscoutmapopengl/MapPainterOpenGL.h` - only if the step needs a
  scratch member or a diagnostic counter; no public API change is intended.
- `Tests/src/PerformanceTest.cpp` - the driver that reports the step's cost; `Tests/CMakeLists.txt` and
  `Tests/meson.build` if a target or its arguments change.
- `TODO.md` - remove the closed entry and record whatever the change finds that it does not fix.

Not affected: any other backend, the public API of the core and OpenGL libraries, the database and file
formats, and the dependencies of either build system. The CMake and Meson descriptions stay in step.

Related capability that is exercised but keeps its requirements: `opengl-performance-test`, whose
harness runs the measurement (shaders and a font still have to be supplied for it to run at all).
