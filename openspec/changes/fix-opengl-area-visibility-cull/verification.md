# Verification

What was run for `fix-opengl-area-visibility-cull`, on which tree, and what the result was. The change
touches the OpenGL backend only; every command below was run in
`/home/tim/projects/libosmscout` on the branch `fix-opengl-area-visibility-cull`.

## Environment

| item | value |
|---|---|
| build | `build/` (CMake, Ninja, `CMAKE_BUILD_TYPE=Release`, `OSMSCOUT_BUILD_MAP_OPENGL=ON`) |
| second build system | `build-meson/` (Meson, `unity=on`) |
| sanitizer build | `build-asan/` (clang/gcc ASan+UBSan, as documented in `AGENTS.md`) |
| GPU | none; GLFW offscreen context, run under `xvfb-run` where a display was needed |
| database used | `maps/Dortmund` (no `db.json`, opened as a directory by `PerformanceTest`) |

## 1. The predicate and its tolerance (tasks 1.1-1.5, specs `map-painter-area-culling`)

`Tests/src/OpenGLAreaVisibilityTest.cpp`, 5 cases, 101 assertions, no GL context needed for four of
them:

```bash
cmake --build build --target OpenGLAreaVisibilityTest
cd build && ctest -R OpenGLAreaVisibilityTest --output-on-failure   # Passed
meson compile -C build-meson OpenGLAreaVisibilityTest
meson test -C build-meson --print-errorlogs "Check OpenGL area visibility"   # Ok: 1, Fail: 0
```

The cases assert the borderline behaviour at 96 and 300 DPI, the DPI dependence (an area between the
two converted tolerances is kept at 300 DPI and rejected at 96 DPI), that an area beyond the tolerance
contributes nothing, and the smallest-drawn-dimension rejection.

**Negative control.** Replacing the conversion in `AreaVisibility.cpp` with
`double pixelOffset=borderWidthMM/2.0;` (the defect this change fixes) makes the suite fail:

```
1/1 Test #128: OpenGLAreaVisibilityTest .........***Failed
```

With the conversion restored, it passes again. The test therefore detects the defect it was written
for, rather than passing for an unrelated reason.

## 2. The per-ring work of the step (tasks 2.1-2.4, `map-painter-area-preparation`)

The fifth case drives `MapPainterOpenGL::ProcessData` through an offscreen GLFW context, as
`PerformanceTestBackendOGL` creates one, with three areas outside the view and one inside it:

```
CHECK( painter.GetExaminedRingCount()==1+outsideAreaCount )   -> 4 == 4
CHECK( painter.GetKeptRingCount()==1 )                        -> 1 == 1
```

**Skip behaviour.** With `glfwCreateWindow` forced to return `nullptr`, the case reports
`test cases: 1 | 0 passed | 1 skipped` and `ctest` reports the test as *Passed* - it skips rather than
fails when no context can be created.

## 3. Measurement on a real database (task 4.1)

Fixed view: `maps/Dortmund`, `stylesheets/public-transport.oss`, zoom 15, bounding box
`51.53 7.44 - 51.50 7.49` (30 tiles).

```bash
./build/Tests/PerformanceTest-1.1.1 --debug --driver opengl --start-zoom 15 --end-zoom 15 \
  --shaders libosmscout-map-opengl/data/shaders \
  --font libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf \
  --icons libosmscout/data/icons/svg/standard \
  maps/Dortmund stylesheets/public-transport.oss 51.53 7.44 51.50 7.49
```

| | loaded data | rings examined | rings kept | draw allocations | step that calls `ProcessData` |
|---|---|---|---|---|---|
| before (`git stash` of the source changes) | nodes 6037, ways 108334, areas 2456 | not observable (no counter) | not observable | 456 566 | total 766.09 ms, avg 25.54 ms/tile |
| after | nodes 6037, ways 108334, areas 2456 | 2571 | 143 | 454 116 | total 624.57 ms, avg 20.82 ms/tile |

The loaded data is identical before and after, as the task requires. The allocation count is
deterministic; the step time is not interleaved between the two builds and is reported with that
caveat.

What the numbers mean:

- The **allocation difference of 2450** is the per-ring node copy (`std::vector<Point> p=ring.nodes`)
  that no longer happens for the rings the decision discards: 2571 rings are examined, 143 are kept, so
  about 2428 copies disappear, which accounts for the difference.
- The **per-ring geometry work now runs for 143 rings instead of for every styled ring** - a 94 %
  reduction of that work on this view. The node copy, the quadratic duplicate-point removal and the
  clipping-ring copy are skipped for the other 2428 rings.
- On the single-tile view of the same database (`51.5145 7.4645 - 51.5140 7.4650`, 107 areas) the step
  examines 113 rings and keeps 3, and its draw allocation count is 21 811 - the same value with the
  `ScreenBox`-based predicate as with the earlier hand-written screen-box comparison, which is the
  check that the rewrite of the predicate did not change which rings are kept.

## 4. Cross-backend rendering comparison (task 4.2) - performed, no difference observed

The task asked for a rendered comparison showing the border pixels of an area whose border crosses the
viewport edge in the OpenGL output after the change and not before. The comparison was performed; **it
found no difference**, so that half of the expected evidence does not exist.

Method: `Demos/DrawMapAll` (Cairo, OpenGL, Qt, SVG and Skia from one invocation) on `maps/Dortmund`
with `stylesheets/public-transport.oss`, level 15 (`zoom = 2^15 = 32768`, see below), 400x400, and a
grid of views. Before-images were rendered from the same command with the source changes stashed, then
compared pixel by pixel.

| sample | views | OpenGL pixels differing before/after | Cairo control |
|---|---|---|---|
| 96 DPI, 3x3 grid around 51.515/7.465 | 9 | 0 | 0 |
| 300 DPI, centre 51.515/7.465 | 1 | 0 | 0 |
| 300 DPI, 3x3 grid, offsets +/-0.0012 deg | 9 | 0 | 0 |

The Cairo and Qt images are identical before and after in every sample, which is the control: neither
painter is touched by this change, so a comparison method that reported differences there would be
wrong. The Qt images were checked for the 300 DPI centre view as well.

**The tolerance conversion changes no decision on these views.** Running the fixed single-tile view
(`51.5145 7.4645 - 51.5140 7.4650`) through the driver with the counters, once with the conversion and
once with the width left in millimetres (the defect), gives the same result both times:
`examined 113, kept 2`. The sampled views simply contain no ring whose bounding box falls inside the
band between the millimetre value and the converted value (for a border of `w` millimetres that band is
`w/2` pixels wide at the frame's DPI), so no decision differs. (The kept count of the same view at 96
DPI is 3 rather than 2; that is the smallest-drawn-dimension check, whose converted value grows with
the DPI, not the tolerance.)

The unit-test case of section 1 constructs exactly the ring that band needs, which is why the contract
and the defect stay verifiable without a data set that happens to place a ring there.

### 4.1 What this means for the spec scenario

The added scenario *"A painter that owns its decision keeps the border of an area crossing the edge"*
expects the frame to **contain the border pixels** of an area within the converted tolerance. That
expectation was not observed: keeping a ring whose bounding box reaches the view by the tolerance does
not by itself put its border pixels into the frame, because the area's geometry is clipped to the frame
and a border that lies wholly outside it contributes nothing. The scenario is therefore worded for
something the sampled data cannot show, and it is *not* refuted - the samples contain no area at that
margin. This is left for the reviewer (see the change summary): either the scenario narrows to what the
contract can guarantee (the ring reaches preparation and its geometry is prepared), or the question
"does a backend draw the border pixels of an area that only reaches the view by the tolerance" becomes
its own investigation.

## 5. Suites and sanitizer (tasks 4.3, 4.4)

Run on the final tree, after the predicate was moved onto
`Projection::BoundingBoxToPixel`/`ScreenBox` (see section 8):

| command | result |
|---|---|
| `xvfb-run -a ctest -j 2 --output-on-failure` in `build/` | **140/140 passed**, 48.9 s |
| `meson compile -C build-meson` then `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` | **140/140 Ok, 0 Fail** |
| `cmake --build build-asan -j 4` | 382 targets, no error |
| `ctest -j 2 --output-on-failure --exclude-regex PerformanceTest` in `build-asan/` | **94/97 passed**; the 3 failures are the pre-existing leak class (section 5.1). `OpenGLAreaVisibilityTest` passes under ASan (0.99 s) |

After the two cosmetic edits of section 8 (`borderReachFactor`, the `NOLINTNEXTLINE`, the test's
includes) the CMake suite was run again (140/140), the Meson suite was run again (140/140 Ok, 0 Fail)
and `HeaderCheckTest` and `OpenGLAreaVisibilityTest` were run again in `build-asan` (both passed); the
full sanitizer numbers of the table are the run before those edits.

### 5.1 The three sanitizer failures are pre-existing and not from this change

`MapPainterShieldTest`, `TextMetricsCairoTest` and `TextMetricsSVGTest` each fail with
`LeakSanitizer: detected memory leaks`, and the allocation frames are dominated by
`libpango-1.0.so.0` (13514 frames), `libfontconfig.so.1` (11971) and `libpangoft2-1.0.so.0` (4187)
against 23145 `libosmscout` frames that are the reachable stack, not the allocation sites. This is the
class `TODO.md` already records under *"LeakSanitizer reports from fontconfig/pango fail three
text-rendering tests in the sanitizer build"*, reproduced here a second time. `build-asan` is a Debug
build without the `xvfb` display, which is the configuration that entry describes.

### 5.2 A finding of the suites: a forbidden package dependency

The first run of the CMake suite, with the initial predicate implementation, failed `HeaderCheckTest`:

```
File '.../libosmscout-map-opengl/src/osmscoutmapopengl/AreaVisibility.cpp in package
'osmscoutmapopengl' has forbidden dependency to package 'osmscout'
```

The cause was an explicit `#include <osmscout/Pixel.h>` for `Vertex2D`. The allowed dependencies of
the package are the sub-packages (`osmscout.util`, `osmscout.projection`, `osmscoutmap`, ...), not
`osmscout` itself. The predicate now projects with `Projection::BoundingBoxToPixel` and enlarges a
`ScreenBox`, the same shape the core painter's `IsPainter`-side decision uses, and includes only
`osmscout/util/GeoBox.h`, `osmscout/util/ScreenBox.h` and `osmscout/projection/Projection.h`.
`HeaderCheckTest` passes; the fixed-view counters are unchanged by the rewrite (section 3).

## 6. Public API

The only change to the test tool is `Tests/src/PerformanceTest.cpp`: the OpenGL driver prints the two
counters after `ProcessData`, so a run reports the examined and kept ring counts of a real view
(section 3).

The change adds two public getters to `MapPainterOpenGL`, `GetExaminedRingCount()` and
`GetKeptRingCount()`, both documented in the header as diagnostics for tests. `AreaVisibility.h` is a
new installed header of `libosmscout-map-opengl`; it is installed by the
`install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/${_includedir} ... PATTERN "*.h")` rule of
`cmake/ProjectConfig.cmake:121`, so no separate install rule was needed. The file lists of both build
systems were extended, so CMake and Meson build the same library and the same test.

## 7. AGENTS.md (task 5.3)

Not updated. This change adds one header/source pair to an existing library and one test to the
existing test suite; it changes neither the project structure, the build options, the CI configuration
nor the platform list, which is what `AGENTS.md` documents. The two build systems' file lists are the
part that had to stay in step, and `Tests/CMakeLists.txt` and `Tests/meson.build` both carry the test.

## 8. Conventions and backlog (tasks 5.1, 5.2)

**TODO.md.** The entry *"OpenGL backend reprocesses all loaded areas per data load"* is removed; this
change closed it. Four findings were added under a new group for this change: the OpenGL crash in the
node triangulation (section 4), the `DrawMapAll` view (section 4), the border-only ring skip, and the
tolerance that covers only the first border style. The stale `file:line` of the `lineOffset` entry was
updated (`:466`/`:492`/`:496` -> `:454`/`:480`/`:484`), and the LeakSanitizer entry gained this
change's second reproduction.

**Formatting.** `uncrustify -c .uncrustify -l CPP --check` reports **0 hunks** for all five touched C++
files (`AreaVisibility.h`, `AreaVisibility.cpp`, `OpenGLAreaVisibilityTest.cpp`, `MapPainterOpenGL.h`,
`MapPainterOpenGL.cpp`) and for the same two existing files at `origin/master`, i.e. neither new drift
nor pre-existing drift in the files this change touched. (`scripts/format-check.sh check` covers the
whole tree and is not usable as a per-file gate, as `TODO.md` records.)

**clang-tidy.** `clang-tidy -p build` reports **0 findings located in `AreaVisibility.cpp`** after the
two it reported were addressed: the magic number `2.0` of the halving became the named constant
`borderReachFactor`, and the two swappable `double` parameters carry a `NOLINTNEXTLINE` whose comment
says why (their units are part of their names). The test file keeps 18 findings in its own lines, all
in classes the untouched `Tests/src/MapPainterAreaVisibilityCullTest.cpp` carries 144 times in the same
directory (magic numbers, `modernize-avoid-c-style-cast`, `modernize-use-emplace`,
`modernize-return-braced-init-list`, `readability-math-missing-parentheses`,
`bugprone-easily-swappable-parameters`, `readability-function-cognitive-complexity`), so they are the
repository's accepted baseline rather than drift this change introduced; the three
`misc-include-cleaner` findings the file had were fixed (unused `<string>` removed, `<cstddef>` and
`<osmscout/Pixel.h>` added).

**Coding style.** The new files follow `guidelines/CodeStyles.md`: `OSMSCOUT_<MODULE>_<NAME>_H` guards,
the include order (own header, standard library, export macro, project headers), 2-space indentation,
Doxygen with `@param`/`@return` on the new public function and the new getters, and `Ref` aliases with
`nullptr` where pointers are used.

## 9. What a reviewer should look at

- The counter increments: `examinedRingCount` before the decision, `keptRingCount` where the geometry
  is prepared (`MapPainterOpenGL.cpp`), so `kept` is not merely a second name for `examined`.
- The predicate's rejection order: the smallest-drawn-dimension test comes before the intersection,
  as in the code it replaces.
- That `sortedAreas` is cleared and reserved on every call, and that the sort - the draw order - is
  still applied.

## 10. CI registration of the test (follow-up on this branch)

The registration of section 1 was written on a machine with GLFW installed. The OpenGL condition of
*the library* (GL, GLM, GLEW, FreeType) does not imply GLFW on the CI runners, and the first run of
those jobs after the implementation commit failed in three ways:

- The Ubuntu Meson jobs do not install `libglfw3-dev` (the CMake job does). `Tests/meson.build`
  registered the test under `if buildMapOpenGL` alone, so it compiled against `GLFW/glfw3.h` and
  stopped with `fatal error: GLFW/glfw3.h: No such file or directory`. The guard now also requires
  `glfwDep.found()`, the same condition the OpenGL driver of `PerformanceTest` uses, which is what
  task 1.5 asked for.
- On MinGW `glfwDep` comes from `compiler.find_library('glfw')`, which does not match the package's
  `libglfw3`; the header still resolves through the default `/mingw64/include`, so the same guard
  keeps the Meson job from failing the link with `undefined reference to 'glfwSetErrorCallback'`.
- The MSYS CMake job finds GLFW and builds the test, but the test step's `PATH` did not name
  `libosmscout-map-opengl`; the process exited with `0xc0000135` (`STATUS_DLL_NOT_FOUND`). The `PATH`
  now names that directory like the other library directories of that step.

Verification: `Tests/meson.build` was configured with a `pkg-config` wrapper that fails only for
`glfw3*`. The resulting target list has no `OpenGLAreaVisibilityTest` while `libosmscout-map-opengl`
stays on, and the target is back with the real `pkg-config`. The Windows and macOS jobs verify the
`PATH` and the skip behaviour themselves.

## Host verification 2026-10-16 (map-dependent items)

Task 4.2 was attempted again. **A GL context can be created headlessly here** — the blocker the task assumed
("no GL context") does not apply on this machine:

```bash
xvfb-run -a ./build/Tests/OpenGLAreaVisibilityTest-1.1.1     # All tests passed (148 assertions in 8 test cases)
```

All eight cases ran; the fifth case (`The per-ring work of the area step follows the rings it keeps`)
creates the offscreen GLFW context and reached its checks (`GetExaminedRingCount()==1+outsideAreaCount`,
`GetKeptRingCount()==1`), so it did not take its skip branch.

The cross-backend render was then performed on the v27 database `maps/nordrhein-westfalen`, one view
through both drivers with the same stylesheet, size and shaders (command in the retained log):

| sample | result |
|---|---|
| view | 400x400, `stylesheets/standard.oss`, centre 51.515/7.465, magnification 32768, `--database maps/nordrhein-westfalen` |
| Cairo vs OpenGL, whole image | `compare -metric AE` = `12838.4` of `160000` (`8.02 %`) |
| Cairo vs OpenGL, inner 384x384 (`-shave 8x8`) | `11787.3` of `147456` (`7.99 %`) |
| 8-px frame (by difference) | `1051.1` of `12544` (`8.4 %`) — the same rate as the interior, no localised border effect |
| distinct colours | Cairo `18578`, OpenGL `19248` |

Read: the two drivers differ globally (labels, anti-aliasing), and no border of an area is present in one
and absent in the other in the frame; there is no localised border-pixel difference to attribute to this
change. **The before/after statement 4.2 asks for** — the border pixels of an area crossing the viewport edge
"appear in the OpenGL output after the change and not before" — is not what this cross-driver sample
shows; it needs a second build of `libosmscout-map-opengl` with a source hunk reverted, which this run's
working rules did not allow. The section *Host verification 2026-10-16 (task 4.2, before/after)* below
was run afterwards with that second build and carries the statement, on the hunk whose revert changes
the border pixels at all - not on the tolerance band, for which section 4.1 keeps its finding that the
sampled data has no ring in it.

A tool note for anyone repeating this: `DrawMapAll`'s third positional argument is a magnification
*factor*, not a level — `15` renders the whole world (400x400 mostly empty, `937` B PNG), `32768` renders
level 15. The comparison above uses `32768`.

Log: `evidence/opengl-cairo-cross-backend-2026-10-16.log` (commands, UTC timestamp, exit 0, revision
`06b197a35`). The before/after half of the task was produced afterwards, in the section below.

## Host verification 2026-10-16 (task 4.2, before/after)

Revision `3b247dce5` (branch `openspec-host-verification-2026-10-16`), build `build/` (CMake, Ninja,
Release), database `maps/Dortmund`, stylesheet `stylesheets/standard.oss`, 400x400, GL context created
headlessly under `xvfb-run`. Log: `evidence/opengl-border-before-after-2026-10-16.log` (every command,
the UTC timestamps, the exit codes and the numbers; the PPMs themselves live in
`build/probe42-2026-10-16/{before,after}/`).

**The before state.** A temporary revert of exactly the source hunk of the archived change
`2026-10-10-fix-opengl-border-only-rings` (implementation commit `e6d24f998`), marked
`TEMP-REVERT-PROBE` on the reverted lines: `if (!fillStyle) { continue; }` back in front of the fill
guard of the ring loop of `MapPainterOpenGL::ProcessAreas` (one file, 7 added lines).
`cmake --build build --target OSMScoutMapOpenGL DrawMapOpenGL DrawMapAll` exited 0 at 15:41:30Z
(4.4 s), the render of the before state ran at 15:42:07Z (exit 0, 16.2 s). The restore
(`git checkout -- …/MapPainterOpenGL.cpp`) was at 15:42:25Z, the rebuild of the same targets exited 0
at 15:42:30Z (4.2 s, the library and the tool timestamped 15:42:29 - after the restored source at
15:42:25), and the render of the after state ran at 15:42:32Z (exit 0, 16.1 s). `git status
--porcelain` after the probe names `?? maps/repository/` only and `grep -rn TEMP-REVERT-PROBE
libosmscout-map-opengl/ Tests/` finds nothing.

What the two states isolate is the border-only ring path only: the visibility decision of *this*
change is in both of them, and the reverted hunk only decides whether a ring that resolves no fill
style reaches the border loop. The tolerance band of the conversion that *this* change fixes is not
what these two images differ by; §4 and §4.1 keep their finding that the sampled data has no ring in
that band.

**The views and how the areas were located.** The closed ways of `maps/Dortmund.osm.pbf` were dumped
(`osmconvert maps/Dortmund.osm.pbf > build/probe42-2026-10-16/dortmund.osm`) and their rings projected
with the mapping of `MercatorProjection::GeoToPixel` (`MercatorProjection.cpp:192`, scale and
`groundWidthEquatorMeter` from `MercatorProjection.cpp:93-112`, `tileDPI = 96`). That predicts, before
any render, where a ring crosses the frame edges; the render then confirmed the prediction to below
one pixel (see claim 1).

| view | area (OSM way) | driver arguments | its border crosses the viewport |
|---|---|---|---|
| D | `boundary_protected_area` "Siedlung Oberdorstfeld" (way 367958283), drawn by a border only at this magnification | 96 dpi, 51.5030 7.4210, magnification 32768 | left edge, at y=179.3 and y=346.6 (predicted) |
| B | `barrier_fence` (way 24414688), `AREA.BORDER` 0.1 mm and no `AREA` fill | 300 dpi, 51.531599 7.519315, magnification 65536 | right edge, at y=77.7 and y=308.9 (predicted) |
| E | `boundary_protected_area` "NSG Waldsiepen" (way 403926788) | 96 dpi, 51.4375 7.2690, magnification 32768 | wholly inside the frame (second instance) |
| A | fence way 17707818 | 300 dpi, 51.499275 7.478769, magnification 65536 | - (`0` differing pixels: that way is not a loaded area) |

**Claim 1 - the border pixels are present in the after state and absent in the before state.** Count
of differing pixels of the 400x400 frame, from the two PPM rasters with
`convert before/<v>.ppm after/<v>.ppm -compose difference -composite -threshold 0 -format
"%[fx:int(mean*w*h+0.5)]" info:`:

| view | differing pixels | difference bounding box | distance of the differing pixels to the projected ring of the area |
|---|---|---|---|
| D | 368 of 160000 (0.230 %) | x 0..86, y 179..346 | max 0.78 px, none farther than 4 px |
| B | 690 of 160000 (0.431 %) | x 200..399, y 76..387 | max 1.25 px, none farther than 4 px |
| E | 564 of 160000 (0.352 %) | x 95..239, y 156..277 | max 0.75 px, none farther than 4 px |

View D: the difference bounding box starts at x=0 and spans y 179..346, which are exactly the two
places where the ring crosses the left viewport edge: the difference *is* the stretch of the area's
border that enters and leaves the frame through that edge. At the differing pixels the before image
shows the map content and the after image the border colour - the most frequent pairs are
`(235,235,235) -> (179,199,170)`, `-> (170,194,160)`, `-> (210,219,206)`; for the fence of view B
`(241,238,233) -> (239,236,231)`, `-> (223,221,217)`, `-> (214,212,209)`. The difference is on the
border of the identified area and not scattered: no differing pixel is farther than 0.78 px (D) or
1.25 px (B) from that area's projected ring.

**Claim 2 - the output strictly inside the view is unchanged.** The whole difference bounding box lies
outside a sub-rectangle of the frame that keeps map content in it, and that sub-rectangle differs in no
pixel:

| view | sub-rectangle | differing pixels of the sub-rectangle |
|---|---|---|
| D | `-crop 280x400+120+0`, i.e. x 120..399, 33 px right of the difference bbox | 0 |
| B | `-crop 200x400+0+0`, the left half | 0 |

`convert <v>.ppm -crop <geometry> +repage b.png`, the same for the after image, then the
`-threshold 0` count above. Together with the corridor check of claim 1 this says: the only pixels
that change between the two states are the border pixels of the area whose border crosses the
viewport edge; the rest of the frame, the interior of that area included, is byte-identical.

**Control.** The Cairo, the Qt, the Skia and the SVG backend are untouched by the reverted hunk, so they
must render the same view identically in both states. `DrawMapAll` renders all backends from one
invocation: for view D its Cairo image differs in **0** pixels, its Qt image in **0** and its Skia image
in **0** pixels between the two states and its SVG file is byte-identical, while its OpenGL image
differs in the same **368** pixels the `DrawMapOpenGL` run of the same view reports. Those images are
therefore also the negative control of the counting recipe.

**Tool caveat.** ImageMagick 7.1.2-34 Q16-HDRI reports `compare -metric AE` as a normalized error and
not as a pixel count (it prints `0.930719` for an image that differs in exactly one pixel), so the
counts above come from the `-threshold 0` recipe, which returns 1 for that same one-pixel control.

**Retained images.** `evidence/border-only-area-before-2026-10-16.png`,
`border-only-area-after-2026-10-16.png`, `border-only-area-difference-2026-10-16.png` (view D) and
`border-only-fence-before-2026-10-16.png`, `border-only-fence-after-2026-10-16.png`,
`border-only-fence-difference-2026-10-16.png` (view B); the difference images were made with
`-compose difference -composite -auto-level`.

