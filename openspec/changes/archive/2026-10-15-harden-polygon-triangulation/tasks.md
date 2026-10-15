# Tasks

## 1. Diagnosis and the regression fixture

- [x] 1.1 Reproduce the crash with the vendored asserts active: build a Debug (or sanitizer) configuration and run `Tests/PerformanceTest --driver opengl` over `maps/Dortmund` with `stylesheets/standard.oss` (the trigger of TODO §97, which also needs `--shaders` and a `--font`), or run the failing node path directly, and capture the ring that reaches `p2t` and the first invariant that fires (spec: `polygon-triangulation`, *geometry captured from the OpenGL node path*).
  Verify: the failing ring is written down as a literal point list, and feeding exactly that list to `osmscout::Triangulate::TriangulatePolygon` reproduces the failure in that configuration without a database.
- [x] 1.2 Create `Tests/src/PolygonTriangulationTest.cpp` as a Catch2 target linking `OSMScout::MapOpenGL` the way `Tests/CMakeLists.txt:568-569` (`OpenGLAreaVisibilityTest`) does, register it in `Tests/CMakeLists.txt` and `Tests/meson.build`, add the captured ring as its first case, and export the class (`OSMSCOUT_MAP_OPENGL_API` on `Triangulate`, as `AreaVisibility.h:51` does for the function its test calls) so the target can link the hidden symbols (spec: *a polygon the triangulation cannot handle is rejected, not fatal*, first scenario).
  Verify: `cmake --build build --target PolygonTriangulationTest` and `meson compile -C build-meson PolygonTriangulationTest` both build, and the new case fails or crashes before the fix, which is what makes it the guard for tasks 2.1-2.2.
- [x] 1.3 Record in the change's `verification.md` what the diagnosis found: the input class that reaches the compiled-out invariant, the frames `gdb` reports, and the command that reproduced it (spec: *rejected, not fatal*).
  Verify: `verification.md` names the configuration used, the reproducing command and the failing point list, so the diagnosis is repeatable from the file alone.

## 2. The triangulation normalizes, rejects and reports

- [x] 2.1 Normalize the polygon inside `Triangulate.cpp` before it is handed to `p2t`: drop a point equal to its predecessor, drop a closing point equal to the ring's first point, drop a point whose neighbours make it redundant, and reject a ring with fewer than three distinct points (spec: *a polygon is normalized before it is triangulated*, both scenarios).
  Verify: new cases in `Tests/src/PolygonTriangulationTest.cpp` — a square with a repeated closing point equals the four-point square, a ring with a duplicated vertex equals the ring without it, and a two-point ring yields no triangle and no crash; `ctest -R PolygonTriangulation` passes.
- [x] 2.2 Reject instead of terminating: wrap the `p2t::CDT` call in every `Triangulate` entry point in `catch (const std::runtime_error&)`, log the reason at `Warn` and return an empty triangle list, keeping the existing public signatures and covering all four overloads plus `TriangulateWithHoles` (spec: *rejected, not fatal*, first and second scenarios, and the traceability of the diagnosis fixture from task 1.2).
  Verify: the fixture case of task 1.2, a ring whose points are two distinct values repeated, and an all-collinear ring each return no triangle; the test process exits with code 0 in both a Release and a Debug build; `ctest -R PolygonTriangulation` passes.
- [x] 2.3 Pin the triangles of accepted polygons: record the triangle lists of a triangle, a square, an L-shaped polygon and a polygon with a hole at the branch point and assert equality after the change (spec: *the triangles of an accepted polygon are unchanged*).
  Verify: the golden cases compare equal before and after in `Tests/src/PolygonTriangulationTest.cpp`, and the recorded lists are committed as the expected values.
- [x] 2.4 Keep the guard cheap: implement the normalization as a single pass over the ring and do not copy the quadratic duplicate removal of `MapPainterOpenGL.cpp:325-334` (design decision 3 and its cost risk).
  Verify: reading `NormalizeRing` shows one linear pass plus the corner pass over the ring and no nested loop; the run's step timings are recorded in the change's `verification.md` instead of being compared against a pre-change number, because the pre-change run of that driver crashed (task 3.2).
- [x] 2.5 Document the reject/normalize contract where the entry points are declared, naming what a caller may rely on (spec: all four requirements of `polygon-triangulation`).
  Verify: `libosmscout-map-opengl/include/osmscoutmapopengl/Triangulate.h` states that a rejection yields no triangles without throwing and that the input is normalized, and no comment claims the triangulation trusts its input.

- [x] 2.6 Guard the one place the vendored sweep dereferences a lost triangle: `Sweep::EdgeEvent` (`libosmscout-map-opengl/src/poly2tri/sweep/sweep.cc:113`) throws when it is handed no triangle, marked as a local deviation from upstream, so the triangulation's catch rejects the polygon instead of the process dying (spec: *rejected, not fatal*, first and third scenarios; design decision 6).
  Verify: the captured thin symbol polygon returns no triangle instead of crashing (`ctest -R PolygonTriangulation`), and the three integration runs of task 3.2 exit 0.

## 3. The painter continues the frame and reports what it skipped

- [x] 3.1 Guard the node path: in `MapPainterOpenGL::ProcessNode` (the triangulation call around `MapPainterOpenGL.cpp:925`) skip the object, log which object was skipped, and continue the frame — the shape the area path already uses around `:348` (spec: *a painter continues the frame and reports what it skipped*, first and second scenarios).
  Verify: the run of task 3.2 completes and its log names the skipped object next to the area path's existing `Skip area` lines; reading the node path shows no unguarded triangulation call.
- [x] 3.2 Run the documented trigger: `Tests/PerformanceTest --driver opengl` over `maps/Dortmund` with `stylesheets/standard.oss`, `cycle.oss` and `winter-sports.oss`, with the shaders and a font that exists on the machine, at more than one zoom level (spec: *the shipped area styles render on the Dortmund database again*, and TODO §97's recipe).
  Verify: each run exits with code 0 and no SIGSEGV, and its log shows areas being drawn rather than skipped wholesale; the pre-change state (SIGSEGV at every zoom level tried) is quoted next to the result.

## 4. The residue in the vendored triangulator

- [x] 4.1 Record that the vendored `poly2tri` enforces its invariants with `assert` and that `NDEBUG` compiles them out (`libosmscout-map-opengl/src/poly2tri/sweep/advancing_front.cc:85`), so a polygon the validation and the library's own detector both miss dereferences instead of throwing; name the Debug/ASan configurations as the check that keeps those invariants active, and hand the entry text to the next `cleanup-todo` pass (it assigns the id).
  Verify: the note exists in the change's `verification.md` with the `file:line`, and the proposed TODO entry names the residual input class and the follow-up of design decisions 2 and 3.

## 5. Integration verification

- [x] 5.1 Both build systems compile the touched files without warnings.
  Verify: `cmake --build build` and `meson compile -C build-meson` complete and report no warning for `Triangulate.cpp`, `Triangulate.h`, `MapPainterOpenGL.cpp` or the new test file.
- [x] 5.2 The existing test suite passes, so nothing regressed outside the touchpoint.
  Verify: `cd build && QT_QPA_PLATFORM=offscreen ctest -j 2 --output-on-failure` (or `xvfb-run`) and `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` report the same failures as before the change (none expected), and `OpenGLAreaVisibilityTest` in particular stays green.
- [x] 5.3 Write the change's `verification.md`: every scenario of `specs/polygon-triangulation/spec.md` mapped to a case or an observation, the integration run quoted, the mutations that make the new cases fail, and the formatter/static-analysis check on the touched files (`.uncrustify`, `.clang-tidy`).
  Verify: `openspec validate harden-polygon-triangulation --strict` passes and `verification.md` names the case or command behind each of the nine scenarios.
